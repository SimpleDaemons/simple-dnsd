/**
 * @file backend.hpp
 * @brief Pluggable DNS backends
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-dnsd/protocol/dns.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace simple_dnsd {

enum class ZoneKind { Native, Master, Slave };

struct ZoneInfo {
  std::int64_t id{-1};
  DnsName name;
  ZoneKind kind{ZoneKind::Native};
  std::string master;
  std::string account;
  std::string file;
  std::vector<std::string> also_notify;
  std::vector<std::string> allow_axfr;
  std::vector<std::string> allow_update;
  uint32_t notified_serial{0};
};

class Transaction {
public:
  virtual ~Transaction() = default;
  virtual bool addRecord(const ResourceRecord &rr) = 0;
  virtual bool replaceRrset(const DnsName &name, RrType type,
                            const std::vector<ResourceRecord> &rrs) = 0;
  virtual bool deleteRrset(const DnsName &name, RrType type) = 0;
  virtual bool deleteName(const DnsName &name) = 0;
  virtual bool createZone(const ZoneInfo &zone, const ResourceRecord &soa) = 0;
  virtual bool deleteZone() = 0;
  virtual bool setSoaSerial(uint32_t serial) = 0;
  virtual bool commit() = 0;
  virtual void rollback() = 0;
};

class Backend {
public:
  virtual ~Backend() = default;
  virtual bool initialize() = 0;
  virtual std::string name() const = 0;
  virtual bool writable() const { return false; }

  virtual std::optional<ZoneInfo> findZone(const DnsName &qname) = 0;
  virtual std::vector<ZoneInfo> listZones() = 0;
  virtual std::vector<ResourceRecord> lookup(const ZoneInfo &zone, const DnsName &qname,
                                             RrType qtype) = 0;
  virtual void listZone(const ZoneInfo &zone,
                        const std::function<void(const ResourceRecord &)> &cb) = 0;
  virtual bool nameExists(const ZoneInfo &zone, const DnsName &qname) = 0;
  virtual std::optional<DnsName> findDelegation(const ZoneInfo &zone,
                                                const DnsName &qname) = 0;

  virtual std::unique_ptr<Transaction> begin(const ZoneInfo &zone) {
    (void)zone;
    return nullptr;
  }
  virtual bool createZone(const ZoneInfo &zone, const ResourceRecord &soa) {
    (void)zone;
    (void)soa;
    return false;
  }
  virtual bool deleteZone(const ZoneInfo &zone) {
    (void)zone;
    return false;
  }
};

std::unique_ptr<Backend> makeMemoryBackend();
std::unique_ptr<Backend> makeBindFileBackend(const std::string &named_conf);
std::unique_ptr<Backend> makeSqliteBackend(const std::string &path,
                                           const std::string &schema_sql = {});
std::unique_ptr<Backend> makePostgresBackend(const std::string &dsn);
std::unique_ptr<Backend> makeMysqlBackend(const std::string &dsn);

class BackendRouter {
public:
  void add(std::unique_ptr<Backend> backend);
  bool initialize();
  const std::vector<std::unique_ptr<Backend>> &backends() const { return backends_; }

  std::optional<ZoneInfo> findZone(const DnsName &qname);
  Backend *ownerOf(const ZoneInfo &zone);
  std::vector<ZoneInfo> listZones();
  std::vector<ResourceRecord> lookup(const ZoneInfo &zone, const DnsName &qname,
                                     RrType qtype);
  void listZone(const ZoneInfo &zone,
                const std::function<void(const ResourceRecord &)> &cb);
  bool nameExists(const ZoneInfo &zone, const DnsName &qname);
  std::optional<DnsName> findDelegation(const ZoneInfo &zone, const DnsName &qname);

  Backend *writableBackend();
  bool createZone(const ZoneInfo &zone, const ResourceRecord &soa);
  bool deleteZone(const ZoneInfo &zone);
  std::unique_ptr<Transaction> begin(const ZoneInfo &zone);
  bool copyZone(const ZoneInfo &from, Backend &dest);

private:
  std::vector<std::unique_ptr<Backend>> backends_;
};

class DnsConfig;
bool loadBackends(BackendRouter &router, const DnsConfig &config);

}  // namespace simple_dnsd
