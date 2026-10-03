/**
 * @file server.cpp
 * @brief UDP/TCP listeners, API, secondary refresh
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/core/server.hpp"
#include "simple-dnsd/utils/logger.hpp"
#include "simple-dnsd/zone/zone.hpp"

#include <chrono>

namespace simple_dnsd {

DnsServer::DnsServer(DnsConfig config) : config_(std::move(config)) {
  loadBackends(router_, config_);
}

DnsServer::~DnsServer() { stop(); }

bool DnsServer::testConfig() { return config_.validate() && router_.initialize(); }

bool DnsServer::start() {
  if (!initializeSockets()) {
    return false;
  }
  LogLevel level{};
  if (parseLogLevel(config_.log_level, level)) {
    Logger::instance().setLevel(level);
  }
  if (!config_.log_file.empty()) {
    Logger::instance().setLogFile(config_.log_file);
  }
  if (!router_.initialize()) {
    Logger::instance().error("backend initialization failed");
    return false;
  }
  engine_ = std::make_unique<AuthoritativeEngine>(router_, config_, stats_);
  if (!udp_.bindAddress(config_.listen_address, config_.dns_port)) {
    Logger::instance().error("failed to bind UDP " + config_.listen_address + ":" +
                             std::to_string(config_.dns_port));
    return false;
  }
  dns_port_ = udp_.boundPort();
  if (!tcp_.bindAndListen(config_.listen_address, dns_port_)) {
    Logger::instance().error("failed to bind TCP");
    return false;
  }
  if (config_.enable_api) {
    if (!api_.bindAndListen(config_.api_listen, config_.api_port)) {
      Logger::instance().error("failed to bind API");
      return false;
    }
    api_port_ = api_.boundPort();
  }
  if (!dropPrivileges(config_.service_user)) {
    return false;
  }
  running_ = true;
  udp_thread_ = std::thread([this] { udpLoop(); });
  tcp_thread_ = std::thread([this] { tcpLoop(); });
  if (config_.enable_api) {
    api_thread_ = std::thread([this] { apiLoop(); });
  }
  secondary_thread_ = std::thread([this] { secondaryLoop(); });
  Logger::instance().info("simple-dnsd listening on UDP/TCP port " + std::to_string(dns_port_));
  return true;
}

void DnsServer::stop() {
  running_ = false;
  udp_.close();
  tcp_.close();
  api_.close();
  if (udp_thread_.joinable()) {
    udp_thread_.join();
  }
  if (tcp_thread_.joinable()) {
    tcp_thread_.join();
  }
  if (api_thread_.joinable()) {
    api_thread_.join();
  }
  if (secondary_thread_.joinable()) {
    secondary_thread_.join();
  }
  for (auto &w : workers_) {
    if (w.joinable()) {
      w.join();
    }
  }
  workers_.clear();
}

void DnsServer::handleUdpQuery(const std::vector<uint8_t> &data, const SockAddr &src) {
  DnsMessage query;
  if (!decodeMessage(data, query)) {
    return;
  }
  QueryContext ctx;
  ctx.peer_host = src.host;
  ctx.peer_port = src.port;
  ctx.tcp = false;
  ctx.raw = data;
  auto resp = engine_->handle(query, ctx);
  std::size_t limit = kDnsUdpMinSize;
  if (query.edns.present) {
    limit = std::min<std::size_t>(query.edns.udp_size, config_.edns_bufsize);
  }
  bool truncated = false;
  auto wire = encodeMessage(resp, limit, &truncated);
  if (truncated) {
    stats_.truncated.fetch_add(1);
  }
  udp_.sendTo(wire, src);
}

void DnsServer::udpLoop() {
  while (running_) {
    std::vector<uint8_t> data;
    SockAddr src;
    if (!udp_.recvFrom(data, src, 200)) {
      continue;
    }
    if (config_.worker_threads > 1) {
      workers_.emplace_back([this, data, src] { handleUdpQuery(data, src); });
      if (workers_.size() > config_.worker_threads * 4) {
        for (auto &w : workers_) {
          if (w.joinable()) {
            w.join();
          }
        }
        workers_.clear();
      }
    } else {
      handleUdpQuery(data, src);
    }
  }
}

void DnsServer::handleTcp(TcpConnection conn) {
  stats_.tcp_sessions.fetch_add(1);
  const int idle = static_cast<int>(config_.idle_timeout * 1000);
  while (running_ && conn.waitReadable(idle > 0 ? idle : 120000)) {
    std::vector<uint8_t> msg;
    if (!conn.recvDnsMessage(msg, config_.max_packet_size)) {
      break;
    }
    DnsMessage query;
    if (!decodeMessage(msg, query)) {
      break;
    }
    QueryContext ctx;
    ctx.peer_host = conn.peer();
    auto colon = ctx.peer_host.rfind(':');
    if (colon != std::string::npos) {
      ctx.peer_port = static_cast<port_t>(std::stoi(ctx.peer_host.substr(colon + 1)));
      ctx.peer_host = ctx.peer_host.substr(0, colon);
    }
    ctx.tcp = true;
    ctx.raw = msg;
    if (!query.questions.empty() &&
        (query.questions[0].qtype == RrType::Axfr || query.questions[0].qtype == RrType::Ixfr)) {
      auto msgs = engine_->handleAxfr(query, ctx);
      for (const auto &m : msgs) {
        conn.sendDnsMessage(encodeMessage(m));
      }
      break;
    }
    auto resp = engine_->handle(query, ctx);
    if (!conn.sendDnsMessage(encodeMessage(resp))) {
      break;
    }
  }
}

void DnsServer::tcpLoop() {
  while (running_) {
    auto conn = tcp_.acceptConnection(200);
    if (!conn) {
      continue;
    }
    if (config_.max_tcp_sessions > 0 && tcp_sessions_.load() >= config_.max_tcp_sessions) {
      continue;
    }
    tcp_sessions_.fetch_add(1);
    workers_.emplace_back([this, c = std::move(*conn)]() mutable {
      handleTcp(std::move(c));
      tcp_sessions_.fetch_sub(1);
    });
  }
}

void DnsServer::apiLoop() {
  while (running_) {
    auto conn = api_.acceptConnection(200);
    if (!conn) {
      continue;
    }
    handleHttpRequest(*this, *conn);
  }
}

void DnsServer::refreshSecondaries() {
  for (const auto &zone : router_.listZones()) {
    if (zone.kind != ZoneKind::Slave || zone.master.empty()) {
      continue;
    }
    std::string host = zone.master;
    port_t port = 53;
    auto colon = host.find(':');
    if (colon != std::string::npos) {
      port = static_cast<port_t>(std::stoi(host.substr(colon + 1)));
      host = host.substr(0, colon);
    }
    auto tcp = TcpConnection::connectTo(host, port);
    if (!tcp) {
      continue;
    }
    auto query = makeQuery(zone.name, RrType::Axfr, 1, false);
    if (!tcp->sendDnsMessage(encodeMessage(query))) {
      continue;
    }
    std::vector<ResourceRecord> rrs;
    while (true) {
      std::vector<uint8_t> msg;
      if (!tcp->recvDnsMessage(msg, config_.max_packet_size)) {
        break;
      }
      DnsMessage parsed;
      if (!decodeMessage(msg, parsed)) {
        break;
      }
      rrs.insert(rrs.end(), parsed.answers.begin(), parsed.answers.end());
      if (rrs.size() >= 2 && rrs.front().type == RrType::Soa &&
          rrs.back().type == RrType::Soa && rrs.size() > 1) {
        break;
      }
    }
    if (rrs.size() < 2) {
      continue;
    }
    ResourceRecord soa = rrs.front();
    auto tx = router_.begin(zone);
    if (!tx) {
      continue;
    }
    std::vector<ResourceRecord> existing;
    router_.listZone(zone, [&](const ResourceRecord &rr) { existing.push_back(rr); });
    for (const auto &rr : existing) {
      tx->deleteName(rr.name);
    }
    for (std::size_t i = 0; i + 1 < rrs.size(); ++i) {
      tx->addRecord(rrs[i]);
    }
    tx->commit();
    engine_->invalidateCache();
  }
}

void DnsServer::secondaryLoop() {
  while (running_) {
    refreshSecondaries();
    for (int i = 0; i < 50 && running_; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
  }
}

}  // namespace simple_dnsd
