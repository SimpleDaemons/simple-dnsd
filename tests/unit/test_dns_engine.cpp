/**
 * @file test_dns_engine.cpp
 * @brief Authoritative engine tests
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/backend/backend.hpp"
#include "simple-dnsd/core/engine.hpp"
#include "simple-dnsd/zone/zone.hpp"

#include <cassert>
#include <iostream>

using namespace simple_dnsd;

static ResourceRecord rr(const char *name, RrType type, const char *content) {
  ResourceRecord r;
  r.name = DnsName::parse(name);
  r.type = type;
  r.content = content;
  contentToRdata(r);
  return r;
}

int main() {
  BackendRouter router;
  router.add(makeMemoryBackend());
  assert(router.initialize());
  const DnsName zone = DnsName::parse("example.com");
  ZoneInfo info;
  info.name = zone;
  info.allow_axfr = {"127.0.0.1"};
  info.allow_update = {"127.0.0.1"};
  assert(router.createZone(info, rr("example.com.", RrType::Soa,
                                    "ns.example.com. hostmaster.example.com. 1 10800 3600 604800 3600")));
  auto z = router.findZone(zone);
  auto tx = router.begin(*z);
  tx->addRecord(rr("example.com.", RrType::Ns, "ns.example.com."));
  tx->addRecord(rr("ns.example.com.", RrType::A, "192.0.2.10"));
  tx->addRecord(rr("www.example.com.", RrType::A, "192.0.2.1"));
  tx->addRecord(rr("alias.example.com.", RrType::Cname, "www.example.com."));
  tx->addRecord(rr("*.example.com.", RrType::A, "192.0.2.99"));
  tx->addRecord(rr("deleg.example.com.", RrType::Ns, "ns.other.test."));
  assert(tx->commit());

  DnsConfig cfg;
  cfg.launch = "memory";
  cfg.allow_axfr = {"127.0.0.1"};
  ServerStats stats;
  AuthoritativeEngine engine(router, cfg, stats);

  QueryContext ctx;
  ctx.peer_host = "127.0.0.1";
  ctx.tcp = true;

  auto a = engine.handle(makeQuery(DnsName::parse("www.example.com"), RrType::A, 1), ctx);
  assert(a.header.rcode == Rcode::NoError);
  assert(a.header.aa);
  assert(!a.answers.empty());

  auto nx = engine.handle(makeQuery(DnsName::parse("nope.nope.com"), RrType::A, 2), ctx);
  assert(nx.header.rcode == Rcode::Refused);

  auto nodata = engine.handle(makeQuery(DnsName::parse("www.example.com"), RrType::Aaaa, 3), ctx);
  assert(nodata.header.rcode == Rcode::NoError);
  assert(nodata.answers.empty());

  auto cname = engine.handle(makeQuery(DnsName::parse("alias.example.com"), RrType::A, 4), ctx);
  assert(cname.header.rcode == Rcode::NoError);
  assert(cname.answers.size() >= 2);

  auto wild = engine.handle(makeQuery(DnsName::parse("random.example.com"), RrType::A, 5), ctx);
  assert(wild.header.rcode == Rcode::NoError);
  assert(!wild.answers.empty());

  auto deleg = engine.handle(makeQuery(DnsName::parse("www.deleg.example.com"), RrType::A, 6), ctx);
  assert(deleg.header.rcode == Rcode::NoError);
  assert(!deleg.header.aa);
  assert(!deleg.authority.empty());

  auto any = engine.handle(makeQuery(DnsName::parse("www.example.com"), RrType::Any, 7), ctx);
  assert(any.header.rcode == Rcode::NoError);
  assert(!any.answers.empty());

  auto axfr = engine.handleAxfr(makeQuery(zone, RrType::Axfr, 8), ctx);
  assert(!axfr.empty());
  assert(axfr[0].header.rcode == Rcode::NoError);
  assert(axfr[0].answers.size() >= 3);

  std::cout << "test_dns_engine: ok" << std::endl;
  return 0;
}
