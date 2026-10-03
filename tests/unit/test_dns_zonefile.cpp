/**
 * @file test_dns_zonefile.cpp
 * @brief Zone file parser/writer tests
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/zone/zone.hpp"

#include <cassert>
#include <fstream>
#include <iostream>

using namespace simple_dnsd;

int main() {
  const std::string text = R"ZONE(
$ORIGIN example.com.
$TTL 3600
@   IN SOA ns hostmaster 1 10800 3600 604800 3600
    IN NS  ns
ns  IN A   192.0.2.10
www IN A   192.0.2.1
mail IN MX 10 mail
)ZONE";
  auto parsed = parseZoneText(text, DnsName::parse("example.com."));
  assert(parsed.ok);
  assert(!parsed.records.empty());
  bool has_soa = false;
  bool has_www = false;
  for (const auto &rr : parsed.records) {
    if (rr.type == RrType::Soa) {
      has_soa = true;
    }
    if (rr.name.equals(DnsName::parse("www.example.com")) && rr.type == RrType::A) {
      has_www = true;
    }
  }
  assert(has_soa);
  assert(has_www);
  auto check = checkZone(DnsName::parse("example.com."), parsed.records);
  assert(check.ok);

  const std::string written = writeZoneText(DnsName::parse("example.com."), parsed.records);
  auto again = parseZoneText(written, DnsName::parse("example.com."));
  assert(again.ok);
  assert(again.records.size() == parsed.records.size());

  const std::string named = R"CONF(
zone "example.com" {
    type master;
    file "db.example.com";
};
)CONF";
  const std::string npath = "test-named.conf";
  std::ofstream out(npath);
  out << named;
  out.close();
  auto zones = parseNamedConf(npath);
  assert(zones.size() == 1);
  assert(zones[0].info.name.equals(DnsName::parse("example.com")));
  assert(zones[0].file == "db.example.com");

  std::cout << "test_dns_zonefile: ok" << std::endl;
  return 0;
}
