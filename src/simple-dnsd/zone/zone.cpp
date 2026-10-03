/**
 * @file zone.cpp
 * @brief BIND master-file parser, writer, and checks
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/zone/zone.hpp"
#include "simple-dnsd/utils/logger.hpp"
#include "simple-dnsd/utils/platform.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#ifndef SIMPLE_DNSD_WINDOWS
#include <dirent.h>
#endif

namespace simple_dnsd {

namespace {

bool isAbsName(const std::string &s) { return !s.empty() && s.back() == '.'; }

DnsName qualifyName(const std::string &token, const DnsName &origin) {
  if (token == "@") {
    return origin;
  }
  DnsName n = DnsName::parse(token);
  if (isAbsName(token)) {
    return n;
  }
  return n.qualify(origin);
}

std::string stripComments(const std::string &line) {
  std::string out;
  bool in_quote = false;
  for (char ch : line) {
    if (ch == '"') {
      in_quote = !in_quote;
    }
    if (ch == ';' && !in_quote) {
      break;
    }
    out.push_back(ch);
  }
  return out;
}

std::vector<std::string> tokenize(const std::string &text) {
  std::vector<std::string> toks;
  std::string cur;
  bool in_quote = false;
  for (char ch : text) {
    if (ch == '"') {
      cur.push_back(ch);
      in_quote = !in_quote;
      continue;
    }
    if (!in_quote && std::isspace(static_cast<unsigned char>(ch))) {
      if (!cur.empty()) {
        toks.push_back(cur);
        cur.clear();
      }
    } else {
      cur.push_back(ch);
    }
  }
  if (!cur.empty()) {
    toks.push_back(cur);
  }
  return toks;
}

std::string flattenParens(const std::string &text) {
  std::string out;
  int depth = 0;
  bool in_quote = false;
  for (char ch : text) {
    if (ch == '"') {
      in_quote = !in_quote;
    }
    if (!in_quote && ch == '(') {
      ++depth;
      out.push_back(' ');
      continue;
    }
    if (!in_quote && ch == ')') {
      if (depth > 0) {
        --depth;
      }
      out.push_back(' ');
      continue;
    }
    if (ch == '\n' && depth > 0) {
      out.push_back(' ');
      continue;
    }
    out.push_back(ch);
  }
  return out;
}

bool looksLikeTtl(const std::string &s) {
  if (s.empty()) {
    return false;
  }
  for (char ch : s) {
    if (!std::isdigit(static_cast<unsigned char>(ch))) {
      return false;
    }
  }
  return true;
}

bool looksLikeClass(const std::string &s) {
  const std::string u = toLower(s);
  return u == "in" || u == "ch" || u == "hs" || u == "any";
}

}  // namespace

ZoneParseResult parseZoneText(const std::string &text, const DnsName &origin,
                              uint32_t default_ttl, const std::string &base_dir) {
  ZoneParseResult result;
  result.origin = origin;
  DnsName current_origin = origin;
  DnsName last_owner = origin;
  uint32_t ttl = default_ttl;
  const std::string flat = flattenParens(text);
  std::istringstream in(flat);
  std::string raw;
  int lineno = 0;
  while (std::getline(in, raw)) {
    ++lineno;
    const std::string line = trim(stripComments(raw));
    if (line.empty()) {
      continue;
    }
    auto toks = tokenize(line);
    if (toks.empty()) {
      continue;
    }
    if (toLower(toks[0]) == "$origin") {
      if (toks.size() < 2) {
        result.error = "$ORIGIN missing name at line " + std::to_string(lineno);
        return result;
      }
      current_origin = qualifyName(toks[1], current_origin);
      continue;
    }
    if (toLower(toks[0]) == "$ttl") {
      if (toks.size() < 2) {
        result.error = "$TTL missing value at line " + std::to_string(lineno);
        return result;
      }
      ttl = static_cast<uint32_t>(std::stoul(toks[1]));
      continue;
    }
    if (toLower(toks[0]) == "$include") {
      if (toks.size() < 2) {
        result.error = "$INCLUDE missing file at line " + std::to_string(lineno);
        return result;
      }
      std::string path = toks[1];
      if (path.front() != '/' && !base_dir.empty()) {
        path = base_dir + "/" + path;
      }
      DnsName inc_origin = current_origin;
      if (toks.size() >= 3) {
        inc_origin = qualifyName(toks[2], current_origin);
      }
      auto inc = parseZoneFile(path, inc_origin, ttl);
      if (!inc.ok) {
        result.error = inc.error;
        return result;
      }
      result.records.insert(result.records.end(), inc.records.begin(), inc.records.end());
      continue;
    }
    std::size_t i = 0;
    DnsName owner;
    if (toks[0] == "@" || (!looksLikeTtl(toks[0]) && !looksLikeClass(toks[0]) &&
                           rrTypeFromString(toks[0]) == std::nullopt)) {
      owner = qualifyName(toks[0], current_origin);
      last_owner = owner;
      i = 1;
    } else {
      owner = last_owner;
    }
    uint32_t rec_ttl = ttl;
    if (i < toks.size() && looksLikeTtl(toks[i])) {
      rec_ttl = static_cast<uint32_t>(std::stoul(toks[i]));
      ++i;
    }
    if (i < toks.size() && looksLikeClass(toks[i])) {
      ++i;
    }
    if (i < toks.size() && looksLikeTtl(toks[i])) {
      rec_ttl = static_cast<uint32_t>(std::stoul(toks[i]));
      ++i;
    }
    if (i >= toks.size()) {
      result.error = "missing type at line " + std::to_string(lineno);
      return result;
    }
    auto type = rrTypeFromString(toks[i]);
    if (!type) {
      result.error = "unknown type " + toks[i] + " at line " + std::to_string(lineno);
      return result;
    }
    ++i;
    ResourceRecord rr;
    rr.name = owner;
    rr.type = *type;
    rr.ttl = rec_ttl;
    rr.content = join(std::vector<std::string>(toks.begin() + static_cast<std::ptrdiff_t>(i),
                                               toks.end()),
                      " ");
    if (*type == RrType::Ns || *type == RrType::Cname || *type == RrType::Ptr ||
        *type == RrType::Mx || *type == RrType::Soa || *type == RrType::Srv) {
      auto parts = splitWs(rr.content);
      if (!parts.empty()) {
        const std::size_t host_idx =
            (*type == RrType::Mx) ? 1 : (*type == RrType::Soa ? 0 : (*type == RrType::Srv ? 3 : 0));
        if (parts.size() > host_idx && !isAbsName(parts[host_idx]) && parts[host_idx] != "@") {
          parts[host_idx] = qualifyName(parts[host_idx], current_origin).toString();
        }
        if (*type == RrType::Soa && parts.size() > 1 && !isAbsName(parts[1]) && parts[1] != "@") {
          parts[1] = qualifyName(parts[1], current_origin).toString();
        }
        if (parts[0] == "@") {
          parts[0] = current_origin.toString();
        }
        rr.content = join(parts, " ");
      }
    }
    if (!contentToRdata(rr)) {
      result.error = "bad rdata at line " + std::to_string(lineno) + ": " + rr.content;
      return result;
    }
    result.records.push_back(std::move(rr));
  }
  result.ok = true;
  return result;
}

ZoneParseResult parseZoneFile(const std::string &path, const DnsName &origin,
                              uint32_t default_ttl) {
  std::ifstream in(path);
  if (!in) {
    ZoneParseResult r;
    r.error = "cannot open zone file: " + path;
    return r;
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  std::string base = path;
  auto slash = base.find_last_of("/\\");
  if (slash != std::string::npos) {
    base = base.substr(0, slash);
  } else {
    base = ".";
  }
  return parseZoneText(ss.str(), origin, default_ttl, base);
}

std::string writeZoneText(const DnsName &origin, const std::vector<ResourceRecord> &rrs) {
  std::ostringstream out;
  out << "$ORIGIN " << origin.toString() << "\n";
  out << "$TTL 3600\n";
  for (auto rr : rrs) {
    if (rr.content.empty()) {
      rdataToContent(rr);
    }
    const auto rel = rr.name.makeRelative(origin);
    const std::string owner = rel.empty() ? "@" : rel.toString(false);
    out << owner << "\t" << rr.ttl << "\tIN\t" << rrTypeToString(rr.type) << "\t"
        << rr.content << "\n";
  }
  return out.str();
}

bool writeZoneFile(const std::string &path, const DnsName &origin,
                   const std::vector<ResourceRecord> &rrs) {
  std::ofstream out(path);
  if (!out) {
    return false;
  }
  out << writeZoneText(origin, rrs);
  return static_cast<bool>(out);
}

std::vector<NamedZone> parseNamedConf(const std::string &path) {
  std::ifstream in(path);
  std::vector<NamedZone> zones;
  if (!in) {
    return zones;
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  const std::string text = ss.str();
  std::size_t pos = 0;
  while (true) {
    auto z = toLower(text).find("zone", pos);
    if (z == std::string::npos) {
      break;
    }
    auto q1 = text.find('"', z);
    if (q1 == std::string::npos) {
      break;
    }
    auto q2 = text.find('"', q1 + 1);
    if (q2 == std::string::npos) {
      break;
    }
    NamedZone nz;
    nz.info.name = DnsName::parse(text.substr(q1 + 1, q2 - q1 - 1));
    auto brace = text.find('{', q2);
    auto end = text.find('}', brace == std::string::npos ? q2 : brace);
    if (end == std::string::npos) {
      break;
    }
    const std::string body = text.substr(brace + 1, end - brace - 1);
    auto type_pos = toLower(body).find("type");
    if (type_pos != std::string::npos) {
      auto rest = splitWs(body.substr(type_pos + 4));
      if (!rest.empty()) {
        std::string t = toLower(rest[0]);
        if (!t.empty() && t.back() == ';') {
          t.pop_back();
        }
        if (t == "slave" || t == "secondary") {
          nz.info.kind = ZoneKind::Slave;
        } else {
          nz.info.kind = ZoneKind::Master;
        }
      }
    }
    auto file_pos = toLower(body).find("file");
    if (file_pos != std::string::npos) {
      auto fq1 = body.find('"', file_pos);
      auto fq2 = body.find('"', fq1 == std::string::npos ? file_pos : fq1 + 1);
      if (fq1 != std::string::npos && fq2 != std::string::npos) {
        nz.file = body.substr(fq1 + 1, fq2 - fq1 - 1);
        nz.info.file = nz.file;
      }
    }
    auto masters = toLower(body).find("masters");
    if (masters != std::string::npos) {
      auto mb = body.find('{', masters);
      auto me = body.find('}', mb == std::string::npos ? masters : mb);
      if (mb != std::string::npos && me != std::string::npos) {
        auto inner = body.substr(mb + 1, me - mb - 1);
        inner.erase(std::remove(inner.begin(), inner.end(), ';'), inner.end());
        nz.info.master = trim(inner);
      }
    }
    zones.push_back(nz);
    pos = end + 1;
  }
  return zones;
}

std::string writeNamedConf(const std::vector<NamedZone> &zones) {
  std::ostringstream out;
  for (const auto &z : zones) {
    out << "zone \"" << z.info.name.toString(false) << "\" {\n";
    out << "    type " << (z.info.kind == ZoneKind::Slave ? "slave" : "master") << ";\n";
    out << "    file \"" << (z.file.empty() ? z.info.name.toString(false) + ".zone" : z.file)
        << "\";\n";
    if (!z.info.master.empty()) {
      out << "    masters { " << z.info.master << "; };\n";
    }
    out << "};\n";
  }
  return out.str();
}

ZoneCheck checkZone(const DnsName &origin, const std::vector<ResourceRecord> &rrs) {
  ZoneCheck c;
  bool has_soa = false;
  bool has_ns = false;
  for (const auto &rr : rrs) {
    if (!rr.name.isSubdomainOf(origin) && !rr.name.equals(origin)) {
      c.ok = false;
      c.errors.push_back("out-of-zone data: " + rr.name.toString());
    }
    if (rr.name.equals(origin) && rr.type == RrType::Soa) {
      has_soa = true;
    }
    if (rr.name.equals(origin) && rr.type == RrType::Ns) {
      has_ns = true;
    }
  }
  if (!has_soa) {
    c.ok = false;
    c.errors.emplace_back("missing SOA at apex");
  }
  if (!has_ns) {
    c.ok = false;
    c.errors.emplace_back("missing NS at apex");
  }
  return c;
}

std::vector<ResourceRecord> rectifyZone(const DnsName &origin,
                                        std::vector<ResourceRecord> rrs) {
  (void)origin;
  for (auto &rr : rrs) {
    if (rr.rdata.empty()) {
      contentToRdata(rr);
    }
    if (rr.content.empty()) {
      rdataToContent(rr);
    }
  }
  return rrs;
}

uint32_t soaSerial(const ResourceRecord &soa) {
  auto parts = splitWs(soa.content);
  if (parts.size() < 3) {
    ResourceRecord copy = soa;
    if (copy.content.empty()) {
      rdataToContent(copy);
    }
    parts = splitWs(copy.content);
  }
  if (parts.size() < 3) {
    return 0;
  }
  try {
    return static_cast<uint32_t>(std::stoul(parts[2]));
  } catch (...) {
    return 0;
  }
}

bool setSoaSerial(ResourceRecord &soa, uint32_t serial) {
  if (soa.content.empty()) {
    rdataToContent(soa);
  }
  auto parts = splitWs(soa.content);
  if (parts.size() < 7) {
    return false;
  }
  parts[2] = std::to_string(serial);
  soa.content = join(parts, " ");
  soa.rdata.clear();
  return contentToRdata(soa);
}

uint32_t bumpSerial(uint32_t serial) { return serial + 1; }

bool exportBind(BackendRouter &router, const std::optional<DnsName> &zone,
                const std::string &out_dir, const std::string &named_conf_path) {
#ifndef SIMPLE_DNSD_WINDOWS
  mkdir(out_dir.c_str(), 0755);
#endif
  std::vector<NamedZone> named;
  auto zones = router.listZones();
  for (const auto &z : zones) {
    if (zone && !z.name.equals(*zone)) {
      continue;
    }
    std::vector<ResourceRecord> rrs;
    router.listZone(z, [&](const ResourceRecord &rr) { rrs.push_back(rr); });
    const std::string file = out_dir + "/" + z.name.toString(false) + ".zone";
    if (!writeZoneFile(file, z.name, rrs)) {
      return false;
    }
    NamedZone nz;
    nz.info = z;
    nz.file = file;
    named.push_back(nz);
  }
  if (!named_conf_path.empty()) {
    std::ofstream out(named_conf_path);
    if (!out) {
      return false;
    }
    out << writeNamedConf(named);
  }
  return true;
}

bool importBind(BackendRouter &router, const std::string &named_conf_or_zone,
                const DnsName &origin) {
  auto named = parseNamedConf(named_conf_or_zone);
  if (named.empty()) {
    DnsName org = origin;
    if (org.empty()) {
      org = DnsName::parse(".");
    }
    auto parsed = parseZoneFile(named_conf_or_zone, org);
    if (!parsed.ok) {
      return false;
    }
    ZoneInfo z;
    z.name = parsed.origin.empty() ? org : parsed.origin;
    z.kind = ZoneKind::Native;
    ResourceRecord soa;
    for (const auto &rr : parsed.records) {
      if (rr.type == RrType::Soa) {
        soa = rr;
        z.name = rr.name;
        break;
      }
    }
    if (soa.rdata.empty() && soa.content.empty()) {
      return false;
    }
    if (!router.createZone(z, soa)) {
      auto existing = router.findZone(z.name);
      if (!existing) {
        return false;
      }
      z = *existing;
    }
    auto tx = router.begin(z);
    if (!tx) {
      return false;
    }
    for (const auto &rr : parsed.records) {
      if (rr.type != RrType::Soa) {
        tx->addRecord(rr);
      }
    }
    return tx->commit();
  }
  for (auto &nz : named) {
    auto parsed = parseZoneFile(nz.file, nz.info.name);
    if (!parsed.ok) {
      Logger::instance().error(parsed.error);
      return false;
    }
    ResourceRecord soa;
    for (const auto &rr : parsed.records) {
      if (rr.type == RrType::Soa) {
        soa = rr;
        break;
      }
    }
    if (!router.createZone(nz.info, soa)) {
      continue;
    }
    auto created = router.findZone(nz.info.name);
    if (!created) {
      continue;
    }
    auto tx = router.begin(*created);
    if (!tx) {
      continue;
    }
    for (const auto &rr : parsed.records) {
      if (rr.type != RrType::Soa) {
        tx->addRecord(rr);
      }
    }
    tx->commit();
  }
  return true;
}

}  // namespace simple_dnsd
