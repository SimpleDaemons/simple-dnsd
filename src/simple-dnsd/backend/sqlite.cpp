/**
 * @file sqlite.cpp
 * @brief SQLite gsql driver
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/backend/backend.hpp"
#include "simple-dnsd/backend/gsql.hpp"
#include "simple-dnsd/utils/logger.hpp"

#ifdef SIMPLE_DNSD_SQLITE
#include <sqlite3.h>
#endif

namespace simple_dnsd {

#ifdef SIMPLE_DNSD_SQLITE

namespace {

class SqliteSession : public SqlSession {
public:
  explicit SqliteSession(sqlite3 *db) : db_(db) {}
  ~SqliteSession() override {
    if (db_ != nullptr) {
      sqlite3_close(db_);
    }
  }
  bool exec(const std::string &sql) override {
    char *err = nullptr;
    const int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err);
    if (err != nullptr) {
      Logger::instance().warning(std::string("sqlite: ") + err);
      sqlite3_free(err);
    }
    return rc == SQLITE_OK;
  }
  bool query(const std::string &sql, const std::vector<std::string> &,
             std::vector<SqlRow> &out) override {
    out.clear();
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
      return false;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      SqlRow row;
      const int n = sqlite3_column_count(stmt);
      for (int i = 0; i < n; ++i) {
        const unsigned char *text = sqlite3_column_text(stmt, i);
        row.cols.emplace_back(text != nullptr ? reinterpret_cast<const char *>(text) : "");
      }
      out.push_back(std::move(row));
    }
    sqlite3_finalize(stmt);
    return true;
  }
  bool begin() override { return exec("BEGIN IMMEDIATE"); }
  bool commit() override { return exec("COMMIT"); }
  void rollback() override { exec("ROLLBACK"); }
  std::int64_t lastInsertId() override { return sqlite3_last_insert_rowid(db_); }

private:
  sqlite3 *db_;
};

class SqliteFactory : public SqlFactory {
public:
  explicit SqliteFactory(std::string path) : path_(std::move(path)) {}
  std::unique_ptr<SqlSession> connect() override {
    sqlite3 *db = nullptr;
    if (sqlite3_open(path_.c_str(), &db) != SQLITE_OK) {
      if (db != nullptr) {
        sqlite3_close(db);
      }
      return nullptr;
    }
    sqlite3_exec(db, "PRAGMA foreign_keys=ON; PRAGMA journal_mode=WAL;", nullptr, nullptr,
                 nullptr);
    return std::make_unique<SqliteSession>(db);
  }
  std::string dialect() const override { return "sqlite"; }

private:
  std::string path_;
};

}  // namespace

std::unique_ptr<Backend> makeSqliteBackend(const std::string &path, const std::string &schema_sql) {
  auto factory = std::make_unique<SqliteFactory>(path);
  const char *schema = schema_sql.empty() ? kSqliteSchema() : schema_sql.c_str();
  return std::make_unique<GsqlBackend>("sqlite", std::move(factory), schema);
}

#else

std::unique_ptr<Backend> makeSqliteBackend(const std::string &, const std::string &) {
  Logger::instance().error("sqlite backend not compiled in");
  return nullptr;
}

#endif

}  // namespace simple_dnsd
