/**
 * @file stubs.cpp
 * @brief Placeholders for backends and REST until later minors
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/backend/backend.hpp"
#include "simple-dnsd/core/server.hpp"
#include "simple-dnsd/utils/logger.hpp"

#include <cstring>
#include <string>
#include <vector>

namespace simple_dnsd {

std::unique_ptr<Backend> makeBindFileBackend(const std::string &) {
  Logger::instance().error("bind backend arrives in 0.5.0");
  return nullptr;
}

std::unique_ptr<Backend> makeSqliteBackend(const std::string &, const std::string &) {
  Logger::instance().error("sqlite backend arrives in 0.6.0");
  return nullptr;
}

std::unique_ptr<Backend> makePostgresBackend(const std::string &) {
  Logger::instance().error("postgres backend arrives in 0.7.0");
  return nullptr;
}

std::unique_ptr<Backend> makeMysqlBackend(const std::string &) {
  Logger::instance().error("mysql backend arrives in 0.7.0");
  return nullptr;
}

bool handleHttpRequest(DnsServer &, TcpConnection &conn) {
  const char *resp =
      "HTTP/1.1 501 Not Implemented\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";
  const auto *begin = reinterpret_cast<const uint8_t *>(resp);
  conn.sendAll(std::vector<uint8_t>(begin, begin + std::strlen(resp)));
  return false;
}

}  // namespace simple_dnsd
