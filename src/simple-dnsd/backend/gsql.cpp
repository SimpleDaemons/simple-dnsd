/**
 * @file gsql.cpp
 * @brief gsql backend
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/backend/gsql.hpp"
#include "simple-dnsd/utils/logger.hpp"
#include "simple-dnsd/utils/platform.hpp"
#include "simple-dnsd/zone/zone.hpp"

namespace simple_dnsd {

namespace {

const char kSchemaSqlite[] = R"SQL(
CREATE TABLE IF NOT EXISTS domains (
  id INTEGER PRIMARY KEY,
  name VARCHAR(255) NOT NULL,
  master VARCHAR(128) DEFAULT NULL,
  last_check INTEGER DEFAULT NULL,
  type VARCHAR(8) NOT NULL,
  notified_serial INTEGER DEFAULT NULL,
  account VARCHAR(40) DEFAULT NULL,
  options TEXT DEFAULT NULL,
  catalog VARCHAR(255) DEFAULT NULL
);
CREATE UNIQUE INDEX IF NOT EXISTS name_index ON domains(name);
CREATE TABLE IF NOT EXISTS records (
  id INTEGER PRIMARY KEY,
  domain_id INTEGER DEFAULT NULL,
  name VARCHAR(255) DEFAULT NULL,
  type VARCHAR(10) DEFAULT NULL,
  content VARCHAR(65535) DEFAULT NULL,
  ttl INTEGER DEFAULT NULL,
  prio INTEGER DEFAULT NULL,
  disabled BOOLEAN DEFAULT 0,
  ordername VARCHAR(255),
  auth INTEGER DEFAULT 1,
  FOREIGN KEY(domain_id) REFERENCES domains(id) ON DELETE CASCADE
);
CREATE INDEX IF NOT EXISTS rec_name_index ON records(name);
CREATE INDEX IF NOT EXISTS nametype_index ON records(name,type);
CREATE INDEX IF NOT EXISTS domain_id ON records(domain_id);
CREATE TABLE IF NOT EXISTS comments (
  id INTEGER PRIMARY KEY,
  domain_id INTEGER NOT NULL,
  name VARCHAR(255) NOT NULL,
  type VARCHAR(10) NOT NULL,
  modified_at INT NOT NULL,
  account VARCHAR(40) DEFAULT NULL,
  comment VARCHAR(65535) NOT NULL
);
CREATE TABLE IF NOT EXISTS domainmetadata (
  id INTEGER PRIMARY KEY,
  domain_id INTEGER NOT NULL,
  kind VARCHAR(32),
  content TEXT
);
CREATE TABLE IF NOT EXISTS cryptokeys (
  id INTEGER PRIMARY KEY,
  domain_id INTEGER NOT NULL,
  flags INT NOT NULL,
  active BOOL,
  published BOOL DEFAULT 1,
  content TEXT
);
CREATE TABLE IF NOT EXISTS tsigkeys (
  id INTEGER PRIMARY KEY,
  name VARCHAR(255),
  algorithm VARCHAR(50),
  secret VARCHAR(255)
);
)SQL";

const char kSchemaPgsql[] = R"SQL(
CREATE TABLE IF NOT EXISTS domains (
  id SERIAL PRIMARY KEY,
  name VARCHAR(255) NOT NULL,
  master VARCHAR(128) DEFAULT NULL,
  last_check INT DEFAULT NULL,
  type VARCHAR(8) NOT NULL,
  notified_serial INT DEFAULT NULL,
  account VARCHAR(40) DEFAULT NULL
);
CREATE TABLE IF NOT EXISTS records (
  id SERIAL PRIMARY KEY,
  domain_id INT DEFAULT NULL,
  name VARCHAR(255) DEFAULT NULL,
  type VARCHAR(10) DEFAULT NULL,
  content VARCHAR(65535) DEFAULT NULL,
  ttl INT DEFAULT NULL,
  prio INT DEFAULT NULL,
  disabled BOOL DEFAULT 'f',
  ordername VARCHAR(255),
  auth BOOL DEFAULT 't'
);
CREATE TABLE IF NOT EXISTS comments (
  id SERIAL PRIMARY KEY,
  domain_id INT NOT NULL,
  name VARCHAR(255) NOT NULL,
  type VARCHAR(10) NOT NULL,
  modified_at INT NOT NULL,
  account VARCHAR(40) DEFAULT NULL,
  comment VARCHAR(65535) NOT NULL
);
CREATE TABLE IF NOT EXISTS domainmetadata (
  id SERIAL PRIMARY KEY,
  domain_id INT NOT NULL,
  kind VARCHAR(32),
  content TEXT
);
CREATE TABLE IF NOT EXISTS cryptokeys (
  id SERIAL PRIMARY KEY,
  domain_id INT NOT NULL,
  flags INT NOT NULL,
  active BOOL,
  published BOOL DEFAULT 't',
  content TEXT
);
CREATE TABLE IF NOT EXISTS tsigkeys (
  id SERIAL PRIMARY KEY,
  name VARCHAR(255),
  algorithm VARCHAR(50),
  secret VARCHAR(255)
);
)SQL";

const char kSchemaMysql[] = R"SQL(
CREATE TABLE IF NOT EXISTS domains (
  id INT AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(255) NOT NULL,
  master VARCHAR(128) DEFAULT NULL,
  last_check INT DEFAULT NULL,
  type VARCHAR(8) NOT NULL,
  notified_serial INT DEFAULT NULL,
  account VARCHAR(40) DEFAULT NULL
);
CREATE TABLE IF NOT EXISTS records (
  id INT AUTO_INCREMENT PRIMARY KEY,
  domain_id INT DEFAULT NULL,
  name VARCHAR(255) DEFAULT NULL,
  type VARCHAR(10) DEFAULT NULL,
  content TEXT DEFAULT NULL,
  ttl INT DEFAULT NULL,
  prio INT DEFAULT NULL,
  disabled TINYINT(1) DEFAULT 0,
  ordername VARCHAR(255),
  auth TINYINT(1) DEFAULT 1
);
CREATE TABLE IF NOT EXISTS comments (
  id INT AUTO_INCREMENT PRIMARY KEY,
  domain_id INT NOT NULL,
  name VARCHAR(255) NOT NULL,
  type VARCHAR(10) NOT NULL,
  modified_at INT NOT NULL,
  account VARCHAR(40) DEFAULT NULL,
  comment TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS domainmetadata (
  id INT AUTO_INCREMENT PRIMARY KEY,
  domain_id INT NOT NULL,
  kind VARCHAR(32),
  content TEXT
);
CREATE TABLE IF NOT EXISTS cryptokeys (
  id INT AUTO_INCREMENT PRIMARY KEY,
  domain_id INT NOT NULL,
  flags INT NOT NULL,
  active BOOL,
  published BOOL DEFAULT 1,
  content TEXT
);
CREATE TABLE IF NOT EXISTS tsigkeys (
  id INT AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(255),
  algorithm VARCHAR(50),
  secret VARCHAR(255)
);
)SQL";

std::string q(const std::string &s) {
  std::string out = "'";
  for (char ch : s) {
    if (ch == '\'') {
      out += "''";
    } else {
      out += ch;
    }
  }
  out += "'";
  return out;
}

}  // namespace

const char *kSqliteSchema() { return kSchemaSqlite; }
const char *kPgsqlSchema() { return kSchemaPgsql; }
const char *kMysqlSchema() { return kSchemaMysql; }

class GsqlTransaction : public Transaction {
public:
  GsqlTransaction(GsqlBackend *backend, ZoneInfo zone, std::unique_ptr<SqlSession> session)
      : backend_(backend), zone_(std::move(zone)), session_(std::move(session)) {
    session_->begin();
  }
  ~GsqlTransaction() override {
    if (session_ && !done_) {
      session_->rollback();
    }
  }
  bool addRecord(const ResourceRecord &rr) override {
    ResourceRecord copy = rr;
    if (copy.content.empty()) {
      rdataToContent(copy);
    }
    const std::string sql =
        "INSERT INTO records(domain_id,name,type,content,ttl,prio,disabled,auth) VALUES(" +
        std::to_string(zone_.id) + "," + q(copy.name.toLowerString(false)) + "," +
        q(rrTypeToString(copy.type)) + "," + q(copy.content) + "," + std::to_string(copy.ttl) +
        "," + std::to_string(copy.prio) + ",0,1)";
    return session_->exec(sql);
  }
  bool replaceRrset(const DnsName &name, RrType type,
                    const std::vector<ResourceRecord> &rrs) override {
    if (!deleteRrset(name, type)) {
      return false;
    }
    for (const auto &rr : rrs) {
      if (!addRecord(rr)) {
        return false;
      }
    }
    return true;
  }
  bool deleteRrset(const DnsName &name, RrType type) override {
    const std::string sql = "DELETE FROM records WHERE domain_id=" + std::to_string(zone_.id) +
                            " AND name=" + q(name.toLowerString(false)) +
                            " AND type=" + q(rrTypeToString(type));
    return session_->exec(sql);
  }
  bool deleteName(const DnsName &name) override {
    const std::string sql = "DELETE FROM records WHERE domain_id=" + std::to_string(zone_.id) +
                            " AND name=" + q(name.toLowerString(false));
    return session_->exec(sql);
  }
  bool createZone(const ZoneInfo &, const ResourceRecord &) override { return false; }
  bool deleteZone() override {
    session_->exec("DELETE FROM records WHERE domain_id=" + std::to_string(zone_.id));
    return session_->exec("DELETE FROM domains WHERE id=" + std::to_string(zone_.id));
  }
  bool setSoaSerial(uint32_t serial) override {
    std::vector<SqlRow> rows;
    session_->query("SELECT id,content FROM records WHERE domain_id=" + std::to_string(zone_.id) +
                        " AND type='SOA'",
                    {}, rows);
    for (const auto &row : rows) {
      if (row.cols.size() < 2) {
        continue;
      }
      ResourceRecord soa;
      soa.type = RrType::Soa;
      soa.content = row.cols[1];
      simple_dnsd::setSoaSerial(soa, serial);
      session_->exec("UPDATE records SET content=" + q(soa.content) + " WHERE id=" + row.cols[0]);
    }
    return true;
  }
  bool commit() override {
    done_ = session_->commit();
    return done_;
  }
  void rollback() override {
    session_->rollback();
    done_ = true;
  }

private:
  GsqlBackend *backend_;
  ZoneInfo zone_;
  std::unique_ptr<SqlSession> session_;
  bool done_{false};
};

GsqlBackend::GsqlBackend(std::string name, std::unique_ptr<SqlFactory> factory,
                         std::string schema_sql)
    : name_(std::move(name)), factory_(std::move(factory)), schema_sql_(std::move(schema_sql)) {}

std::unique_ptr<SqlSession> GsqlBackend::db() { return factory_->connect(); }

bool GsqlBackend::initialize() {
  auto session = db();
  if (!session) {
    return false;
  }
  if (!schema_sql_.empty()) {
    std::string stmt;
    for (char ch : schema_sql_) {
      stmt.push_back(ch);
      if (ch == ';') {
        const std::string trimmed = trim(stmt);
        if (trimmed.size() > 1) {
          session->exec(trimmed);
        }
        stmt.clear();
      }
    }
  }
  return true;
}

ZoneInfo GsqlBackend::rowToZone(const SqlRow &row) const {
  ZoneInfo z;
  if (row.cols.size() >= 1) {
    z.id = std::stoll(row.cols[0].empty() ? "0" : row.cols[0]);
  }
  if (row.cols.size() >= 2) {
    z.name = DnsName::parse(row.cols[1]);
  }
  if (row.cols.size() >= 3) {
    z.master = row.cols[2];
  }
  if (row.cols.size() >= 5) {
    const std::string t = toLower(row.cols[4]);
    if (t == "slave" || t == "secondary") {
      z.kind = ZoneKind::Slave;
    } else if (t == "master") {
      z.kind = ZoneKind::Master;
    } else {
      z.kind = ZoneKind::Native;
    }
  }
  if (row.cols.size() >= 6 && !row.cols[5].empty()) {
    z.notified_serial = static_cast<uint32_t>(std::stoul(row.cols[5]));
  }
  if (row.cols.size() >= 7) {
    z.account = row.cols[6];
  }
  return z;
}

ResourceRecord GsqlBackend::rowToRr(const SqlRow &row) const {
  ResourceRecord rr;
  // content,ttl,prio,type,domain_id,disabled,name,auth
  if (row.cols.size() >= 7) {
    rr.content = row.cols[0];
    if (!row.cols[1].empty()) {
      rr.ttl = static_cast<uint32_t>(std::stoul(row.cols[1]));
    }
    if (!row.cols[2].empty()) {
      rr.prio = static_cast<uint16_t>(std::stoi(row.cols[2]));
    }
    auto type = rrTypeFromString(row.cols[3]);
    if (type) {
      rr.type = *type;
    }
    rr.disabled = row.cols[5] == "1" || toLower(row.cols[5]) == "t";
    rr.name = DnsName::parse(row.cols[6]);
    if (row.cols.size() >= 8) {
      rr.auth = row.cols[7] != "0" && toLower(row.cols[7]) != "f";
    }
    contentToRdata(rr);
  }
  return rr;
}

std::optional<ZoneInfo> GsqlBackend::findZone(const DnsName &qname) {
  auto session = db();
  if (!session) {
    return std::nullopt;
  }
  DnsName cur = qname;
  while (true) {
    std::vector<SqlRow> rows;
    session->query("SELECT id,name,master,last_check,type,notified_serial,account FROM domains WHERE name=" +
                       q(cur.toLowerString(false)),
                   {}, rows);
    if (!rows.empty()) {
      return rowToZone(rows[0]);
    }
    if (cur.empty()) {
      break;
    }
    cur = cur.parent();
  }
  return std::nullopt;
}

std::vector<ZoneInfo> GsqlBackend::listZones() {
  std::vector<ZoneInfo> out;
  auto session = db();
  if (!session) {
    return out;
  }
  std::vector<SqlRow> rows;
  session->query("SELECT id,name,master,last_check,type,notified_serial,account FROM domains", {},
                 rows);
  for (const auto &row : rows) {
    out.push_back(rowToZone(row));
  }
  return out;
}

std::vector<ResourceRecord> GsqlBackend::lookup(const ZoneInfo &zone, const DnsName &qname,
                                                RrType qtype) {
  std::vector<ResourceRecord> out;
  auto session = db();
  if (!session) {
    return out;
  }
  std::string sql =
      "SELECT content,ttl,prio,type,domain_id,disabled,name,auth FROM records WHERE domain_id=" +
      std::to_string(zone.id) + " AND name=" + q(qname.toLowerString(false)) + " AND disabled=0";
  if (qtype != RrType::Any) {
    sql += " AND type=" + q(rrTypeToString(qtype));
  }
  std::vector<SqlRow> rows;
  session->query(sql, {}, rows);
  for (const auto &row : rows) {
    out.push_back(rowToRr(row));
  }
  return out;
}

void GsqlBackend::listZone(const ZoneInfo &zone,
                           const std::function<void(const ResourceRecord &)> &cb) {
  auto session = db();
  if (!session) {
    return;
  }
  std::vector<SqlRow> rows;
  session->query(
      "SELECT content,ttl,prio,type,domain_id,disabled,name,auth FROM records WHERE domain_id=" +
          std::to_string(zone.id),
      {}, rows);
  for (const auto &row : rows) {
    cb(rowToRr(row));
  }
}

bool GsqlBackend::nameExists(const ZoneInfo &zone, const DnsName &qname) {
  auto session = db();
  if (!session) {
    return false;
  }
  std::vector<SqlRow> rows;
  session->query("SELECT id FROM records WHERE domain_id=" + std::to_string(zone.id) +
                     " AND (name=" + q(qname.toLowerString(false)) + " OR name LIKE " +
                     q("%." + qname.toLowerString(false)) + ") LIMIT 1",
                 {}, rows);
  return !rows.empty();
}

std::optional<DnsName> GsqlBackend::findDelegation(const ZoneInfo &zone, const DnsName &qname) {
  DnsName cur = qname;
  while (!cur.equals(zone.name) && !cur.empty()) {
    auto ns = lookup(zone, cur, RrType::Ns);
    if (!ns.empty() && !cur.equals(zone.name)) {
      return cur;
    }
    cur = cur.parent();
  }
  return std::nullopt;
}

std::unique_ptr<Transaction> GsqlBackend::begin(const ZoneInfo &zone) {
  auto session = db();
  if (!session) {
    return nullptr;
  }
  return std::make_unique<GsqlTransaction>(this, zone, std::move(session));
}

bool GsqlBackend::createZone(const ZoneInfo &zone, const ResourceRecord &soa) {
  auto session = db();
  if (!session) {
    return false;
  }
  std::string type = "NATIVE";
  if (zone.kind == ZoneKind::Master) {
    type = "MASTER";
  } else if (zone.kind == ZoneKind::Slave) {
    type = "SLAVE";
  }
  if (!session->exec("INSERT INTO domains(name,type,master,account) VALUES(" +
                     q(zone.name.toLowerString(false)) + "," + q(type) + "," + q(zone.master) +
                     "," + q(zone.account) + ")")) {
    return false;
  }
  const auto id = session->lastInsertId();
  ResourceRecord s = soa;
  if (s.content.empty()) {
    rdataToContent(s);
  }
  return session->exec(
      "INSERT INTO records(domain_id,name,type,content,ttl,prio,disabled,auth) VALUES(" +
      std::to_string(id) + "," + q(zone.name.toLowerString(false)) + ",'SOA'," + q(s.content) +
      "," + std::to_string(s.ttl) + ",0,0,1)");
}

bool GsqlBackend::deleteZone(const ZoneInfo &zone) {
  auto session = db();
  if (!session) {
    return false;
  }
  session->exec("DELETE FROM records WHERE domain_id=" + std::to_string(zone.id));
  return session->exec("DELETE FROM domains WHERE id=" + std::to_string(zone.id));
}

}  // namespace simple_dnsd
