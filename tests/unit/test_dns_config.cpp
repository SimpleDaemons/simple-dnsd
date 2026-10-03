/**
 * @file test_dns_config.cpp
 * @brief Configuration unit tests
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/config/config.hpp"
#include "simple-dnsd/version.hpp"

#include <cassert>
#include <fstream>
#include <iostream>

using namespace simple_dnsd;

static bool testDefaults() {
  DnsConfig c;
  assert(c.dns_port == kDnsDefaultPort);
  assert(c.launch == "memory");
  assert(c.validate());
  return true;
}

static bool testLaunchList() {
  DnsConfig c;
  c.launch = "sqlite, bind";
  auto b = c.launchBackends();
  assert(b.size() == 2);
  assert(b[0] == "sqlite");
  assert(b[1] == "bind");
  return true;
}

static bool testSqliteRequiresFile() {
  DnsConfig c;
  c.launch = "sqlite";
  return !c.validate();
}

static bool testLoadFile() {
  const std::string path = "test-simple-dnsd.conf";
  std::ofstream out(path);
  out << "listen_address = 127.0.0.1\n";
  out << "dns_port = 5353\n";
  out << "launch = memory\n";
  out << "log_level = debug\n";
  out << "enable_api = true\n";
  out << "api_key = secret\n";
  out.close();
  DnsConfig c;
  assert(c.loadFromFile(path));
  assert(c.dns_port == 5353);
  assert(c.enable_api);
  assert(c.api_key == "secret");
  assert(c.validate());
  return true;
}

static bool testApiRequiresKey() {
  DnsConfig c;
  c.enable_api = true;
  return !c.validate();
}

int main() {
  assert(testDefaults());
  assert(testLaunchList());
  assert(testSqliteRequiresFile());
  assert(testLoadFile());
  assert(testApiRequiresKey());
  std::cout << "test_dns_config: ok (" << kVersion << ")" << std::endl;
  return 0;
}
