/**
 * @file dns.cpp
 * @brief DNS names, records, and wire codec
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/protocol/dns.hpp"
#include "simple-dnsd/utils/platform.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace simple_dnsd {

namespace {

void put16(std::vector<uint8_t> &out, uint16_t v) {
  out.push_back(static_cast<uint8_t>(v >> 8));
  out.push_back(static_cast<uint8_t>(v & 0xff));
}

void put32(std::vector<uint8_t> &out, uint32_t v) {
  out.push_back(static_cast<uint8_t>(v >> 24));
  out.push_back(static_cast<uint8_t>(v >> 16));
  out.push_back(static_cast<uint8_t>(v >> 8));
  out.push_back(static_cast<uint8_t>(v & 0xff));
}

bool get16(const std::vector<uint8_t> &in, std::size_t &off, uint16_t &v) {
  if (off + 2 > in.size()) {
    return false;
  }
  v = static_cast<uint16_t>((in[off] << 8) | in[off + 1]);
  off += 2;
  return true;
}

bool get32(const std::vector<uint8_t> &in, std::size_t &off, uint32_t &v) {
  if (off + 4 > in.size()) {
    return false;
  }
  v = (static_cast<uint32_t>(in[off]) << 24) | (static_cast<uint32_t>(in[off + 1]) << 16) |
      (static_cast<uint32_t>(in[off + 2]) << 8) | static_cast<uint32_t>(in[off + 3]);
  off += 4;
  return true;
}

std::string suffixKey(const std::vector<std::string> &labels, std::size_t from) {
  std::string key;
  for (std::size_t i = from; i < labels.size(); ++i) {
    if (i != from) {
      key.push_back('.');
    }
    key += toLower(labels[i]);
  }
  return key;
}

bool encodeUncompressed(std::vector<uint8_t> &out, const DnsName &name) {
  for (const auto &label : name.labels()) {
    if (label.size() > 63) {
      return false;
    }
    out.push_back(static_cast<uint8_t>(label.size()));
    out.insert(out.end(), label.begin(), label.end());
  }
  out.push_back(0);
  return true;
}

}  // namespace

DnsName DnsName::root() { return {}; }

DnsName DnsName::parse(const std::string &presentation) {
  DnsName n;
  std::string s = trim(presentation);
  if (s.empty() || s == ".") {
    return n;
  }
  if (s.back() == '.') {
    s.pop_back();
  }
  std::string cur;
  for (std::size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '\\' && i + 1 < s.size()) {
      ++i;
      cur.push_back(s[i]);
      continue;
    }
    if (s[i] == '.') {
      if (!cur.empty()) {
        n.labels_.push_back(cur);
        cur.clear();
      }
    } else {
      cur.push_back(s[i]);
    }
  }
  if (!cur.empty()) {
    n.labels_.push_back(cur);
  }
  return n;
}

DnsName DnsName::wildcard(const DnsName &parent) {
  DnsName n;
  n.labels_.emplace_back("*");
  n.labels_.insert(n.labels_.end(), parent.labels_.begin(), parent.labels_.end());
  return n;
}

std::string DnsName::toString(bool trailing_dot) const {
  if (labels_.empty()) {
    return ".";
  }
  std::string s;
  for (std::size_t i = 0; i < labels_.size(); ++i) {
    if (i != 0) {
      s.push_back('.');
    }
    s += labels_[i];
  }
  if (trailing_dot) {
    s.push_back('.');
  }
  return s;
}

std::string DnsName::toLowerString(bool trailing_dot) const {
  return toLower(toString(trailing_dot));
}

DnsName DnsName::parent() const {
  DnsName n;
  if (labels_.size() > 1) {
    n.labels_.assign(labels_.begin() + 1, labels_.end());
  }
  return n;
}

bool DnsName::isSubdomainOf(const DnsName &zone) const {
  if (labels_.size() < zone.labels_.size()) {
    return false;
  }
  const std::size_t off = labels_.size() - zone.labels_.size();
  for (std::size_t i = 0; i < zone.labels_.size(); ++i) {
    if (toLower(labels_[off + i]) != toLower(zone.labels_[i])) {
      return false;
    }
  }
  return true;
}

bool DnsName::equals(const DnsName &other) const {
  if (labels_.size() != other.labels_.size()) {
    return false;
  }
  for (std::size_t i = 0; i < labels_.size(); ++i) {
    if (toLower(labels_[i]) != toLower(other.labels_[i])) {
      return false;
    }
  }
  return true;
}

bool DnsName::operator<(const DnsName &other) const {
  return toLowerString() < other.toLowerString();
}

DnsName DnsName::makeRelative(const DnsName &origin) const {
  if (!isSubdomainOf(origin) || labels_.size() < origin.labels_.size()) {
    return *this;
  }
  DnsName n;
  n.labels_.assign(labels_.begin(), labels_.end() - static_cast<std::ptrdiff_t>(origin.labels_.size()));
  return n;
}

DnsName DnsName::qualify(const DnsName &origin) const {
  if (labels_.empty()) {
    return origin;
  }
  DnsName n = *this;
  n.labels_.insert(n.labels_.end(), origin.labels_.begin(), origin.labels_.end());
  return n;
}

bool DnsName::isWildcard() const { return !labels_.empty() && labels_[0] == "*"; }

std::string rrTypeToString(RrType type) {
  switch (type) {
  case RrType::A:
    return "A";
  case RrType::Ns:
    return "NS";
  case RrType::Cname:
    return "CNAME";
  case RrType::Soa:
    return "SOA";
  case RrType::Ptr:
    return "PTR";
  case RrType::Mx:
    return "MX";
  case RrType::Txt:
    return "TXT";
  case RrType::Aaaa:
    return "AAAA";
  case RrType::Srv:
    return "SRV";
  case RrType::Naptr:
    return "NAPTR";
  case RrType::Opt:
    return "OPT";
  case RrType::Tsig:
    return "TSIG";
  case RrType::Ixfr:
    return "IXFR";
  case RrType::Axfr:
    return "AXFR";
  case RrType::Any:
    return "ANY";
  case RrType::Caa:
    return "CAA";
  case RrType::Ds:
    return "DS";
  case RrType::Rrsig:
    return "RRSIG";
  case RrType::Nsec:
    return "NSEC";
  case RrType::Dnskey:
    return "DNSKEY";
  }
  return "TYPE" + std::to_string(static_cast<int>(type));
}

std::optional<RrType> rrTypeFromString(const std::string &name) {
  const std::string u = toLower(name);
  if (u == "a") {
    return RrType::A;
  }
  if (u == "ns") {
    return RrType::Ns;
  }
  if (u == "cname") {
    return RrType::Cname;
  }
  if (u == "soa") {
    return RrType::Soa;
  }
  if (u == "ptr") {
    return RrType::Ptr;
  }
  if (u == "mx") {
    return RrType::Mx;
  }
  if (u == "txt") {
    return RrType::Txt;
  }
  if (u == "aaaa") {
    return RrType::Aaaa;
  }
  if (u == "srv") {
    return RrType::Srv;
  }
  if (u == "naptr") {
    return RrType::Naptr;
  }
  if (u == "opt") {
    return RrType::Opt;
  }
  if (u == "tsig") {
    return RrType::Tsig;
  }
  if (u == "ixfr") {
    return RrType::Ixfr;
  }
  if (u == "axfr") {
    return RrType::Axfr;
  }
  if (u == "any") {
    return RrType::Any;
  }
  if (u == "caa") {
    return RrType::Caa;
  }
  if (u == "ds") {
    return RrType::Ds;
  }
  if (u == "rrsig") {
    return RrType::Rrsig;
  }
  if (u == "nsec") {
    return RrType::Nsec;
  }
  if (u == "dnskey") {
    return RrType::Dnskey;
  }
  if (u.rfind("type", 0) == 0) {
    try {
      return static_cast<RrType>(std::stoi(u.substr(4)));
    } catch (...) {
      return std::nullopt;
    }
  }
  return std::nullopt;
}

std::string rcodeToString(Rcode rcode) {
  switch (rcode) {
  case Rcode::NoError:
    return "NOERROR";
  case Rcode::FormErr:
    return "FORMERR";
  case Rcode::ServFail:
    return "SERVFAIL";
  case Rcode::NxDomain:
    return "NXDOMAIN";
  case Rcode::NotImp:
    return "NOTIMP";
  case Rcode::Refused:
    return "REFUSED";
  default:
    return "RCODE" + std::to_string(static_cast<int>(rcode));
  }
}

bool encodeName(std::vector<uint8_t> &out, const DnsName &name,
                std::map<std::string, uint16_t> *compression, bool compress) {
  if (!compress || compression == nullptr) {
    return encodeUncompressed(out, name);
  }
  if (name.labels().empty()) {
    out.push_back(0);
    return true;
  }
  for (std::size_t i = 0; i < name.labels().size(); ++i) {
    const std::string key = suffixKey(name.labels(), i);
    auto it = compression->find(key);
    if (it != compression->end() && it->second < 0x4000) {
      const uint16_t ptr = static_cast<uint16_t>(0xC000u | it->second);
      put16(out, ptr);
      return true;
    }
    if (out.size() < 0x4000) {
      (*compression)[key] = static_cast<uint16_t>(out.size());
    }
    const auto &label = name.labels()[i];
    if (label.size() > 63) {
      return false;
    }
    out.push_back(static_cast<uint8_t>(label.size()));
    out.insert(out.end(), label.begin(), label.end());
  }
  out.push_back(0);
  return true;
}

bool decodeName(const std::vector<uint8_t> &in, std::size_t &offset, DnsName &name, int jumps) {
  if (jumps > 20) {
    return false;
  }
  std::vector<std::string> labels;
  std::size_t pos = offset;
  bool jumped = false;
  while (pos < in.size()) {
    const uint8_t len = in[pos];
    if ((len & 0xC0) == 0xC0) {
      if (pos + 1 >= in.size()) {
        return false;
      }
      const uint16_t ptr = static_cast<uint16_t>(((len & 0x3F) << 8) | in[pos + 1]);
      if (!jumped) {
        offset = pos + 2;
        jumped = true;
      }
      std::size_t tmp = ptr;
      DnsName rest;
      if (!decodeName(in, tmp, rest, jumps + 1)) {
        return false;
      }
      labels.insert(labels.end(), rest.labels().begin(), rest.labels().end());
      name = DnsName::parse(join(labels, ".") + ".");
      return true;
    }
    if ((len & 0xC0) != 0) {
      return false;
    }
    ++pos;
    if (len == 0) {
      if (!jumped) {
        offset = pos;
      }
      name = labels.empty() ? DnsName::root() : DnsName::parse(join(labels, ".") + ".");
      return true;
    }
    if (pos + len > in.size()) {
      return false;
    }
    labels.emplace_back(reinterpret_cast<const char *>(&in[pos]), len);
    pos += len;
  }
  return false;
}

namespace {

bool encodeRr(std::vector<uint8_t> &out, const ResourceRecord &rr,
              std::map<std::string, uint16_t> *compression) {
  if (!encodeName(out, rr.name, compression, true)) {
    return false;
  }
  put16(out, static_cast<uint16_t>(rr.type));
  put16(out, static_cast<uint16_t>(rr.rclass));
  put32(out, rr.ttl);
  put16(out, static_cast<uint16_t>(rr.rdata.size()));
  out.insert(out.end(), rr.rdata.begin(), rr.rdata.end());
  return true;
}

bool decodeRr(const std::vector<uint8_t> &in, std::size_t &off, ResourceRecord &rr) {
  if (!decodeName(in, off, rr.name)) {
    return false;
  }
  uint16_t type = 0;
  uint16_t rclass = 0;
  uint16_t rdlen = 0;
  if (!get16(in, off, type) || !get16(in, off, rclass) || !get32(in, off, rr.ttl) ||
      !get16(in, off, rdlen)) {
    return false;
  }
  if (off + rdlen > in.size()) {
    return false;
  }
  rr.type = static_cast<RrType>(type);
  rr.rclass = static_cast<RrClass>(rclass);
  rr.rdata.assign(in.begin() + static_cast<std::ptrdiff_t>(off),
                  in.begin() + static_cast<std::ptrdiff_t>(off + rdlen));
  off += rdlen;
  rdataToContent(rr);
  return true;
}

}  // namespace

std::vector<uint8_t> encodeMessage(const DnsMessage &msg, std::size_t truncate_at,
                                   bool *truncated) {
  if (truncated) {
    *truncated = false;
  }
  std::vector<uint8_t> out;
  out.resize(12);
  std::map<std::string, uint16_t> compression;
  auto writeSections = [&](bool answers, bool authority, bool additional) {
    out.resize(12);
    compression.clear();
    for (const auto &q : msg.questions) {
      encodeName(out, q.qname, &compression, true);
      put16(out, static_cast<uint16_t>(q.qtype));
      put16(out, static_cast<uint16_t>(q.qclass));
    }
    if (answers) {
      for (const auto &rr : msg.answers) {
        encodeRr(out, rr, &compression);
      }
    }
    if (authority) {
      for (const auto &rr : msg.authority) {
        encodeRr(out, rr, &compression);
      }
    }
    if (additional) {
      for (const auto &rr : msg.additional) {
        encodeRr(out, rr, &compression);
      }
      if (msg.tsig) {
        encodeRr(out, *msg.tsig, nullptr);
      }
    }
  };
  writeSections(true, true, true);
  bool tc = msg.header.tc;
  if (truncate_at > 0 && out.size() > truncate_at) {
    tc = true;
    if (truncated) {
      *truncated = true;
    }
    writeSections(false, false, false);
  }
  const uint16_t flags =
      static_cast<uint16_t>((msg.header.qr ? 0x8000 : 0) |
                            (static_cast<uint16_t>(msg.header.opcode) << 11) |
                            (msg.header.aa ? 0x0400 : 0) | (tc ? 0x0200 : 0) |
                            (msg.header.rd ? 0x0100 : 0) | (msg.header.ra ? 0x0080 : 0) |
                            (msg.header.ad ? 0x0020 : 0) | (msg.header.cd ? 0x0010 : 0) |
                            (static_cast<uint16_t>(msg.header.rcode) & 0x0f));
  out[0] = static_cast<uint8_t>(msg.header.id >> 8);
  out[1] = static_cast<uint8_t>(msg.header.id & 0xff);
  out[2] = static_cast<uint8_t>(flags >> 8);
  out[3] = static_cast<uint8_t>(flags & 0xff);
  const uint16_t qd = static_cast<uint16_t>(msg.questions.size());
  const uint16_t an = tc ? 0 : static_cast<uint16_t>(msg.answers.size());
  const uint16_t ns = tc ? 0 : static_cast<uint16_t>(msg.authority.size());
  uint16_t ar = tc ? 0 : static_cast<uint16_t>(msg.additional.size());
  if (!tc && msg.tsig) {
    ++ar;
  }
  out[4] = static_cast<uint8_t>(qd >> 8);
  out[5] = static_cast<uint8_t>(qd & 0xff);
  out[6] = static_cast<uint8_t>(an >> 8);
  out[7] = static_cast<uint8_t>(an & 0xff);
  out[8] = static_cast<uint8_t>(ns >> 8);
  out[9] = static_cast<uint8_t>(ns & 0xff);
  out[10] = static_cast<uint8_t>(ar >> 8);
  out[11] = static_cast<uint8_t>(ar & 0xff);
  return out;
}

bool decodeMessage(const std::vector<uint8_t> &in, DnsMessage &msg) {
  msg = {};
  if (in.size() < 12) {
    return false;
  }
  std::size_t off = 0;
  if (!get16(in, off, msg.header.id)) {
    return false;
  }
  uint16_t flags = 0;
  if (!get16(in, off, flags)) {
    return false;
  }
  msg.header.qr = (flags & 0x8000) != 0;
  msg.header.opcode = static_cast<Opcode>((flags >> 11) & 0x0f);
  msg.header.aa = (flags & 0x0400) != 0;
  msg.header.tc = (flags & 0x0200) != 0;
  msg.header.rd = (flags & 0x0100) != 0;
  msg.header.ra = (flags & 0x0080) != 0;
  msg.header.ad = (flags & 0x0020) != 0;
  msg.header.cd = (flags & 0x0010) != 0;
  msg.header.rcode = static_cast<Rcode>(flags & 0x0f);
  if (!get16(in, off, msg.header.qdcount) || !get16(in, off, msg.header.ancount) ||
      !get16(in, off, msg.header.nscount) || !get16(in, off, msg.header.arcount)) {
    return false;
  }
  for (uint16_t i = 0; i < msg.header.qdcount; ++i) {
    Question q;
    if (!decodeName(in, off, q.qname)) {
      return false;
    }
    uint16_t t = 0;
    uint16_t c = 0;
    if (!get16(in, off, t) || !get16(in, off, c)) {
      return false;
    }
    q.qtype = static_cast<RrType>(t);
    q.qclass = static_cast<RrClass>(c);
    msg.questions.push_back(q);
  }
  auto readN = [&](uint16_t n, std::vector<ResourceRecord> &into) {
    for (uint16_t i = 0; i < n; ++i) {
      ResourceRecord rr;
      if (!decodeRr(in, off, rr)) {
        return false;
      }
      into.push_back(std::move(rr));
    }
    return true;
  };
  if (!readN(msg.header.ancount, msg.answers) || !readN(msg.header.nscount, msg.authority)) {
    return false;
  }
  std::vector<ResourceRecord> ar;
  if (!readN(msg.header.arcount, ar)) {
    return false;
  }
  for (auto &rr : ar) {
    if (rr.type == RrType::Opt) {
      msg.edns.present = true;
      msg.edns.udp_size = static_cast<uint16_t>(rr.rclass);
      msg.edns.ext_rcode = static_cast<uint8_t>((rr.ttl >> 24) & 0xff);
      msg.edns.version = static_cast<uint8_t>((rr.ttl >> 16) & 0xff);
      msg.edns.dnssec_ok = (rr.ttl & 0x8000) != 0;
      msg.additional.push_back(rr);
    } else if (rr.type == RrType::Tsig) {
      msg.tsig = rr;
    } else {
      msg.additional.push_back(rr);
    }
  }
  return true;
}

namespace {

bool parseIpv4(const std::string &s, std::vector<uint8_t> &out) {
  in_addr addr{};
  if (inet_pton(AF_INET, s.c_str(), &addr) != 1) {
    return false;
  }
  const uint8_t *p = reinterpret_cast<const uint8_t *>(&addr);
  out.assign(p, p + 4);
  return true;
}

bool parseIpv6(const std::string &s, std::vector<uint8_t> &out) {
  in6_addr addr{};
  if (inet_pton(AF_INET6, s.c_str(), &addr) != 1) {
    return false;
  }
  const uint8_t *p = reinterpret_cast<const uint8_t *>(&addr);
  out.assign(p, p + 16);
  return true;
}

std::string ipv4ToString(const std::vector<uint8_t> &rdata) {
  if (rdata.size() != 4) {
    return {};
  }
  char buf[INET_ADDRSTRLEN] = {};
  inet_ntop(AF_INET, rdata.data(), buf, sizeof(buf));
  return buf;
}

std::string ipv6ToString(const std::vector<uint8_t> &rdata) {
  if (rdata.size() != 16) {
    return {};
  }
  char buf[INET6_ADDRSTRLEN] = {};
  inet_ntop(AF_INET6, rdata.data(), buf, sizeof(buf));
  return buf;
}

bool encodeDomainRdata(std::vector<uint8_t> &out, const std::string &name) {
  return encodeName(out, DnsName::parse(name), nullptr, false);
}

bool decodeDomainRdata(const std::vector<uint8_t> &in, std::size_t &off, std::string &out) {
  DnsName n;
  if (!decodeName(in, off, n)) {
    return false;
  }
  out = n.toString();
  return true;
}

}  // namespace

bool contentToRdata(ResourceRecord &rr) {
  if (!rr.rdata.empty() && rr.content.empty()) {
    return rdataToContent(rr);
  }
  if (rr.content.empty() && !rr.rdata.empty()) {
    return true;
  }
  std::vector<uint8_t> out;
  switch (rr.type) {
  case RrType::A:
    if (!parseIpv4(rr.content, out)) {
      return false;
    }
    break;
  case RrType::Aaaa:
    if (!parseIpv6(rr.content, out)) {
      return false;
    }
    break;
  case RrType::Ns:
  case RrType::Cname:
  case RrType::Ptr:
    if (!encodeDomainRdata(out, rr.content)) {
      return false;
    }
    break;
  case RrType::Mx: {
    uint16_t prio = rr.prio;
    std::string host = rr.content;
    auto parts = splitWs(rr.content);
    if (parts.size() >= 2) {
      try {
        prio = static_cast<uint16_t>(std::stoi(parts[0]));
        host = parts[1];
      } catch (...) {
      }
    }
    rr.prio = prio;
    put16(out, prio);
    if (!encodeDomainRdata(out, host)) {
      return false;
    }
    break;
  }
  case RrType::Soa: {
    auto parts = splitWs(rr.content);
    if (parts.size() < 7) {
      return false;
    }
    if (!encodeDomainRdata(out, parts[0]) || !encodeDomainRdata(out, parts[1])) {
      return false;
    }
    for (int i = 2; i < 7; ++i) {
      put32(out, static_cast<uint32_t>(std::stoul(parts[static_cast<std::size_t>(i)])));
    }
    break;
  }
  case RrType::Txt: {
    std::string text = rr.content;
    if (text.size() >= 2 && text.front() == '"' && text.back() == '"') {
      text = text.substr(1, text.size() - 2);
    }
    std::size_t i = 0;
    while (i < text.size()) {
      const std::size_t n = std::min<std::size_t>(255, text.size() - i);
      out.push_back(static_cast<uint8_t>(n));
      out.insert(out.end(), text.begin() + static_cast<std::ptrdiff_t>(i),
                 text.begin() + static_cast<std::ptrdiff_t>(i + n));
      i += n;
    }
    if (text.empty()) {
      out.push_back(0);
    }
    break;
  }
  case RrType::Srv: {
    auto parts = splitWs(rr.content);
    uint16_t prio = rr.prio;
    uint16_t weight = 0;
    uint16_t port = 0;
    std::string target;
    if (parts.size() == 4) {
      prio = static_cast<uint16_t>(std::stoi(parts[0]));
      weight = static_cast<uint16_t>(std::stoi(parts[1]));
      port = static_cast<uint16_t>(std::stoi(parts[2]));
      target = parts[3];
    } else if (parts.size() == 3) {
      weight = static_cast<uint16_t>(std::stoi(parts[0]));
      port = static_cast<uint16_t>(std::stoi(parts[1]));
      target = parts[2];
    } else {
      return false;
    }
    rr.prio = prio;
    put16(out, prio);
    put16(out, weight);
    put16(out, port);
    if (!encodeDomainRdata(out, target)) {
      return false;
    }
    break;
  }
  case RrType::Caa: {
    auto parts = splitWs(rr.content);
    if (parts.size() < 3) {
      return false;
    }
    out.push_back(static_cast<uint8_t>(std::stoi(parts[0])));
    out.push_back(static_cast<uint8_t>(parts[1].size()));
    out.insert(out.end(), parts[1].begin(), parts[1].end());
    std::string val = join(std::vector<std::string>(parts.begin() + 2, parts.end()), " ");
    if (val.size() >= 2 && val.front() == '"' && val.back() == '"') {
      val = val.substr(1, val.size() - 2);
    }
    out.insert(out.end(), val.begin(), val.end());
    break;
  }
  default: {
    if (!rr.rdata.empty()) {
      return true;
    }
    if (rr.content.rfind("\\#", 0) == 0) {
      auto parts = splitWs(rr.content);
      if (parts.size() < 2) {
        return false;
      }
      std::string hex;
      for (std::size_t i = 2; i < parts.size(); ++i) {
        hex += parts[i];
      }
      if (hex.size() % 2 != 0) {
        return false;
      }
      for (std::size_t i = 0; i < hex.size(); i += 2) {
        out.push_back(static_cast<uint8_t>(std::stoi(hex.substr(i, 2), nullptr, 16)));
      }
    } else {
      return false;
    }
    break;
  }
  }
  rr.rdata = std::move(out);
  return true;
}

bool rdataToContent(ResourceRecord &rr) {
  if (rr.rdata.empty() && !rr.content.empty()) {
    return true;
  }
  std::size_t off = 0;
  switch (rr.type) {
  case RrType::A:
    rr.content = ipv4ToString(rr.rdata);
    return !rr.content.empty();
  case RrType::Aaaa:
    rr.content = ipv6ToString(rr.rdata);
    return !rr.content.empty();
  case RrType::Ns:
  case RrType::Cname:
  case RrType::Ptr:
    return decodeDomainRdata(rr.rdata, off, rr.content);
  case RrType::Mx: {
    uint16_t prio = 0;
    if (!get16(rr.rdata, off, prio)) {
      return false;
    }
    rr.prio = prio;
    std::string host;
    if (!decodeDomainRdata(rr.rdata, off, host)) {
      return false;
    }
    rr.content = std::to_string(prio) + " " + host;
    return true;
  }
  case RrType::Soa: {
    std::string mname;
    std::string rname;
    if (!decodeDomainRdata(rr.rdata, off, mname) || !decodeDomainRdata(rr.rdata, off, rname)) {
      return false;
    }
    uint32_t serial = 0, refresh = 0, retry = 0, expire = 0, minimum = 0;
    if (!get32(rr.rdata, off, serial) || !get32(rr.rdata, off, refresh) ||
        !get32(rr.rdata, off, retry) || !get32(rr.rdata, off, expire) ||
        !get32(rr.rdata, off, minimum)) {
      return false;
    }
    rr.content = mname + " " + rname + " " + std::to_string(serial) + " " +
                 std::to_string(refresh) + " " + std::to_string(retry) + " " +
                 std::to_string(expire) + " " + std::to_string(minimum);
    return true;
  }
  case RrType::Txt: {
    std::string text;
    while (off < rr.rdata.size()) {
      const uint8_t n = rr.rdata[off++];
      if (off + n > rr.rdata.size()) {
        return false;
      }
      text.append(reinterpret_cast<const char *>(&rr.rdata[off]), n);
      off += n;
    }
    rr.content = "\"" + text + "\"";
    return true;
  }
  case RrType::Srv: {
    uint16_t prio = 0, weight = 0, port = 0;
    if (!get16(rr.rdata, off, prio) || !get16(rr.rdata, off, weight) ||
        !get16(rr.rdata, off, port)) {
      return false;
    }
    rr.prio = prio;
    std::string target;
    if (!decodeDomainRdata(rr.rdata, off, target)) {
      return false;
    }
    rr.content = std::to_string(prio) + " " + std::to_string(weight) + " " +
                 std::to_string(port) + " " + target;
    return true;
  }
  case RrType::Caa: {
    if (rr.rdata.size() < 2) {
      return false;
    }
    const uint8_t flag = rr.rdata[0];
    const uint8_t taglen = rr.rdata[1];
    if (static_cast<std::size_t>(2 + taglen) > rr.rdata.size()) {
      return false;
    }
    const std::string tag(reinterpret_cast<const char *>(&rr.rdata[2]), taglen);
    const std::string val(reinterpret_cast<const char *>(&rr.rdata[2 + taglen]),
                          rr.rdata.size() - 2 - taglen);
    rr.content = std::to_string(flag) + " " + tag + " \"" + val + "\"";
    return true;
  }
  default: {
    std::ostringstream hex;
    hex << "\\# " << rr.rdata.size();
    for (uint8_t b : rr.rdata) {
      hex << " " << std::hex << std::uppercase;
      if (b < 16) {
        hex << "0";
      }
      hex << static_cast<int>(b);
    }
    rr.content = hex.str();
    return true;
  }
  }
}

DnsMessage makeQuery(const DnsName &qname, RrType qtype, uint16_t id, bool rd) {
  DnsMessage msg;
  msg.header.id = id;
  msg.header.rd = rd;
  Question q;
  q.qname = qname;
  q.qtype = qtype;
  msg.questions.push_back(q);
  return msg;
}

DnsMessage makeResponse(const DnsMessage &query) {
  DnsMessage msg;
  msg.header.id = query.header.id;
  msg.header.qr = true;
  msg.header.opcode = query.header.opcode;
  msg.header.rd = query.header.rd;
  msg.questions = query.questions;
  msg.edns = query.edns;
  return msg;
}

}  // namespace simple_dnsd
