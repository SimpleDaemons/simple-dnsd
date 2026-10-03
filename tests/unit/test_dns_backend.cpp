/**
 * @file test_dns_backend.cpp
 * @brief Memory and SQLite backend tests
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/backend/backend.hpp"
#include "simple-dnsd/zone/zone.hpp"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>

using namespace simple_dnsd;

static ResourceRecord soa(const DnsName &zone) {
  ResourceRecord rr;
  rr.name = zone;
  rr.type = RrType::Soa;
  rr.content = "ns.example.com. hostmaster.example.com. 1 10800 3600 604800 3600";
  contentToRdata(rr);
  return rr;
}

static void seed(Backend &b) {
  const DnsName zone = DnsName::parse("example.com");
  ZoneInfo z;
  z.name = zone;
  assert(b.createZone(z, soa(zone)));
  auto found = b.findZone(DnsName::parse("www.example.com"));
  assert(found);
  auto tx = b.begin(*found);
  assert(tx);
  ResourceRecord a;
  a.name = DnsName::parse("www.example.com");
  a.type = RrType::A;
  a.content = "192.0.2.1";
  contentToRdata(a);
  tx->addRecord(a);
  ResourceRecord ns;
  ns.name = zone;
  ns.type = RrType::Ns;
  ns.content = "ns.example.com.";
  contentToRdata(ns);
  tx->addRecord(ns);
  assert(tx->commit());
}

static void checkBackend(Backend &b, const char *label) {
  auto zone = b.findZone(DnsName::parse("www.example.com"));
  assert(zone);
  auto a = b.lookup(*zone, DnsName::parse("www.example.com"), RrType::A);
  assert(a.size() == 1);
  auto missing = b.lookup(*zone, DnsName::parse("nope.example.com"), RrType::A);
  assert(missing.empty());
  assert(b.nameExists(*zone, DnsName::parse("www.example.com")));
  std::cout << "  " << label << ": ok" << std::endl;
}

int main() {
  auto mem = makeMemoryBackend();
  assert(mem->initialize());
  seed(*mem);
  checkBackend(*mem, "memory");

#ifdef SIMPLE_DNSD_SQLITE
  std::remove("test-simple-dnsd.sqlite");
  auto sqlite = makeSqliteBackend("test-simple-dnsd.sqlite");
  assert(sqlite);
  assert(sqlite->initialize());
  seed(*sqlite);
  checkBackend(*sqlite, "sqlite");
#else
  std::cout << "  sqlite: skipped (not compiled)" << std::endl;
#endif

  const char *pg = std::getenv("SIMPLE_DNSD_PG_DSN");
  if (pg != nullptr && *pg != '\0') {
    auto backend = makePostgresBackend(pg);
    if (backend && backend->initialize()) {
      seed(*backend);
      checkBackend(*backend, "postgres");
    } else {
      std::cout << "  postgres: skipped (connect failed)" << std::endl;
    }
  } else {
    std::cout << "  postgres: skipped (no SIMPLE_DNSD_PG_DSN)" << std::endl;
  }
  const char *my = std::getenv("SIMPLE_DNSD_MYSQL_DSN");
  if (my != nullptr && *my != '\0') {
    auto backend = makeMysqlBackend(my);
    if (backend && backend->initialize()) {
      seed(*backend);
      checkBackend(*backend, "mysql");
    } else {
      std::cout << "  mysql: skipped (connect failed)" << std::endl;
    }
  } else {
    std::cout << "  mysql: skipped (no SIMPLE_DNSD_MYSQL_DSN)" << std::endl;
  }

  std::cout << "test_dns_backend: ok" << std::endl;
  return 0;
}
