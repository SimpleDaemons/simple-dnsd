/**
 * @file daemon.hpp
 * @brief Daemon wrapper
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-dnsd/config/config.hpp"
#include "simple-dnsd/core/server.hpp"
#include <memory>

namespace simple_dnsd {

class DnsDaemon {
public:
  explicit DnsDaemon(DnsConfig config);
  bool start();
  void stop();
  bool running() const;
  bool testConfig();
  DnsServer *server() { return server_.get(); }

private:
  DnsConfig config_;
  std::unique_ptr<DnsServer> server_;
};

int runDnsutil(int argc, char *argv[]);

}  // namespace simple_dnsd
