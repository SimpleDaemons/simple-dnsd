/**
 * @file router.cpp
 * @brief Multi-backend zone router
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/backend/backend.hpp"
#include "simple-dnsd/config/config.hpp"
#include "simple-dnsd/utils/logger.hpp"
#include "simple-dnsd/zone/zone.hpp"

namespace simple_dnsd {

std::unique_ptr<Backend> makeBackend(const std::string &kind, const DnsConfig &config) {
  if (kind == "memory") {
    return makeMemoryBackend();
  }
  if (kind == "bind" || kind == "bindfile") {
    return makeBindFileBackend(config.bind_config);
  }
  if (kind == "sqlite" || kind == "gsqlite3") {
    return makeSqliteBackend(config.sqlite_file);
  }
  if (kind == "postgres" || kind == "pgsql" || kind == "gpgsql") {
    return makePostgresBackend(config.postgres_dsn);
  }
  if (kind == "mysql" || kind == "mariadb" || kind == "gmysql") {
    return makeMysqlBackend(config.mysql_dsn);
  }
  return nullptr;
}

bool loadBackends(BackendRouter &router, const DnsConfig &config) {
  for (const auto &kind : config.launchBackends()) {
    auto backend = makeBackend(kind, config);
    if (!backend) {
      Logger::instance().error("unknown or unavailable backend: " + kind);
      return false;
    }
    router.add(std::move(backend));
  }
  return router.initialize();
}

void BackendRouter::add(std::unique_ptr<Backend> backend) {
  if (backend) {
    backends_.push_back(std::move(backend));
  }
}

bool BackendRouter::initialize() {
  for (auto &b : backends_) {
    if (!b->initialize()) {
      Logger::instance().error("backend initialize failed: " + b->name());
      return false;
    }
  }
  return !backends_.empty();
}

std::optional<ZoneInfo> BackendRouter::findZone(const DnsName &qname) {
  std::optional<ZoneInfo> best;
  for (auto &b : backends_) {
    auto z = b->findZone(qname);
    if (!z) {
      continue;
    }
    if (!best || z->name.labelCount() > best->name.labelCount()) {
      best = z;
    }
  }
  return best;
}

Backend *BackendRouter::ownerOf(const ZoneInfo &zone) {
  for (auto &b : backends_) {
    auto zones = b->listZones();
    for (const auto &z : zones) {
      if (z.name.equals(zone.name)) {
        return b.get();
      }
    }
  }
  return nullptr;
}

std::vector<ZoneInfo> BackendRouter::listZones() {
  std::vector<ZoneInfo> out;
  for (auto &b : backends_) {
    auto zs = b->listZones();
    out.insert(out.end(), zs.begin(), zs.end());
  }
  return out;
}

std::vector<ResourceRecord> BackendRouter::lookup(const ZoneInfo &zone, const DnsName &qname,
                                                  RrType qtype) {
  auto *b = ownerOf(zone);
  if (b == nullptr) {
    return {};
  }
  return b->lookup(zone, qname, qtype);
}

void BackendRouter::listZone(const ZoneInfo &zone,
                             const std::function<void(const ResourceRecord &)> &cb) {
  auto *b = ownerOf(zone);
  if (b != nullptr) {
    b->listZone(zone, cb);
  }
}

bool BackendRouter::nameExists(const ZoneInfo &zone, const DnsName &qname) {
  auto *b = ownerOf(zone);
  return b != nullptr && b->nameExists(zone, qname);
}

std::optional<DnsName> BackendRouter::findDelegation(const ZoneInfo &zone, const DnsName &qname) {
  auto *b = ownerOf(zone);
  if (b == nullptr) {
    return std::nullopt;
  }
  return b->findDelegation(zone, qname);
}

Backend *BackendRouter::writableBackend() {
  for (auto &b : backends_) {
    if (b->writable()) {
      return b.get();
    }
  }
  return nullptr;
}

bool BackendRouter::createZone(const ZoneInfo &zone, const ResourceRecord &soa) {
  auto *b = writableBackend();
  return b != nullptr && b->createZone(zone, soa);
}

bool BackendRouter::deleteZone(const ZoneInfo &zone) {
  auto *b = ownerOf(zone);
  return b != nullptr && b->deleteZone(zone);
}

std::unique_ptr<Transaction> BackendRouter::begin(const ZoneInfo &zone) {
  auto *b = ownerOf(zone);
  if (b == nullptr) {
    return nullptr;
  }
  return b->begin(zone);
}

bool BackendRouter::copyZone(const ZoneInfo &from, Backend &dest) {
  std::vector<ResourceRecord> rrs;
  listZone(from, [&](const ResourceRecord &rr) { rrs.push_back(rr); });
  ResourceRecord soa;
  for (const auto &rr : rrs) {
    if (rr.type == RrType::Soa) {
      soa = rr;
      break;
    }
  }
  ZoneInfo z = from;
  z.id = -1;
  if (!dest.createZone(z, soa)) {
    return false;
  }
  auto created = dest.findZone(from.name);
  if (!created) {
    return false;
  }
  auto tx = dest.begin(*created);
  if (!tx) {
    return false;
  }
  for (const auto &rr : rrs) {
    if (rr.type != RrType::Soa) {
      tx->addRecord(rr);
    }
  }
  return tx->commit();
}

}  // namespace simple_dnsd
