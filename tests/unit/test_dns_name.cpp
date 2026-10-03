/**
 * @file test_dns_name.cpp
 * @brief DnsName unit tests
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/protocol/dns.hpp"

#include <cassert>
#include <iostream>
#include <map>

using namespace simple_dnsd;

int main() {
  auto a = DnsName::parse("www.Example.COM.");
  auto b = DnsName::parse("www.example.com");
  assert(a.equals(b));
  assert(a.toLowerString(false) == "www.example.com");
  assert(a.parent().equals(DnsName::parse("example.com")));
  assert(a.isSubdomainOf(DnsName::parse("example.com")));
  assert(DnsName::parse(".").isRoot());
  auto wild = DnsName::wildcard(DnsName::parse("example.com"));
  assert(wild.isWildcard());
  assert(wild.toLowerString(false) == "*.example.com");

  std::vector<uint8_t> wire;
  std::map<std::string, uint16_t> compression;
  assert(encodeName(wire, a, &compression, true));
  assert(encodeName(wire, DnsName::parse("mail.example.com"), &compression, true));
  std::size_t off = 0;
  DnsName decoded;
  assert(decodeName(wire, off, decoded));
  assert(decoded.equals(a));
  DnsName decoded2;
  assert(decodeName(wire, off, decoded2));
  assert(decoded2.equals(DnsName::parse("mail.example.com")));

  std::cout << "test_dns_name: ok" << std::endl;
  return 0;
}
