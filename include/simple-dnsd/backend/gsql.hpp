/**
 * @file gsql.hpp
 * @brief Shared gsql backend
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-dnsd/backend/backend.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace simple_dnsd {

struct SqlRow {
  std::vector<std::string> cols;
};

class SqlSession {
public:
  virtual ~SqlSession() = default;
  virtual bool exec(const std::string &sql) = 0;
  virtual bool query(const std::string &sql, const std::vector<std::string> &params,
                     std::vector<SqlRow> &out) = 0;
  virtual bool begin() = 0;
  virtual bool commit() = 0;
  virtual void rollback() = 0;
  virtual std::int64_t lastInsertId() = 0;
};

class SqlFactory {
public:
  virtual ~SqlFactory() = default;
  virtual std::unique_ptr<SqlSession> connect() = 0;
  virtual std::string dialect() const = 0;
};

class GsqlBackend : public Backend {
public:
  GsqlBackend(std::string name, std::unique_ptr<SqlFactory> factory, std::string schema_sql);
  bool initialize() override;
  std::string name() const override { return name_; }
  bool writable() const override { return true; }

  std::optional<ZoneInfo> findZone(const DnsName &qname) override;
  std::vector<ZoneInfo> listZones() override;
  std::vector<ResourceRecord> lookup(const ZoneInfo &zone, const DnsName &qname,
                                     RrType qtype) override;
  void listZone(const ZoneInfo &zone,
                const std::function<void(const ResourceRecord &)> &cb) override;
  bool nameExists(const ZoneInfo &zone, const DnsName &qname) override;
  std::optional<DnsName> findDelegation(const ZoneInfo &zone, const DnsName &qname) override;
  std::unique_ptr<Transaction> begin(const ZoneInfo &zone) override;
  bool createZone(const ZoneInfo &zone, const ResourceRecord &soa) override;
  bool deleteZone(const ZoneInfo &zone) override;

private:
  friend class GsqlTransaction;
  ZoneInfo rowToZone(const SqlRow &row) const;
  ResourceRecord rowToRr(const SqlRow &row) const;
  std::unique_ptr<SqlSession> db();

  std::string name_;
  std::unique_ptr<SqlFactory> factory_;
  std::string schema_sql_;
  mutable std::mutex mutex_;
};

const char *kSqliteSchema();
const char *kPgsqlSchema();
const char *kMysqlSchema();

}  // namespace simple_dnsd
