/**
 * @file daemon.cpp
 * @brief Daemon wrapper
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/core/daemon.hpp"
#include "simple-dnsd/utils/logger.hpp"

namespace simple_dnsd {

DnsDaemon::DnsDaemon(DnsConfig config) : config_(std::move(config)) {}

bool DnsDaemon::start() {
  server_ = std::make_unique<DnsServer>(config_);
  return server_->start();
}

void DnsDaemon::stop() {
  if (server_) {
    server_->stop();
  }
}

bool DnsDaemon::running() const { return server_ && server_->running(); }

bool DnsDaemon::testConfig() {
  DnsServer tmp(config_);
  return tmp.testConfig();
}

}  // namespace simple_dnsd
