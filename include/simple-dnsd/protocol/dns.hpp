/**
 * @file dns.hpp
 * @brief DNS names, records, and wire codec
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace simple_dnsd {

enum class Opcode : uint8_t { Query = 0, Iquery = 1, Status = 2, Notify = 4, Update = 5 };

enum class Rcode : uint8_t {
  NoError = 0,
  FormErr = 1,
  ServFail = 2,
  NxDomain = 3,
  NotImp = 4,
  Refused = 5,
  YxDomain = 6,
  YxRrset = 7,
  NxRrset = 8,
  NotAuth = 9,
  NotZone = 10
};

enum class RrType : uint16_t {
  A = 1,
  Ns = 2,
  Cname = 5,
  Soa = 6,
  Ptr = 12,
  Mx = 15,
  Txt = 16,
  Aaaa = 28,
  Srv = 33,
  Naptr = 35,
  Opt = 41,
  Ds = 43,
  Rrsig = 46,
  Nsec = 47,
  Dnskey = 48,
  Tsig = 250,
  Ixfr = 251,
  Axfr = 252,
  Any = 255,
  Caa = 257
};

enum class RrClass : uint16_t { In = 1, Cs = 2, Ch = 3, Hs = 4, Any = 255 };

class DnsName {
public:
  DnsName() = default;
  static DnsName root();
  static DnsName parse(const std::string &presentation);
  static DnsName wildcard(const DnsName &parent);

  bool empty() const { return labels_.empty(); }
  bool isRoot() const { return labels_.empty(); }
  std::size_t labelCount() const { return labels_.size(); }
  const std::vector<std::string> &labels() const { return labels_; }

  std::string toString(bool trailing_dot = true) const;
  std::string toLowerString(bool trailing_dot = true) const;
  DnsName parent() const;
  bool isSubdomainOf(const DnsName &zone) const;
  bool equals(const DnsName &other) const;
  bool operator==(const DnsName &other) const { return equals(other); }
  bool operator!=(const DnsName &other) const { return !equals(other); }
  bool operator<(const DnsName &other) const;

  DnsName makeRelative(const DnsName &origin) const;
  DnsName qualify(const DnsName &origin) const;
  bool isWildcard() const;

private:
  std::vector<std::string> labels_;
};

struct ResourceRecord {
  DnsName name;
  RrType type{RrType::A};
  RrClass rclass{RrClass::In};
  uint32_t ttl{3600};
  std::vector<uint8_t> rdata;
  std::string content;
  uint16_t prio{0};
  bool disabled{false};
  bool auth{true};
};

struct Question {
  DnsName qname;
  RrType qtype{RrType::A};
  RrClass qclass{RrClass::In};
};

struct DnsHeader {
  uint16_t id{0};
  bool qr{false};
  Opcode opcode{Opcode::Query};
  bool aa{false};
  bool tc{false};
  bool rd{false};
  bool ra{false};
  bool ad{false};
  bool cd{false};
  Rcode rcode{Rcode::NoError};
  uint16_t qdcount{0};
  uint16_t ancount{0};
  uint16_t nscount{0};
  uint16_t arcount{0};
};

struct EdnsInfo {
  bool present{false};
  uint16_t udp_size{512};
  uint8_t ext_rcode{0};
  uint8_t version{0};
  bool dnssec_ok{false};
};

struct DnsMessage {
  DnsHeader header;
  std::vector<Question> questions;
  std::vector<ResourceRecord> answers;
  std::vector<ResourceRecord> authority;
  std::vector<ResourceRecord> additional;
  EdnsInfo edns;
  std::optional<ResourceRecord> tsig;
};

std::string rrTypeToString(RrType type);
std::optional<RrType> rrTypeFromString(const std::string &name);
std::string rcodeToString(Rcode rcode);

bool encodeName(std::vector<uint8_t> &out, const DnsName &name,
                std::map<std::string, uint16_t> *compression, bool compress = true);
bool decodeName(const std::vector<uint8_t> &in, std::size_t &offset, DnsName &name,
                int jumps = 0);

std::vector<uint8_t> encodeMessage(const DnsMessage &msg, std::size_t truncate_at = 0,
                                   bool *truncated = nullptr);
bool decodeMessage(const std::vector<uint8_t> &in, DnsMessage &msg);

bool contentToRdata(ResourceRecord &rr);
bool rdataToContent(ResourceRecord &rr);

DnsMessage makeQuery(const DnsName &qname, RrType qtype, uint16_t id = 0,
                     bool rd = false);
DnsMessage makeResponse(const DnsMessage &query);

}  // namespace simple_dnsd
