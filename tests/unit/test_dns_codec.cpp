/**
 * @file test_dns_codec.cpp
 * @brief Message codec and rdata tests
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/protocol/dns.hpp"

#include <cassert>
#include <iostream>

using namespace simple_dnsd;

static ResourceRecord makeRr(const std::string &name, RrType type, const std::string &content,
                             uint32_t ttl = 300) {
  ResourceRecord rr;
  rr.name = DnsName::parse(name);
  rr.type = type;
  rr.ttl = ttl;
  rr.content = content;
  assert(contentToRdata(rr));
  return rr;
}

int main() {
  auto q = makeQuery(DnsName::parse("www.example.com"), RrType::A, 42, false);
  auto wire = encodeMessage(q);
  DnsMessage parsed;
  assert(decodeMessage(wire, parsed));
  assert(parsed.header.id == 42);
  assert(parsed.questions.size() == 1);
  assert(parsed.questions[0].qname.equals(DnsName::parse("www.example.com")));
  assert(parsed.questions[0].qtype == RrType::A);

  DnsMessage resp = makeResponse(q);
  resp.header.aa = true;
  resp.answers.push_back(makeRr("www.example.com.", RrType::A, "192.0.2.1"));
  resp.answers.push_back(makeRr("www.example.com.", RrType::Aaaa, "2001:db8::1"));
  resp.answers.push_back(makeRr("example.com.", RrType::Ns, "ns.example.com."));
  resp.answers.push_back(makeRr("example.com.", RrType::Mx, "10 mail.example.com."));
  resp.answers.push_back(
      makeRr("example.com.", RrType::Soa, "ns.example.com. hostmaster.example.com. 1 10800 3600 604800 3600"));
  resp.answers.push_back(makeRr("example.com.", RrType::Txt, "\"hello world\""));
  resp.answers.push_back(makeRr("_sip._tcp.example.com.", RrType::Srv, "0 5 5060 sip.example.com."));
  resp.answers.push_back(makeRr("example.com.", RrType::Caa, "0 issue \"letsencrypt.org\""));
  auto rwire = encodeMessage(resp);
  DnsMessage back;
  assert(decodeMessage(rwire, back));
  assert(back.header.aa);
  assert(back.answers.size() == resp.answers.size());
  for (auto &rr : back.answers) {
    assert(!rr.content.empty() || rdataToContent(rr));
  }

  bool truncated = false;
  auto small = encodeMessage(resp, 32, &truncated);
  assert(truncated);
  DnsMessage trunc;
  assert(decodeMessage(small, trunc));
  assert(trunc.header.tc);

  std::cout << "test_dns_codec: ok" << std::endl;
  return 0;
}
