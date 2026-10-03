/**
 * @file tsig.hpp
 * @brief TSIG HMAC-SHA256 (RFC 2845 / 4635)
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-dnsd/protocol/dns.hpp"
#include <string>
#include <vector>

namespace simple_dnsd {

struct TsigKey {
  DnsName name;
  std::string algorithm{"hmac-sha256."};
  std::vector<uint8_t> secret;
};

bool decodeBase64(const std::string &in, std::vector<uint8_t> &out);
std::string encodeBase64(const std::vector<uint8_t> &in);

std::vector<uint8_t> hmacSha256(const std::vector<uint8_t> &key,
                                const std::vector<uint8_t> &data);

bool signMessage(DnsMessage &msg, const TsigKey &key, uint16_t fudge = 300);
bool verifyMessage(const DnsMessage &msg, const std::vector<uint8_t> &wire,
                   const TsigKey &key, std::string &error);

}  // namespace simple_dnsd
