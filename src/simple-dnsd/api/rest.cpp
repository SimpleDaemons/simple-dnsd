/**
 * @file rest.cpp
 * @brief REST API subset
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/core/server.hpp"
#include "simple-dnsd/utils/logger.hpp"
#include "simple-dnsd/utils/platform.hpp"
#include "simple-dnsd/version.hpp"
#include "simple-dnsd/zone/zone.hpp"

#ifdef SIMPLE_DNSD_JSON
#include <json/json.h>
#endif

#include <sstream>

namespace simple_dnsd {

namespace {

std::string jsonEscape(const std::string &s) {
  std::string out;
  for (char ch : s) {
    if (ch == '"' || ch == '\\') {
      out.push_back('\\');
    }
    out.push_back(ch);
  }
  return out;
}

std::string zoneToJson(BackendRouter &router, const ZoneInfo &z) {
  std::vector<ResourceRecord> rrs;
  router.listZone(z, [&](const ResourceRecord &rr) { rrs.push_back(rr); });
  std::ostringstream out;
  out << "{\"id\":\"" << z.name.toString(false) << "\",\"name\":\"" << z.name.toString()
      << "\",\"kind\":\""
      << (z.kind == ZoneKind::Slave ? "Slave" : (z.kind == ZoneKind::Master ? "Master" : "Native"))
      << "\",\"rrsets\":[";
  bool first = true;
  for (auto rr : rrs) {
    if (rr.content.empty()) {
      rdataToContent(rr);
    }
    if (!first) {
      out << ",";
    }
    first = false;
    out << "{\"name\":\"" << rr.name.toString() << "\",\"type\":\"" << rrTypeToString(rr.type)
        << "\",\"ttl\":" << rr.ttl << ",\"records\":[{\"content\":\"" << jsonEscape(rr.content)
        << "\",\"disabled\":false}]}";
  }
  out << "]}";
  return out.str();
}

bool sendHttp(TcpConnection &conn, int code, const std::string &status, const std::string &body,
              const std::string &type = "application/json") {
  std::ostringstream resp;
  resp << "HTTP/1.1 " << code << " " << status << "\r\n";
  resp << "Content-Type: " << type << "\r\n";
  resp << "Content-Length: " << body.size() << "\r\n";
  resp << "Connection: close\r\n\r\n";
  resp << body;
  const auto s = resp.str();
  return conn.sendAll(std::vector<uint8_t>(s.begin(), s.end()));
}

std::string urlDecode(const std::string &s) {
  std::string out;
  for (std::size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '%' && i + 2 < s.size()) {
      out.push_back(static_cast<char>(std::stoi(s.substr(i + 1, 2), nullptr, 16)));
      i += 2;
    } else if (s[i] == '+') {
      out.push_back(' ');
    } else {
      out.push_back(s[i]);
    }
  }
  return out;
}

}  // namespace

bool handleHttpRequest(DnsServer &server, TcpConnection &conn) {
  if (!conn.waitReadable(static_cast<int>(server.engine().config().idle_timeout * 1000))) {
    return false;
  }
  uint8_t tmp[1024];
  std::string req;
  while (req.find("\r\n\r\n") == std::string::npos && req.size() < 65536) {
    if (!conn.waitReadable(2000)) {
      break;
    }
    // read incrementally via recvExact isn't ideal; use native recv
#ifdef SIMPLE_DNSD_WINDOWS
    const int n = ::recv(conn.native(), reinterpret_cast<char *>(tmp), sizeof(tmp), 0);
#else
    const ssize_t n = ::recv(conn.native(), tmp, sizeof(tmp), 0);
#endif
    if (n <= 0) {
      break;
    }
    req.append(reinterpret_cast<char *>(tmp), static_cast<std::size_t>(n));
  }
  auto line_end = req.find("\r\n");
  if (line_end == std::string::npos) {
    return sendHttp(conn, 400, "Bad Request", "{\"error\":\"bad request\"}");
  }
  const std::string request_line = req.substr(0, line_end);
  auto parts = splitWs(request_line);
  if (parts.size() < 2) {
    return sendHttp(conn, 400, "Bad Request", "{\"error\":\"bad request\"}");
  }
  const std::string method = parts[0];
  std::string path = parts[1];
  auto qpos = path.find('?');
  if (qpos != std::string::npos) {
    path = path.substr(0, qpos);
  }
  std::string api_key;
  auto key_pos = toLower(req).find("x-api-key:");
  if (key_pos != std::string::npos) {
    auto ke = req.find("\r\n", key_pos);
    api_key = trim(req.substr(key_pos + 10, ke - key_pos - 10));
  }
  const auto cfg = server.engine().config();
  if (!cfg.api_key.empty() && api_key != cfg.api_key) {
    return sendHttp(conn, 401, "Unauthorized", "{\"error\":\"unauthorized\"}");
  }
  std::string body;
  auto blank = req.find("\r\n\r\n");
  if (blank != std::string::npos) {
    body = req.substr(blank + 4);
  }
  auto &router = server.router();

  if (path == "/api/v1/servers" && method == "GET") {
    return sendHttp(conn, 200, "OK",
                    "[{\"type\":\"Server\",\"id\":\"localhost\",\"daemon_type\":\"simple-dnsd\"}]");
  }
  if (path == "/api/v1/servers/localhost" && method == "GET") {
    return sendHttp(conn, 200, "OK",
                    "{\"type\":\"Server\",\"id\":\"localhost\",\"daemon_type\":\"simple-dnsd\",\"version\":\"" +
                        std::string(kVersion) + "\"}");
  }
  if (path == "/api/v1/servers/localhost/statistics" && method == "GET") {
    auto &st = server.stats();
    std::ostringstream json;
    json << "{\"queries\":" << st.queries.load() << ",\"answers\":" << st.answers.load()
         << ",\"nxdomain\":" << st.nxdomain.load() << ",\"refused\":" << st.refused.load()
         << ",\"axfr\":" << st.axfr.load() << ",\"updates\":" << st.updates.load()
         << ",\"cache_hits\":" << st.cache_hits.load() << ",\"cache_misses\":" << st.cache_misses.load()
         << "}";
    return sendHttp(conn, 200, "OK", json.str());
  }
  if (path == "/api/v1/servers/localhost/zones" && method == "GET") {
    std::ostringstream json;
    json << "[";
    bool first = true;
    for (const auto &z : router.listZones()) {
      if (!first) {
        json << ",";
      }
      first = false;
      json << "{\"id\":\"" << z.name.toString(false) << "\",\"name\":\"" << z.name.toString()
           << "\",\"kind\":\"Native\"}";
    }
    json << "]";
    return sendHttp(conn, 200, "OK", json.str());
  }
  if (path == "/api/v1/servers/localhost/zones" && method == "POST") {
#ifdef SIMPLE_DNSD_JSON
    Json::Value root;
    Json::CharReaderBuilder b;
    std::string errs;
    std::istringstream is(body);
    if (!Json::parseFromStream(b, is, &root, &errs)) {
      return sendHttp(conn, 400, "Bad Request", "{\"error\":\"invalid json\"}");
    }
    ZoneInfo z;
    z.name = DnsName::parse(root.get("name", "").asString());
    z.kind = ZoneKind::Native;
    ResourceRecord soa;
    soa.name = z.name;
    soa.type = RrType::Soa;
    soa.content = "ns." + z.name.toString(false) + " hostmaster." + z.name.toString(false) +
                  " 1 10800 3600 604800 3600";
    if (root.isMember("rrsets")) {
      for (const auto &rrset : root["rrsets"]) {
        if (rrset.get("type", "").asString() == "SOA" && rrset.isMember("records") &&
            !rrset["records"].empty()) {
          soa.content = rrset["records"][0].get("content", soa.content).asString();
        }
      }
    }
    contentToRdata(soa);
    if (!router.createZone(z, soa)) {
      return sendHttp(conn, 409, "Conflict", "{\"error\":\"zone exists\"}");
    }
    auto created = router.findZone(z.name);
    if (created && root.isMember("rrsets")) {
      auto tx = router.begin(*created);
      if (tx) {
        for (const auto &rrset : root["rrsets"]) {
          const auto type = rrTypeFromString(rrset.get("type", "A").asString());
          if (!type || *type == RrType::Soa) {
            continue;
          }
          DnsName name = DnsName::parse(rrset.get("name", z.name.toString()).asString());
          const uint32_t ttl = rrset.get("ttl", 3600).asUInt();
          if (rrset.isMember("records")) {
            for (const auto &rec : rrset["records"]) {
              ResourceRecord rr;
              rr.name = name;
              rr.type = *type;
              rr.ttl = ttl;
              rr.content = rec.get("content", "").asString();
              contentToRdata(rr);
              tx->addRecord(rr);
            }
          }
        }
        tx->commit();
      }
    }
    return sendHttp(conn, 201, "Created", zoneToJson(router, created ? *created : z));
#else
    return sendHttp(conn, 501, "Not Implemented", "{\"error\":\"json disabled\"}");
#endif
  }

  const std::string prefix = "/api/v1/servers/localhost/zones/";
  if (path.rfind(prefix, 0) == 0) {
    std::string rest = urlDecode(path.substr(prefix.size()));
    bool want_export = false;
    if (rest.size() > 7 && rest.substr(rest.size() - 7) == "/export") {
      want_export = true;
      rest = rest.substr(0, rest.size() - 7);
    }
    DnsName zname = DnsName::parse(rest);
    auto zone = router.findZone(zname);
    if (!zone || !zone->name.equals(zname)) {
      return sendHttp(conn, 404, "Not Found", "{\"error\":\"not found\"}");
    }
    if (method == "GET" && want_export) {
      std::vector<ResourceRecord> rrs;
      router.listZone(*zone, [&](const ResourceRecord &rr) { rrs.push_back(rr); });
      return sendHttp(conn, 200, "OK", writeZoneText(zone->name, rrs), "text/plain");
    }
    if (method == "GET") {
      return sendHttp(conn, 200, "OK", zoneToJson(router, *zone));
    }
    if (method == "DELETE") {
      router.deleteZone(*zone);
      server.engine().invalidateCache();
      return sendHttp(conn, 204, "No Content", "");
    }
    if (method == "PATCH") {
#ifdef SIMPLE_DNSD_JSON
      Json::Value root;
      Json::CharReaderBuilder b;
      std::string errs;
      std::istringstream is(body);
      if (!Json::parseFromStream(b, is, &root, &errs)) {
        return sendHttp(conn, 400, "Bad Request", "{\"error\":\"invalid json\"}");
      }
      auto tx = router.begin(*zone);
      if (!tx) {
        return sendHttp(conn, 500, "Error", "{\"error\":\"no transaction\"}");
      }
      if (root.isMember("rrsets")) {
        for (const auto &rrset : root["rrsets"]) {
          const auto type = rrTypeFromString(rrset.get("type", "A").asString());
          if (!type) {
            continue;
          }
          DnsName name = DnsName::parse(rrset.get("name", zone->name.toString()).asString());
          const std::string changetype = toLower(rrset.get("changetype", "REPLACE").asString());
          if (changetype == "delete") {
            tx->deleteRrset(name, *type);
            continue;
          }
          std::vector<ResourceRecord> rrs;
          const uint32_t ttl = rrset.get("ttl", 3600).asUInt();
          if (rrset.isMember("records")) {
            for (const auto &rec : rrset["records"]) {
              ResourceRecord rr;
              rr.name = name;
              rr.type = *type;
              rr.ttl = ttl;
              rr.content = rec.get("content", "").asString();
              contentToRdata(rr);
              rrs.push_back(rr);
            }
          }
          tx->replaceRrset(name, *type, rrs);
        }
      }
      auto soa = router.lookup(*zone, zone->name, RrType::Soa);
      if (!soa.empty()) {
        tx->setSoaSerial(bumpSerial(soaSerial(soa[0])));
      }
      if (!tx->commit()) {
        return sendHttp(conn, 500, "Error", "{\"error\":\"commit failed\"}");
      }
      server.engine().invalidateCache();
      auto updated = router.findZone(zname);
      return sendHttp(conn, 200, "OK", zoneToJson(router, updated ? *updated : *zone));
#else
      return sendHttp(conn, 501, "Not Implemented", "{\"error\":\"json disabled\"}");
#endif
    }
  }
  return sendHttp(conn, 404, "Not Found", "{\"error\":\"not found\"}");
}

}  // namespace simple_dnsd
