/**
 * @file bindfile.cpp
 * @brief BIND zone-file backend
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/backend/backend.hpp"
#include "simple-dnsd/utils/logger.hpp"
#include "simple-dnsd/zone/zone.hpp"

#include <algorithm>
#include <fstream>
#include <map>
#include <mutex>

namespace simple_dnsd {

namespace {

class BindFileBackend : public Backend {
public:
  explicit BindFileBackend(std::string named_conf) : named_conf_(std::move(named_conf)) {}

  bool initialize() override {
    auto named = parseNamedConf(named_conf_);
    if (named.empty()) {
      Logger::instance().error("bind backend: no zones in " + named_conf_);
      return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    zones_.clear();
    for (auto &nz : named) {
      auto parsed = parseZoneFile(nz.file, nz.info.name);
      if (!parsed.ok) {
        Logger::instance().error(parsed.error);
        return false;
      }
      Mem z;
      z.info = nz.info;
      z.file = nz.file;
      z.records = parsed.records;
      z.info.id = static_cast<std::int64_t>(zones_.size() + 1);
      zones_[z.info.name.toLowerString()] = std::move(z);
    }
    return true;
  }

  std::string name() const override { return "bind"; }
  bool writable() const override { return true; }

  std::optional<ZoneInfo> findZone(const DnsName &qname) override {
    std::lock_guard<std::mutex> lock(mutex_);
    std::optional<ZoneInfo> best;
    std::size_t best_labels = 0;
    for (auto &pair : zones_) {
      if (qname.isSubdomainOf(pair.second.info.name) || qname.equals(pair.second.info.name)) {
        if (pair.second.info.name.labelCount() >= best_labels) {
          best_labels = pair.second.info.name.labelCount();
          best = pair.second.info;
        }
      }
    }
    return best;
  }

  std::vector<ZoneInfo> listZones() override {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ZoneInfo> out;
    for (auto &pair : zones_) {
      out.push_back(pair.second.info);
    }
    return out;
  }

  std::vector<ResourceRecord> lookup(const ZoneInfo &zone, const DnsName &qname,
                                     RrType qtype) override {
    std::lock_guard<std::mutex> lock(mutex_);
    auto *z = get(zone);
    std::vector<ResourceRecord> out;
    if (z == nullptr) {
      return out;
    }
    for (const auto &rr : z->records) {
      if (!rr.name.equals(qname)) {
        continue;
      }
      if (qtype == RrType::Any || rr.type == qtype) {
        out.push_back(rr);
      }
    }
    return out;
  }

  void listZone(const ZoneInfo &zone,
                const std::function<void(const ResourceRecord &)> &cb) override {
    std::lock_guard<std::mutex> lock(mutex_);
    auto *z = get(zone);
    if (z == nullptr) {
      return;
    }
    for (const auto &rr : z->records) {
      cb(rr);
    }
  }

  bool nameExists(const ZoneInfo &zone, const DnsName &qname) override {
    std::lock_guard<std::mutex> lock(mutex_);
    auto *z = get(zone);
    if (z == nullptr) {
      return false;
    }
    for (const auto &rr : z->records) {
      if (rr.name.equals(qname) || rr.name.isSubdomainOf(qname)) {
        return true;
      }
    }
    return false;
  }

  std::optional<DnsName> findDelegation(const ZoneInfo &zone, const DnsName &qname) override {
    std::lock_guard<std::mutex> lock(mutex_);
    auto *z = get(zone);
    if (z == nullptr) {
      return std::nullopt;
    }
    DnsName cur = qname;
    while (!cur.equals(zone.name) && !cur.empty()) {
      for (const auto &rr : z->records) {
        if (rr.name.equals(cur) && rr.type == RrType::Ns && !cur.equals(zone.name)) {
          return cur;
        }
      }
      cur = cur.parent();
    }
    return std::nullopt;
  }

  bool createZone(const ZoneInfo &zone, const ResourceRecord &soa) override {
    std::lock_guard<std::mutex> lock(mutex_);
    Mem z;
    z.info = zone;
    z.info.id = static_cast<std::int64_t>(zones_.size() + 1);
    ResourceRecord s = soa;
    s.name = zone.name;
    s.type = RrType::Soa;
    contentToRdata(s);
    z.records.push_back(s);
    zones_[zone.name.toLowerString()] = std::move(z);
    persistLocked();
    return true;
  }

  bool deleteZone(const ZoneInfo &zone) override {
    std::lock_guard<std::mutex> lock(mutex_);
    zones_.erase(zone.name.toLowerString());
    persistLocked();
    return true;
  }

  std::unique_ptr<Transaction> begin(const ZoneInfo &zone) override {
    std::lock_guard<std::mutex> lock(mutex_);
    auto *z = get(zone);
    if (z == nullptr) {
      return nullptr;
    }
    struct Tx : Transaction {
      BindFileBackend *self;
      std::string key;
      std::vector<ResourceRecord> records;
      bool addRecord(const ResourceRecord &rr) override {
        records.push_back(rr);
        return true;
      }
      bool replaceRrset(const DnsName &name, RrType type,
                        const std::vector<ResourceRecord> &rrs) override {
        deleteRrset(name, type);
        records.insert(records.end(), rrs.begin(), rrs.end());
        return true;
      }
      bool deleteRrset(const DnsName &name, RrType type) override {
        records.erase(std::remove_if(records.begin(), records.end(),
                                     [&](const ResourceRecord &rr) {
                                       return rr.name.equals(name) && rr.type == type;
                                     }),
                      records.end());
        return true;
      }
      bool deleteName(const DnsName &name) override {
        records.erase(std::remove_if(records.begin(), records.end(),
                                     [&](const ResourceRecord &rr) { return rr.name.equals(name); }),
                      records.end());
        return true;
      }
      bool createZone(const ZoneInfo &, const ResourceRecord &) override { return false; }
      bool deleteZone() override { return true; }
      bool setSoaSerial(uint32_t serial) override {
        for (auto &rr : records) {
          if (rr.type == RrType::Soa) {
            simple_dnsd::setSoaSerial(rr, serial);
          }
        }
        return true;
      }
      bool commit() override {
        std::lock_guard<std::mutex> lock(self->mutex_);
        auto it = self->zones_.find(key);
        if (it == self->zones_.end()) {
          return false;
        }
        it->second.records = records;
        self->persistLocked();
        return true;
      }
      void rollback() override {}
    };
    auto tx = std::make_unique<Tx>();
    tx->self = this;
    tx->key = zone.name.toLowerString();
    tx->records = z->records;
    return tx;
  }

private:
  struct Mem {
    ZoneInfo info;
    std::string file;
    std::vector<ResourceRecord> records;
  };
  Mem *get(const ZoneInfo &zone) {
    auto it = zones_.find(zone.name.toLowerString());
    if (it == zones_.end()) {
      return nullptr;
    }
    return &it->second;
  }
  void persistLocked() {
    std::vector<NamedZone> named;
    for (auto &pair : zones_) {
      NamedZone nz;
      nz.info = pair.second.info;
      nz.file = pair.second.file.empty() ? pair.second.info.name.toString(false) + ".zone"
                                         : pair.second.file;
      writeZoneFile(nz.file, pair.second.info.name, pair.second.records);
      named.push_back(nz);
    }
    std::ofstream out(named_conf_);
    if (out) {
      out << writeNamedConf(named);
    }
  }

  std::string named_conf_;
  std::map<std::string, Mem> zones_;
  std::mutex mutex_;
};

}  // namespace

std::unique_ptr<Backend> makeBindFileBackend(const std::string &named_conf) {
  return std::make_unique<BindFileBackend>(named_conf);
}

}  // namespace simple_dnsd
