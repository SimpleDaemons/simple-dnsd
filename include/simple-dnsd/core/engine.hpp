/**
 * @file engine.hpp
 * @brief Authoritative DNS engine
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-dnsd/backend/backend.hpp"
#include "simple-dnsd/config/config.hpp"
#include "simple-dnsd/protocol/dns.hpp"
#include "simple-dnsd/protocol/tsig.hpp"
#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>

namespace simple_dnsd {

struct QueryContext {
  std::string peer_host;
  port_t peer_port{0};
  bool tcp{false};
  std::vector<uint8_t> raw;
};

struct ServerStats {
  std::atomic<uint64_t> queries{0};
  std::atomic<uint64_t> answers{0};
  std::atomic<uint64_t> nxdomain{0};
  std::atomic<uint64_t> refused{0};
  std::atomic<uint64_t> axfr{0};
  std::atomic<uint64_t> updates{0};
  std::atomic<uint64_t> cache_hits{0};
  std::atomic<uint64_t> cache_misses{0};
  std::atomic<uint64_t> truncated{0};
  std::atomic<uint64_t> tcp_sessions{0};
};

class PacketCache {
public:
  explicit PacketCache(std::size_t max_entries = 10000);
  void put(const std::string &key, const std::vector<uint8_t> &wire, uint32_t ttl);
  bool get(const std::string &key, std::vector<uint8_t> &wire);
  void invalidate();
  void clear();

private:
  struct Entry {
    std::vector<uint8_t> wire;
    uint64_t expires_ms{0};
  };
  std::size_t max_entries_;
  std::mutex mutex_;
  std::map<std::string, Entry> entries_;
};

class ResponseRateLimiter {
public:
  explicit ResponseRateLimiter(uint32_t rate_per_sec);
  bool allow(const std::string &ip);

private:
  uint32_t rate_;
  std::mutex mutex_;
  std::map<std::string, std::pair<uint64_t, uint32_t>> buckets_;
};

class AuthoritativeEngine {
public:
  AuthoritativeEngine(BackendRouter &router, DnsConfig config, ServerStats &stats);

  DnsMessage handle(const DnsMessage &query, const QueryContext &ctx);
  std::vector<DnsMessage> handleAxfr(const DnsMessage &query, const QueryContext &ctx);
  void notifySecondaries(const ZoneInfo &zone);

  BackendRouter &router() { return router_; }
  const DnsConfig &config() const { return config_; }
  PacketCache &cache() { return cache_; }
  void invalidateCache() { cache_.invalidate(); }

  std::optional<TsigKey> tsigKey(const DnsName &name) const;
  void addTsigKey(const TsigKey &key);

private:
  DnsMessage answerCached(const DnsMessage &query, const QueryContext &ctx);
  DnsMessage answerQuery(const DnsMessage &query, const QueryContext &ctx);
  DnsMessage handleUpdate(const DnsMessage &query, const QueryContext &ctx);
  void addSoa(DnsMessage &resp, const ZoneInfo &zone);
  void addGlue(DnsMessage &resp, const ZoneInfo &zone, const std::vector<ResourceRecord> &nsset);
  bool allowedAxfr(const ZoneInfo &zone, const std::string &peer) const;
  bool allowedUpdate(const ZoneInfo &zone, const std::string &peer,
                     const DnsMessage &query) const;

  BackendRouter &router_;
  DnsConfig config_;
  ServerStats &stats_;
  PacketCache cache_;
  ResponseRateLimiter rrl_;
  std::vector<TsigKey> tsig_keys_;
  mutable std::mutex tsig_mutex_;
};

}  // namespace simple_dnsd
