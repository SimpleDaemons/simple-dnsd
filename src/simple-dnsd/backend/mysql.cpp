/**
 * @file mysql.cpp
 * @brief MySQL/MariaDB gsql driver
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/backend/backend.hpp"
#include "simple-dnsd/backend/gsql.hpp"
#include "simple-dnsd/utils/logger.hpp"
#include "simple-dnsd/utils/platform.hpp"

#ifdef SIMPLE_DNSD_MYSQL
#ifdef __has_include
#if __has_include(<mysql.h>)
#include <mysql.h>
#elif __has_include(<mysql/mysql.h>)
#include <mysql/mysql.h>
#elif __has_include(<mariadb/mysql.h>)
#include <mariadb/mysql.h>
#endif
#else
#include <mysql.h>
#endif
#endif

namespace simple_dnsd {

#ifdef SIMPLE_DNSD_MYSQL

namespace {

class MysqlSession : public SqlSession {
public:
  explicit MysqlSession(MYSQL *conn) : conn_(conn) {}
  ~MysqlSession() override {
    if (conn_ != nullptr) {
      mysql_close(conn_);
    }
  }
  bool exec(const std::string &sql) override {
    if (mysql_query(conn_, sql.c_str()) != 0) {
      Logger::instance().warning(std::string("mysql: ") + mysql_error(conn_));
      return false;
    }
    MYSQL_RES *res = mysql_store_result(conn_);
    if (res != nullptr) {
      mysql_free_result(res);
    }
    return true;
  }
  bool query(const std::string &sql, const std::vector<std::string> &,
             std::vector<SqlRow> &out) override {
    out.clear();
    if (mysql_query(conn_, sql.c_str()) != 0) {
      Logger::instance().warning(std::string("mysql: ") + mysql_error(conn_));
      return false;
    }
    MYSQL_RES *res = mysql_store_result(conn_);
    if (res == nullptr) {
      return true;
    }
    MYSQL_ROW row;
    const unsigned int cols = mysql_num_fields(res);
    while ((row = mysql_fetch_row(res)) != nullptr) {
      SqlRow r;
      for (unsigned int i = 0; i < cols; ++i) {
        r.cols.emplace_back(row[i] != nullptr ? row[i] : "");
      }
      out.push_back(std::move(r));
    }
    mysql_free_result(res);
    return true;
  }
  bool begin() override { return exec("START TRANSACTION"); }
  bool commit() override { return exec("COMMIT"); }
  void rollback() override { exec("ROLLBACK"); }
  std::int64_t lastInsertId() override {
    return static_cast<std::int64_t>(mysql_insert_id(conn_));
  }

private:
  MYSQL *conn_;
};

class MysqlFactory : public SqlFactory {
public:
  explicit MysqlFactory(std::string dsn) : dsn_(std::move(dsn)) {}
  std::unique_ptr<SqlSession> connect() override {
    MYSQL *conn = mysql_init(nullptr);
    if (conn == nullptr) {
      return nullptr;
    }
    std::string host = "127.0.0.1";
    std::string user = "root";
    std::string pass;
    std::string db = "simplednsd";
    unsigned int port = 3306;
    for (const auto &part : split(dsn_, ';')) {
      auto eq = part.find('=');
      if (eq == std::string::npos) {
        continue;
      }
      const std::string k = toLower(trim(part.substr(0, eq)));
      const std::string v = trim(part.substr(eq + 1));
      if (k == "host") {
        host = v;
      } else if (k == "user") {
        user = v;
      } else if (k == "password" || k == "pass") {
        pass = v;
      } else if (k == "database" || k == "dbname" || k == "db") {
        db = v;
      } else if (k == "port") {
        port = static_cast<unsigned int>(std::stoul(v));
      }
    }
    if (mysql_real_connect(conn, host.c_str(), user.c_str(), pass.c_str(), db.c_str(), port,
                           nullptr, 0) == nullptr) {
      Logger::instance().error(std::string("mysql connect: ") + mysql_error(conn));
      mysql_close(conn);
      return nullptr;
    }
    return std::make_unique<MysqlSession>(conn);
  }
  std::string dialect() const override { return "mysql"; }

private:
  std::string dsn_;
};

}  // namespace

std::unique_ptr<Backend> makeMysqlBackend(const std::string &dsn) {
  auto factory = std::make_unique<MysqlFactory>(dsn);
  return std::make_unique<GsqlBackend>("mysql", std::move(factory), kMysqlSchema());
}

#else

std::unique_ptr<Backend> makeMysqlBackend(const std::string &) {
  Logger::instance().error("mysql backend not compiled in");
  return nullptr;
}

#endif

}  // namespace simple_dnsd
