/**
 * @file engine.cpp
 * @brief Authoritative DNS engine
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/core/engine.hpp"
#include "simple-dnsd/utils/logger.hpp"
#include "simple-dnsd/utils/net.hpp"
#include "simple-dnsd/zone/zone.hpp"

#include <algorithm>
#include <chrono>
#include <sstream>

namespace simple_dnsd {

namespace {

uint64_t nowMs() {
  return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::steady_clock::now().time_since_epoch())
                                   .count());
}

bool ipAllowed(const std::vector<std::string> &list, const std::string &ip) {
  if (list.empty()) {
    return false;
  }
  for (const auto &item : list) {
    if (item == "*" || item == "any" || item == ip) {
      return true;
    }
    if (item.find('/') != std::string::npos) {
      auto slash = item.find('/');
      const std::string net = item.substr(0, slash);
      if (ip == net || ip.rfind(net, 0) == 0) {
        return true;
      }
    }
  }
  return false;
}

ResourceRecord makeHinfo(const DnsName &name) {
  ResourceRecord rr;
  rr.name = name;
  rr.type = static_cast<RrType>(13);  // HINFO
  rr.ttl = 3600;
  rr.content = "\"RFC8482\" \"\"";
  std::vector<uint8_t> rdata;
  auto push = [&](const std::string &s) {
    rdata.push_back(static_cast<uint8_t>(s.size()));
    rdata.insert(rdata.end(), s.begin(), s.end());
  };
  push("RFC8482");
  push("");
  rr.rdata = rdata;
  return rr;
}

void addEdns(DnsMessage &resp, const DnsMessage &query, uint16_t udp_size) {
  if (!query.edns.present) {
    return;
  }
  ResourceRecord opt;
  opt.name = DnsName::root();
  opt.type = RrType::Opt;
  opt.rclass = static_cast<RrClass>(udp_size);
  opt.ttl = 0;
  resp.additional.push_back(opt);
  resp.edns.present = true;
  resp.edns.udp_size = udp_size;
}

}  // namespace

PacketCache::PacketCache(std::size_t max_entries) : max_entries_(max_entries) {}

void PacketCache::put(const std::string &key, const std::vector<uint8_t> &wire, uint32_t ttl) {
  if (ttl == 0 || max_entries_ == 0) {
    return;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  if (entries_.size() >= max_entries_) {
    entries_.clear();
  }
  Entry e;
  e.wire = wire;
  e.expires_ms = nowMs() + static_cast<uint64_t>(ttl) * 1000;
  entries_[key] = std::move(e);
}

bool PacketCache::get(const std::string &key, std::vector<uint8_t> &wire) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = entries_.find(key);
  if (it == entries_.end()) {
    return false;
  }
  if (it->second.expires_ms < nowMs()) {
    entries_.erase(it);
    return false;
  }
  wire = it->second.wire;
  return true;
}

void PacketCache::invalidate() {
  std::lock_guard<std::mutex> lock(mutex_);
  entries_.clear();
}

void PacketCache::clear() { invalidate(); }

ResponseRateLimiter::ResponseRateLimiter(uint32_t rate_per_sec) : rate_(rate_per_sec) {}

bool ResponseRateLimiter::allow(const std::string &ip) {
  if (rate_ == 0) {
    return true;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  const uint64_t sec = nowMs() / 1000;
  auto &b = buckets_[ip];
  if (b.first != sec) {
    b.first = sec;
    b.second = 0;
  }
  if (b.second >= rate_) {
    return false;
  }
  ++b.second;
  return true;
}

AuthoritativeEngine::AuthoritativeEngine(BackendRouter &router, DnsConfig config,
                                         ServerStats &stats)
    : router_(router),
      config_(std::move(config)),
      stats_(stats),
      cache_(config_.cache_size),
      rrl_(config_.rrl_rate) {
  if (!config_.tsig_key_name.empty() && !config_.tsig_key_secret.empty()) {
    TsigKey key;
    key.name = DnsName::parse(config_.tsig_key_name);
    decodeBase64(config_.tsig_key_secret, key.secret);
    tsig_keys_.push_back(key);
  }
}

std::optional<TsigKey> AuthoritativeEngine::tsigKey(const DnsName &name) const {
  std::lock_guard<std::mutex> lock(tsig_mutex_);
  for (const auto &k : tsig_keys_) {
    if (k.name.equals(name)) {
      return k;
    }
  }
  return std::nullopt;
}

void AuthoritativeEngine::addTsigKey(const TsigKey &key) {
  std::lock_guard<std::mutex> lock(tsig_mutex_);
  tsig_keys_.push_back(key);
}

void AuthoritativeEngine::addSoa(DnsMessage &resp, const ZoneInfo &zone) {
  auto soa = router_.lookup(zone, zone.name, RrType::Soa);
  resp.authority.insert(resp.authority.end(), soa.begin(), soa.end());
}

void AuthoritativeEngine::addGlue(DnsMessage &resp, const ZoneInfo &zone,
                                  const std::vector<ResourceRecord> &nsset) {
  for (auto ns : nsset) {
    if (ns.content.empty()) {
      rdataToContent(ns);
    }
    const DnsName target = DnsName::parse(ns.content);
    if (!target.isSubdomainOf(zone.name) && !target.equals(zone.name)) {
      continue;
    }
    auto a = router_.lookup(zone, target, RrType::A);
    auto aaaa = router_.lookup(zone, target, RrType::Aaaa);
    resp.additional.insert(resp.additional.end(), a.begin(), a.end());
    resp.additional.insert(resp.additional.end(), aaaa.begin(), aaaa.end());
  }
}

bool AuthoritativeEngine::allowedAxfr(const ZoneInfo &zone, const std::string &peer) const {
  if (ipAllowed(config_.allow_axfr, peer) || ipAllowed(zone.allow_axfr, peer)) {
    return true;
  }
  return false;
}

bool AuthoritativeEngine::allowedUpdate(const ZoneInfo &zone, const std::string &peer,
                                        const DnsMessage &query) const {
  if (ipAllowed(zone.allow_update, peer) || ipAllowed(config_.allow_axfr, peer)) {
    return true;
  }
  if (query.tsig && tsigKey(query.tsig->name)) {
    return true;
  }
  return false;
}

DnsMessage AuthoritativeEngine::handle(const DnsMessage &query, const QueryContext &ctx) {
  stats_.queries.fetch_add(1);
  if (!rrl_.allow(ctx.peer_host)) {
    auto resp = makeResponse(query);
    resp.header.rcode = Rcode::Refused;
    stats_.refused.fetch_add(1);
    return resp;
  }
  if (query.header.opcode == Opcode::Update) {
    return handleUpdate(query, ctx);
  }
  if (query.header.opcode == Opcode::Notify) {
    auto resp = makeResponse(query);
    resp.header.rcode = Rcode::NoError;
    return resp;
  }
  if (query.header.opcode != Opcode::Query) {
    auto resp = makeResponse(query);
    resp.header.rcode = Rcode::NotImp;
    return resp;
  }
  if (query.questions.empty()) {
    auto resp = makeResponse(query);
    resp.header.rcode = Rcode::FormErr;
    return resp;
  }
  const auto qtype = query.questions[0].qtype;
  if (qtype == RrType::Axfr || qtype == RrType::Ixfr) {
    auto msgs = handleAxfr(query, ctx);
    return msgs.empty() ? makeResponse(query) : msgs.front();
  }
  return answerCached(query, ctx);
}

DnsMessage AuthoritativeEngine::answerCached(const DnsMessage &query, const QueryContext &ctx) {
  const Question &q = query.questions[0];
  const std::string key = q.qname.toLowerString() + "|" + rrTypeToString(q.qtype) + "|" +
                          (ctx.tcp ? "t" : "u") + (query.edns.present ? "|e" : "");
  if (config_.cache_size > 0) {
    std::vector<uint8_t> wire;
    if (cache_.get(key, wire)) {
      DnsMessage cached;
      if (decodeMessage(wire, cached)) {
        cached.header.id = query.header.id;
        stats_.cache_hits.fetch_add(1);
        stats_.answers.fetch_add(1);
        return cached;
      }
    }
    stats_.cache_misses.fetch_add(1);
  }
  auto resp = answerQuery(query, ctx);
  if (config_.cache_size > 0 &&
      (resp.header.rcode == Rcode::NoError || resp.header.rcode == Rcode::NxDomain)) {
    uint32_t ttl = config_.cache_ttl;
    if (ttl == 0) {
      for (const auto *set : {&resp.answers, &resp.authority}) {
        for (const auto &rr : *set) {
          if (rr.type == RrType::Opt) {
            continue;
          }
          ttl = (ttl == 0) ? rr.ttl : std::min(ttl, rr.ttl);
        }
      }
    }
    if (ttl > 0) {
      cache_.put(key, encodeMessage(resp), ttl);
    }
  }
  return resp;
}

DnsMessage AuthoritativeEngine::answerQuery(const DnsMessage &query, const QueryContext &ctx) {
  auto resp = makeResponse(query);
  const Question &q = query.questions[0];

  auto zone = router_.findZone(q.qname);
  if (!zone) {
    resp.header.rcode = Rcode::Refused;
    addEdns(resp, query, static_cast<uint16_t>(config_.edns_bufsize));
    stats_.refused.fetch_add(1);
    return resp;
  }

  auto cut = router_.findDelegation(*zone, q.qname);
  if (cut && !cut->equals(q.qname)) {
    resp.header.aa = false;
    auto ns = router_.lookup(*zone, *cut, RrType::Ns);
    resp.authority.insert(resp.authority.end(), ns.begin(), ns.end());
    addGlue(resp, *zone, ns);
    addEdns(resp, query, static_cast<uint16_t>(config_.edns_bufsize));
    stats_.answers.fetch_add(1);
    return resp;
  }
  if (cut && cut->equals(q.qname) && q.qtype != RrType::Ns && q.qtype != RrType::Ds) {
    resp.header.aa = false;
    auto ns = router_.lookup(*zone, *cut, RrType::Ns);
    resp.authority.insert(resp.authority.end(), ns.begin(), ns.end());
    addGlue(resp, *zone, ns);
    addEdns(resp, query, static_cast<uint16_t>(config_.edns_bufsize));
    stats_.answers.fetch_add(1);
    return resp;
  }

  resp.header.aa = true;

  if (q.qtype == RrType::Any) {
    resp.answers.push_back(makeHinfo(q.qname));
    addEdns(resp, query, static_cast<uint16_t>(config_.edns_bufsize));
    stats_.answers.fetch_add(1);
    return resp;
  }

  DnsName chase = q.qname;
  for (int hop = 0; hop < 8; ++hop) {
    auto cname = router_.lookup(*zone, chase, RrType::Cname);
    auto match = router_.lookup(*zone, chase, q.qtype);
    if (q.qtype != RrType::Cname && !cname.empty()) {
      resp.answers.insert(resp.answers.end(), cname.begin(), cname.end());
      ResourceRecord cn = cname[0];
      if (cn.content.empty()) {
        rdataToContent(cn);
      }
      chase = DnsName::parse(cn.content);
      if (!chase.isSubdomainOf(zone->name) && !chase.equals(zone->name)) {
        addEdns(resp, query, static_cast<uint16_t>(config_.edns_bufsize));
        stats_.answers.fetch_add(1);
        return resp;
      }
      continue;
    }
    if (!match.empty()) {
      resp.answers.insert(resp.answers.end(), match.begin(), match.end());
      if (q.qtype == RrType::Ns || q.qtype == RrType::Mx) {
        addGlue(resp, *zone, match);
      }
      addEdns(resp, query, static_cast<uint16_t>(config_.edns_bufsize));
      stats_.answers.fetch_add(1);
      return resp;
    }
    break;
  }

  DnsName parent = q.qname.parent();
  while (parent.isSubdomainOf(zone->name) || parent.equals(zone->name) || parent.empty()) {
    auto wild = router_.lookup(*zone, DnsName::wildcard(parent), q.qtype);
    auto wild_cname = router_.lookup(*zone, DnsName::wildcard(parent), RrType::Cname);
    if (!wild.empty() || !wild_cname.empty()) {
      auto synth = wild.empty() ? wild_cname : wild;
      for (auto rr : synth) {
        rr.name = q.qname;
        resp.answers.push_back(rr);
      }
      addEdns(resp, query, static_cast<uint16_t>(config_.edns_bufsize));
      stats_.answers.fetch_add(1);
      return resp;
    }
    if (parent.equals(zone->name) || parent.empty()) {
      break;
    }
    parent = parent.parent();
  }

  if (router_.nameExists(*zone, q.qname)) {
    addSoa(resp, *zone);
    addEdns(resp, query, static_cast<uint16_t>(config_.edns_bufsize));
    stats_.answers.fetch_add(1);
    return resp;
  }

  resp.header.rcode = Rcode::NxDomain;
  addSoa(resp, *zone);
  addEdns(resp, query, static_cast<uint16_t>(config_.edns_bufsize));
  stats_.nxdomain.fetch_add(1);
  return resp;
}

std::vector<DnsMessage> AuthoritativeEngine::handleAxfr(const DnsMessage &query,
                                                        const QueryContext &ctx) {
  std::vector<DnsMessage> out;
  auto resp = makeResponse(query);
  if (!ctx.tcp) {
    resp.header.rcode = Rcode::FormErr;
    out.push_back(resp);
    return out;
  }
  if (query.questions.empty()) {
    resp.header.rcode = Rcode::FormErr;
    out.push_back(resp);
    return out;
  }
  auto zone = router_.findZone(query.questions[0].qname);
  if (!zone || !zone->name.equals(query.questions[0].qname)) {
    resp.header.rcode = Rcode::Refused;
    stats_.refused.fetch_add(1);
    out.push_back(resp);
    return out;
  }
  if (!allowedAxfr(*zone, ctx.peer_host) && query.tsig == std::nullopt) {
    resp.header.rcode = Rcode::Refused;
    stats_.refused.fetch_add(1);
    out.push_back(resp);
    return out;
  }
  if (query.tsig) {
    auto key = tsigKey(query.tsig->name);
    std::string err;
    if (!key || !verifyMessage(query, ctx.raw, *key, err)) {
      resp.header.rcode = Rcode::NotAuth;
      out.push_back(resp);
      return out;
    }
  }
  resp.header.aa = true;
  std::vector<ResourceRecord> rrs;
  router_.listZone(*zone, [&](const ResourceRecord &rr) { rrs.push_back(rr); });
  ResourceRecord soa;
  std::vector<ResourceRecord> rest;
  for (const auto &rr : rrs) {
    if (rr.type == RrType::Soa) {
      soa = rr;
    } else {
      rest.push_back(rr);
    }
  }
  if (soa.rdata.empty() && soa.content.empty()) {
    resp.header.rcode = Rcode::ServFail;
    out.push_back(resp);
    return out;
  }
  resp.answers.push_back(soa);
  resp.answers.insert(resp.answers.end(), rest.begin(), rest.end());
  resp.answers.push_back(soa);
  stats_.axfr.fetch_add(1);
  out.push_back(resp);
  return out;
}

DnsMessage AuthoritativeEngine::handleUpdate(const DnsMessage &query, const QueryContext &ctx) {
  auto resp = makeResponse(query);
  stats_.updates.fetch_add(1);
  if (query.questions.empty()) {
    resp.header.rcode = Rcode::FormErr;
    return resp;
  }
  auto zone = router_.findZone(query.questions[0].qname);
  if (!zone) {
    resp.header.rcode = Rcode::NotAuth;
    return resp;
  }
  if (!allowedUpdate(*zone, ctx.peer_host, query)) {
    resp.header.rcode = Rcode::Refused;
    return resp;
  }
  if (query.tsig) {
    auto key = tsigKey(query.tsig->name);
    std::string err;
    if (!key || !verifyMessage(query, ctx.raw, *key, err)) {
      resp.header.rcode = Rcode::NotAuth;
      return resp;
    }
  }
  auto tx = router_.begin(*zone);
  if (!tx) {
    resp.header.rcode = Rcode::ServFail;
    return resp;
  }
  // RFC 2136: zone section is questions; update section is authorities.
  for (const auto &rr : query.authority) {
    if (rr.rclass == RrClass::Any && rr.rdata.empty()) {
      if (rr.type == RrType::Any) {
        tx->deleteName(rr.name);
      } else {
        tx->deleteRrset(rr.name, rr.type);
      }
    } else if (static_cast<uint16_t>(rr.rclass) == 254) {  // NONE
      tx->deleteRrset(rr.name, rr.type);
    } else {
      tx->addRecord(rr);
    }
  }
  auto soa = router_.lookup(*zone, zone->name, RrType::Soa);
  if (!soa.empty()) {
    tx->setSoaSerial(bumpSerial(soaSerial(soa[0])));
  }
  if (!tx->commit()) {
    resp.header.rcode = Rcode::ServFail;
    return resp;
  }
  cache_.invalidate();
  notifySecondaries(*zone);
  if (config_.continuous_export && !config_.export_dir.empty()) {
    exportBind(router_, zone->name, config_.export_dir, config_.export_named_conf);
  }
  resp.header.rcode = Rcode::NoError;
  return resp;
}

void AuthoritativeEngine::notifySecondaries(const ZoneInfo &zone) {
  std::vector<std::string> targets = config_.also_notify;
  targets.insert(targets.end(), zone.also_notify.begin(), zone.also_notify.end());
  auto ns = router_.lookup(zone, zone.name, RrType::Ns);
  for (auto rr : ns) {
    if (rr.content.empty()) {
      rdataToContent(rr);
    }
    auto glue = router_.lookup(zone, DnsName::parse(rr.content), RrType::A);
    for (auto g : glue) {
      if (g.content.empty()) {
        rdataToContent(g);
      }
      targets.push_back(g.content);
    }
  }
  DnsMessage notify;
  notify.header.opcode = Opcode::Notify;
  notify.header.aa = true;
  notify.header.id = 1;
  Question q;
  q.qname = zone.name;
  q.qtype = RrType::Soa;
  notify.questions.push_back(q);
  auto soa = router_.lookup(zone, zone.name, RrType::Soa);
  notify.answers.insert(notify.answers.end(), soa.begin(), soa.end());
  const auto wire = encodeMessage(notify);
  for (const auto &ip : targets) {
    if (ip.empty()) {
      continue;
    }
    std::string host = ip;
    port_t port = 53;
    auto colon = ip.rfind(':');
    if (colon != std::string::npos && ip.find('.') != std::string::npos) {
      host = ip.substr(0, colon);
      port = static_cast<port_t>(std::stoi(ip.substr(colon + 1)));
    }
    std::vector<uint8_t> reply;
    UdpSocket::sendOnce(host, port, wire, reply, 500);
  }
}

}  // namespace simple_dnsd
