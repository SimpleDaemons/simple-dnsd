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



bool handleHttpRequest(DnsServer &, TcpConnection &conn) {
  const char *resp =
      "HTTP/1.1 501 Not Implemented\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";
  const auto *begin = reinterpret_cast<const uint8_t *>(resp);
  conn.sendAll(std::vector<uint8_t>(begin, begin + std::strlen(resp)));
  return false;
}

}  // namespace simple_dnsd
