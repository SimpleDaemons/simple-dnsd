/**
 * @file test_dns_server.cpp
 * @brief Live UDP/TCP authoritative answers (0.4.0)
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/core/server.hpp"
#include "simple-dnsd/utils/net.hpp"
#include "simple-dnsd/zone/zone.hpp"

#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

using namespace simple_dnsd;

static ResourceRecord rr(const char *name, RrType type, const char *content) {
  ResourceRecord r;
  r.name = DnsName::parse(name);
  r.type = type;
  r.content = content;
  contentToRdata(r);
  return r;
}

static DnsMessage queryUdp(port_t port, const DnsName &name, RrType type) {
  auto q = makeQuery(name, type, 11);
  auto wire = encodeMessage(q);
  std::vector<uint8_t> reply;
  assert(UdpSocket::sendOnce("127.0.0.1", port, wire, reply, 2000));
  DnsMessage parsed;
  assert(decodeMessage(reply, parsed));
  return parsed;
}

int main() {
  initializeSockets();
  DnsConfig cfg;
  cfg.listen_address = "127.0.0.1";
  cfg.dns_port = 0;
  cfg.launch = "memory";
  cfg.worker_threads = 1;
  cfg.idle_timeout = 1;
  cfg.foreground = true;
  cfg.log_level = "error";
  cfg.enable_api = true;
  cfg.api_listen = "127.0.0.1";
  cfg.api_port = 0;
  cfg.api_key = "testkey";
  cfg.allow_axfr = {"127.0.0.1"};

  DnsServer server(cfg);
  assert(server.start());
  const port_t dns_port = server.dnsPort();
  const port_t api_port = server.apiPort();
  assert(dns_port != 0);
  assert(api_port != 0);

  ZoneInfo z;
  z.name = DnsName::parse("example.com");
  z.allow_axfr = {"127.0.0.1"};
  assert(server.router().createZone(
      z, rr("example.com.", RrType::Soa,
            "ns.example.com. hostmaster.example.com. 1 10800 3600 604800 3600")));
  auto zone = server.router().findZone(z.name);
  auto tx = server.router().begin(*zone);
  tx->addRecord(rr("example.com.", RrType::Ns, "ns.example.com."));
  tx->addRecord(rr("www.example.com.", RrType::A, "192.0.2.1"));
  assert(tx->commit());

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  auto a = queryUdp(dns_port, DnsName::parse("www.example.com"), RrType::A);
  assert(a.header.rcode == Rcode::NoError);
  assert(!a.answers.empty());

  // Second identical query is served from the packet cache.
  const auto misses_before = server.stats().cache_misses.load();
  const auto hits_before = server.stats().cache_hits.load();
  auto a2 = queryUdp(dns_port, DnsName::parse("www.example.com"), RrType::A);
  assert(a2.header.rcode == Rcode::NoError);
  assert(server.stats().cache_hits.load() > hits_before);
  assert(server.stats().cache_misses.load() == misses_before);
  assert(server.stats().queries.load() >= 2);

  auto nx = queryUdp(dns_port, DnsName::parse("missing.example.com"), RrType::A);
  assert(nx.header.rcode == Rcode::NxDomain);

  auto tcp = TcpConnection::connectTo("127.0.0.1", dns_port);
  if (!tcp) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    tcp = TcpConnection::connectTo("127.0.0.1", dns_port);
  }
  assert(tcp);
  auto tq = makeQuery(DnsName::parse("www.example.com"), RrType::A, 12);
  assert(tcp->sendDnsMessage(encodeMessage(tq)));
  std::vector<uint8_t> twire;
  assert(tcp->recvDnsMessage(twire));
  DnsMessage tmsg;
  assert(decodeMessage(twire, tmsg));
  assert(tmsg.header.rcode == Rcode::NoError);
  tcp->close();

  auto xfr = TcpConnection::connectTo("127.0.0.1", dns_port);
  assert(xfr);
  auto axfrq = makeQuery(z.name, RrType::Axfr, 13);
  assert(xfr->sendDnsMessage(encodeMessage(axfrq)));
  std::vector<uint8_t> axfr_wire;
  assert(xfr->recvDnsMessage(axfr_wire));
  DnsMessage axfr;
  assert(decodeMessage(axfr_wire, axfr));
  assert(axfr.header.rcode == Rcode::NoError);
  assert(axfr.answers.size() >= 3);
  xfr->close();

  auto http = TcpConnection::connectTo("127.0.0.1", api_port);
  assert(http);
  std::string req =
      "GET /api/v1/servers/localhost/zones HTTP/1.1\r\nHost: localhost\r\n"
      "X-API-Key: testkey\r\nConnection: close\r\n\r\n";
  assert(http->sendAll(std::vector<uint8_t>(req.begin(), req.end())));
  std::string body;
  uint8_t buf[2048];
  while (http->waitReadable(1000)) {
#ifdef SIMPLE_DNSD_WINDOWS
    const int n = ::recv(http->native(), reinterpret_cast<char *>(buf), sizeof(buf), 0);
#else
    const ssize_t n = ::recv(http->native(), buf, sizeof(buf), 0);
#endif
    if (n <= 0) {
      break;
    }
    body.append(reinterpret_cast<char *>(buf), static_cast<std::size_t>(n));
  }
  assert(body.find("example.com") != std::string::npos);
  http->close();

  server.stop();
  shutdownSockets();
  std::cout << "test_dns_server: ok" << std::endl;
  return 0;
}
