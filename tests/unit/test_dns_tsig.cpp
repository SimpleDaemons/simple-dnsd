/**
 * @file test_dns_tsig.cpp
 * @brief TSIG HMAC-SHA256 tests
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/protocol/tsig.hpp"

#include <cassert>
#include <iostream>

using namespace simple_dnsd;

int main() {
  std::vector<uint8_t> decoded;
  assert(decodeBase64("AQIDBA==", decoded));
  assert(decoded.size() == 4);
  assert(encodeBase64(decoded) == "AQIDBA==");

  TsigKey key;
  key.name = DnsName::parse("test.key.");
  decodeBase64("c2VjcmV0c2VjcmV0c2VjcmV0c2VjcmV0c2VjcmV0c2Vj", key.secret);
  auto msg = makeQuery(DnsName::parse("example.com"), RrType::Soa, 9);
  assert(signMessage(msg, key));
  assert(msg.tsig);
  auto wire = encodeMessage(msg);
  std::string err;
  DnsMessage parsed;
  assert(decodeMessage(wire, parsed));
  assert(verifyMessage(parsed, wire, key, err));

  TsigKey wrong = key;
  wrong.secret.assign(32, 0x11);
  assert(!verifyMessage(parsed, wire, wrong, err));

  std::cout << "test_dns_tsig: ok" << std::endl;
  return 0;
}
