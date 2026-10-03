/**
 * @file postgres.cpp
 * @brief PostgreSQL gsql driver
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/backend/backend.hpp"
#include "simple-dnsd/backend/gsql.hpp"
#include "simple-dnsd/utils/logger.hpp"

#ifdef SIMPLE_DNSD_POSTGRES
#include <libpq-fe.h>
#endif

namespace simple_dnsd {

#ifdef SIMPLE_DNSD_POSTGRES

namespace {

class PgSession : public SqlSession {
public:
  explicit PgSession(PGconn *conn) : conn_(conn) {}
  ~PgSession() override {
    if (conn_ != nullptr) {
      PQfinish(conn_);
    }
  }
  bool exec(const std::string &sql) override {
    PGresult *res = PQexec(conn_, sql.c_str());
    const ExecStatusType st = PQresultStatus(res);
    if (st != PGRES_COMMAND_OK && st != PGRES_TUPLES_OK) {
      Logger::instance().warning(std::string("postgres: ") + PQerrorMessage(conn_));
      PQclear(res);
      return false;
    }
    PQclear(res);
    return true;
  }
  bool query(const std::string &sql, const std::vector<std::string> &,
             std::vector<SqlRow> &out) override {
    out.clear();
    PGresult *res = PQexec(conn_, sql.c_str());
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
      Logger::instance().warning(std::string("postgres: ") + PQerrorMessage(conn_));
      PQclear(res);
      return false;
    }
    const int rows = PQntuples(res);
    const int cols = PQnfields(res);
    for (int r = 0; r < rows; ++r) {
      SqlRow row;
      for (int c = 0; c < cols; ++c) {
        row.cols.emplace_back(PQgetvalue(res, r, c));
      }
      out.push_back(std::move(row));
    }
    PQclear(res);
    return true;
  }
  bool begin() override { return exec("BEGIN"); }
  bool commit() override { return exec("COMMIT"); }
  void rollback() override { exec("ROLLBACK"); }
  std::int64_t lastInsertId() override {
    std::vector<SqlRow> rows;
    query("SELECT lastval()", {}, rows);
    if (rows.empty() || rows[0].cols.empty()) {
      return 0;
    }
    return std::stoll(rows[0].cols[0]);
  }

private:
  PGconn *conn_;
};

class PgFactory : public SqlFactory {
public:
  explicit PgFactory(std::string dsn) : dsn_(std::move(dsn)) {}
  std::unique_ptr<SqlSession> connect() override {
    PGconn *conn = PQconnectdb(dsn_.c_str());
    if (PQstatus(conn) != CONNECTION_OK) {
      Logger::instance().error(std::string("postgres connect: ") + PQerrorMessage(conn));
      PQfinish(conn);
      return nullptr;
    }
    return std::make_unique<PgSession>(conn);
  }
  std::string dialect() const override { return "pgsql"; }

private:
  std::string dsn_;
};

}  // namespace

std::unique_ptr<Backend> makePostgresBackend(const std::string &dsn) {
  auto factory = std::make_unique<PgFactory>(dsn);
  return std::make_unique<GsqlBackend>("postgres", std::move(factory), kPgsqlSchema());
}

#else

std::unique_ptr<Backend> makePostgresBackend(const std::string &) {
  Logger::instance().error("postgres backend not compiled in");
  return nullptr;
}

#endif

}  // namespace simple_dnsd
