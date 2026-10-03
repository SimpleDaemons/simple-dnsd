/**
 * @file tsig.cpp
 * @brief TSIG HMAC-SHA256
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/protocol/tsig.hpp"

#ifdef SIMPLE_DNSD_SSL
#include <openssl/evp.h>
#include <openssl/hmac.h>
#endif

#include <chrono>
#include <cstring>

namespace simple_dnsd {

namespace {

const char kB64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void put16(std::vector<uint8_t> &out, uint16_t v) {
  out.push_back(static_cast<uint8_t>(v >> 8));
  out.push_back(static_cast<uint8_t>(v & 0xff));
}

void put48(std::vector<uint8_t> &out, uint64_t v) {
  out.push_back(static_cast<uint8_t>((v >> 40) & 0xff));
  out.push_back(static_cast<uint8_t>((v >> 32) & 0xff));
  out.push_back(static_cast<uint8_t>((v >> 24) & 0xff));
  out.push_back(static_cast<uint8_t>((v >> 16) & 0xff));
  out.push_back(static_cast<uint8_t>((v >> 8) & 0xff));
  out.push_back(static_cast<uint8_t>(v & 0xff));
}

}  // namespace

bool decodeBase64(const std::string &in, std::vector<uint8_t> &out) {
  out.clear();
  int val = 0;
  int valb = -8;
  for (unsigned char c : in) {
    if (c == '=' || c == '\n' || c == '\r' || c == ' ') {
      continue;
    }
    const char *p = std::strchr(kB64, c);
    if (p == nullptr) {
      return false;
    }
    val = (val << 6) + static_cast<int>(p - kB64);
    valb += 6;
    if (valb >= 0) {
      out.push_back(static_cast<uint8_t>((val >> valb) & 0xff));
      valb -= 8;
    }
  }
  return true;
}

std::string encodeBase64(const std::vector<uint8_t> &in) {
  std::string out;
  int val = 0;
  int valb = -6;
  for (uint8_t c : in) {
    val = (val << 8) + c;
    valb += 8;
    while (valb >= 0) {
      out.push_back(kB64[(val >> valb) & 0x3f]);
      valb -= 6;
    }
  }
  if (valb > -6) {
    out.push_back(kB64[((val << 8) >> (valb + 8)) & 0x3f]);
  }
  while (out.size() % 4 != 0) {
    out.push_back('=');
  }
  return out;
}

std::vector<uint8_t> hmacSha256(const std::vector<uint8_t> &key, const std::vector<uint8_t> &data) {
  std::vector<uint8_t> out(32);
#ifdef SIMPLE_DNSD_SSL
  unsigned int len = 32;
  HMAC(EVP_sha256(), key.data(), static_cast<int>(key.size()), data.data(), data.size(),
       out.data(), &len);
  out.resize(len);
#else
  (void)key;
  (void)data;
  out.clear();
#endif
  return out;
}

bool signMessage(DnsMessage &msg, const TsigKey &key, uint16_t fudge) {
  DnsMessage unsigned_msg = msg;
  unsigned_msg.tsig.reset();
  const auto wire = encodeMessage(unsigned_msg);
  const uint64_t now = static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
  std::vector<uint8_t> mac_input = wire;
  encodeName(mac_input, key.name, nullptr, false);
  put16(mac_input, static_cast<uint16_t>(RrClass::Any));
  mac_input.insert(mac_input.end(), {0, 0, 0, 0});
  encodeName(mac_input, DnsName::parse(key.algorithm), nullptr, false);
  put48(mac_input, now);
  put16(mac_input, fudge);
  put16(mac_input, 0);
  put16(mac_input, 0);
  const auto mac = hmacSha256(key.secret, mac_input);
  ResourceRecord tsig;
  tsig.name = key.name;
  tsig.type = RrType::Tsig;
  tsig.rclass = RrClass::Any;
  tsig.ttl = 0;
  std::vector<uint8_t> rdata;
  encodeName(rdata, DnsName::parse(key.algorithm), nullptr, false);
  put48(rdata, now);
  put16(rdata, fudge);
  put16(rdata, static_cast<uint16_t>(mac.size()));
  rdata.insert(rdata.end(), mac.begin(), mac.end());
  put16(rdata, msg.header.id);
  put16(rdata, 0);
  put16(rdata, 0);
  tsig.rdata = std::move(rdata);
  rdataToContent(tsig);
  msg.tsig = tsig;
  return !mac.empty() || key.secret.empty();
}

bool verifyMessage(const DnsMessage &msg, const std::vector<uint8_t> &wire, const TsigKey &key,
                   std::string &error) {
  if (!msg.tsig) {
    error = "no tsig";
    return false;
  }
  DnsMessage unsigned_msg = msg;
  unsigned_msg.tsig.reset();
  const auto rebuilt = encodeMessage(unsigned_msg);
  (void)wire;
  const auto &rdata = msg.tsig->rdata;
  std::size_t off = 0;
  DnsName alg;
  if (!decodeName(rdata, off, alg)) {
    error = "bad tsig rdata";
    return false;
  }
  if (off + 6 + 2 + 2 > rdata.size()) {
    error = "short tsig";
    return false;
  }
  uint64_t time_signed = 0;
  for (int i = 0; i < 6; ++i) {
    time_signed = (time_signed << 8) | rdata[off++];
  }
  const uint16_t fudge = static_cast<uint16_t>((rdata[off] << 8) | rdata[off + 1]);
  off += 2;
  const uint16_t mac_size = static_cast<uint16_t>((rdata[off] << 8) | rdata[off + 1]);
  off += 2;
  if (off + mac_size > rdata.size()) {
    error = "short mac";
    return false;
  }
  std::vector<uint8_t> mac(rdata.begin() + static_cast<std::ptrdiff_t>(off),
                           rdata.begin() + static_cast<std::ptrdiff_t>(off + mac_size));
  const uint64_t now = static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
  const uint64_t diff = now > time_signed ? now - time_signed : time_signed - now;
  if (diff > fudge) {
    error = "tsig time";
    return false;
  }
  std::vector<uint8_t> mac_input = rebuilt;
  encodeName(mac_input, key.name, nullptr, false);
  put16(mac_input, static_cast<uint16_t>(RrClass::Any));
  mac_input.insert(mac_input.end(), {0, 0, 0, 0});
  encodeName(mac_input, DnsName::parse(key.algorithm), nullptr, false);
  put48(mac_input, time_signed);
  put16(mac_input, fudge);
  put16(mac_input, 0);
  put16(mac_input, 0);
  const auto expect = hmacSha256(key.secret, mac_input);
  if (expect != mac) {
    error = "bad mac";
    return false;
  }
  return true;
}

}  // namespace simple_dnsd
