/**
 * @file server.hpp
 * @brief UDP/TCP listeners and REST API
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-dnsd/core/engine.hpp"
#include "simple-dnsd/utils/net.hpp"
#include <atomic>
#include <memory>
#include <thread>
#include <vector>

namespace simple_dnsd {

class DnsServer {
public:
  explicit DnsServer(DnsConfig config);
  ~DnsServer();

  bool start();
  void stop();
  bool running() const { return running_; }
  port_t dnsPort() const { return dns_port_; }
  port_t apiPort() const { return api_port_; }
  AuthoritativeEngine &engine() { return *engine_; }
  BackendRouter &router() { return router_; }
  ServerStats &stats() { return stats_; }
  bool testConfig();

private:
  void udpLoop();
  void tcpLoop();
  void apiLoop();
  void secondaryLoop();
  void handleTcp(TcpConnection conn);
  void handleUdpQuery(const std::vector<uint8_t> &data, const SockAddr &src);
  void refreshSecondaries();

  DnsConfig config_;
  BackendRouter router_;
  ServerStats stats_;
  std::unique_ptr<AuthoritativeEngine> engine_;
  UdpSocket udp_;
  TcpListener tcp_;
  TcpListener api_;
  port_t dns_port_{0};
  port_t api_port_{0};
  std::atomic<bool> running_{false};
  std::atomic<uint32_t> tcp_sessions_{0};
  std::thread udp_thread_;
  std::thread tcp_thread_;
  std::thread api_thread_;
  std::thread secondary_thread_;
  std::vector<std::thread> workers_;
};

bool handleHttpRequest(DnsServer &server, TcpConnection &conn);

}  // namespace simple_dnsd
