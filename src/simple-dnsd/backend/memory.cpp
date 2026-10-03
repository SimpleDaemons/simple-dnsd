/**
 * @file memory.cpp
 * @brief In-memory DNS backend
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/backend/backend.hpp"
#include "simple-dnsd/zone/zone.hpp"

#include <algorithm>
#include <map>
#include <mutex>

namespace simple_dnsd {

namespace {

struct MemZone {
  ZoneInfo info;
  std::vector<ResourceRecord> records;
};

class MemoryTransaction : public Transaction {
public:
  MemoryTransaction(MemZone *zone, std::mutex *mutex) : zone_(zone), mutex_(mutex) {}

  bool addRecord(const ResourceRecord &rr) override {
    pending_.push_back(rr);
    adds_.push_back(true);
    return true;
  }
  bool replaceRrset(const DnsName &name, RrType type,
                    const std::vector<ResourceRecord> &rrs) override {
    deleteRrset(name, type);
    for (const auto &rr : rrs) {
      addRecord(rr);
    }
    return true;
  }
  bool deleteRrset(const DnsName &name, RrType type) override {
    deletes_.push_back({name, type, false});
    return true;
  }
  bool deleteName(const DnsName &name) override {
    deletes_.push_back({name, RrType::Any, true});
    return true;
  }
  bool createZone(const ZoneInfo &, const ResourceRecord &) override { return false; }
  bool deleteZone() override {
    drop_zone_ = true;
    return true;
  }
  bool setSoaSerial(uint32_t serial) override {
    serial_ = serial;
    has_serial_ = true;
    return true;
  }
  bool commit() override {
    std::lock_guard<std::mutex> lock(*mutex_);
    if (drop_zone_) {
      zone_->records.clear();
      return true;
    }
    for (const auto &d : deletes_) {
      zone_->records.erase(std::remove_if(zone_->records.begin(), zone_->records.end(),
                                          [&](const ResourceRecord &rr) {
                                            if (!rr.name.equals(d.name)) {
                                              return false;
                                            }
                                            return d.all || rr.type == d.type;
                                          }),
                           zone_->records.end());
    }
    for (auto rr : pending_) {
      if (rr.rdata.empty()) {
        contentToRdata(rr);
      }
      zone_->records.push_back(rr);
    }
    if (has_serial_) {
      for (auto &rr : zone_->records) {
        if (rr.type == RrType::Soa && rr.name.equals(zone_->info.name)) {
          simple_dnsd::setSoaSerial(rr, serial_);
        }
      }
    }
    return true;
  }
  void rollback() override {
    pending_.clear();
    deletes_.clear();
  }

private:
  struct Del {
    DnsName name;
    RrType type;
    bool all;
  };
  MemZone *zone_;
  std::mutex *mutex_;
  std::vector<ResourceRecord> pending_;
  std::vector<bool> adds_;
  std::vector<Del> deletes_;
  bool drop_zone_{false};
  bool has_serial_{false};
  uint32_t serial_{0};
};

class MemoryBackend : public Backend {
public:
  bool initialize() override { return true; }
  std::string name() const override { return "memory"; }
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
      if (rr.disabled || !rr.name.equals(qname)) {
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
      if (rr.name.equals(qname)) {
        return true;
      }
      if (rr.name.isSubdomainOf(qname)) {
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
      bool has_ns = false;
      for (const auto &rr : z->records) {
        if (rr.name.equals(cur) && rr.type == RrType::Ns && !cur.equals(zone.name)) {
          has_ns = true;
          break;
        }
      }
      if (has_ns) {
        return cur;
      }
      cur = cur.parent();
    }
    return std::nullopt;
  }

  std::unique_ptr<Transaction> begin(const ZoneInfo &zone) override {
    std::lock_guard<std::mutex> lock(mutex_);
    auto *z = get(zone);
    if (z == nullptr) {
      return nullptr;
    }
    return std::make_unique<MemoryTransaction>(z, &mutex_);
  }

  bool createZone(const ZoneInfo &zone, const ResourceRecord &soa) override {
    std::lock_guard<std::mutex> lock(mutex_);
    const std::string key = zone.name.toLowerString();
    if (zones_.count(key)) {
      return false;
    }
    MemZone mz;
    mz.info = zone;
    mz.info.id = static_cast<std::int64_t>(zones_.size() + 1);
    ResourceRecord s = soa;
    s.name = zone.name;
    s.type = RrType::Soa;
    if (s.rdata.empty()) {
      contentToRdata(s);
    }
    mz.records.push_back(s);
    zones_[key] = std::move(mz);
    return true;
  }

  bool deleteZone(const ZoneInfo &zone) override {
    std::lock_guard<std::mutex> lock(mutex_);
    return zones_.erase(zone.name.toLowerString()) > 0;
  }

private:
  MemZone *get(const ZoneInfo &zone) {
    auto it = zones_.find(zone.name.toLowerString());
    if (it == zones_.end()) {
      return nullptr;
    }
    return &it->second;
  }

  std::map<std::string, MemZone> zones_;
  std::mutex mutex_;
};

}  // namespace

std::unique_ptr<Backend> makeMemoryBackend() { return std::make_unique<MemoryBackend>(); }

}  // namespace simple_dnsd
