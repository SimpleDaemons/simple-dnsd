/**
 * @file net.hpp
 * @brief UDP/TCP helpers
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-dnsd/utils/platform.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace simple_dnsd {

bool initializeSockets();
void shutdownSockets();

struct SockAddr {
  std::string host{"0.0.0.0"};
  port_t port{0};
  bool ipv6{false};
};

class UdpSocket {
public:
  UdpSocket() = default;
  ~UdpSocket();
  UdpSocket(const UdpSocket &) = delete;
  UdpSocket &operator=(const UdpSocket &) = delete;
  UdpSocket(UdpSocket &&other) noexcept;
  UdpSocket &operator=(UdpSocket &&other) noexcept;

  bool bindAddress(const std::string &address, port_t port);
  void close();
  bool isOpen() const;
  socket_t native() const { return fd_; }
  port_t boundPort() const;

  bool sendTo(const std::vector<uint8_t> &data, const SockAddr &dest);
  bool recvFrom(std::vector<uint8_t> &data, SockAddr &src, int timeout_ms = -1,
                std::size_t max_size = kDnsMaxPacket);

  static bool sendOnce(const std::string &host, port_t port,
                       const std::vector<uint8_t> &query, std::vector<uint8_t> &reply,
                       int timeout_ms = 2000);

private:
  socket_t fd_{INVALID_SOCKET_VALUE};
};

class TcpConnection {
public:
  TcpConnection() = default;
  explicit TcpConnection(socket_t fd, std::string peer = {});
  TcpConnection(TcpConnection &&other) noexcept;
  TcpConnection &operator=(TcpConnection &&other) noexcept;
  ~TcpConnection();
  TcpConnection(const TcpConnection &) = delete;
  TcpConnection &operator=(const TcpConnection &) = delete;

  bool valid() const;
  void close();
  const std::string &peer() const { return peer_; }
  socket_t native() const { return fd_; }

  bool sendAll(const std::vector<uint8_t> &data);
  bool recvExact(uint8_t *data, size_t size);
  bool recvDnsMessage(std::vector<uint8_t> &msg, size_t max_size = kDnsMaxPacket);
  bool sendDnsMessage(const std::vector<uint8_t> &msg);
  bool waitReadable(int timeout_ms) const;

  static std::optional<TcpConnection> connectTo(const std::string &host, port_t port);

private:
  socket_t fd_{INVALID_SOCKET_VALUE};
  std::string peer_;
};

class TcpListener {
public:
  TcpListener() = default;
  ~TcpListener();
  TcpListener(const TcpListener &) = delete;
  TcpListener &operator=(const TcpListener &) = delete;

  bool bindAndListen(const std::string &address, port_t port);
  void close();
  bool isOpen() const;
  socket_t native() const { return fd_; }
  port_t boundPort() const;
  std::optional<TcpConnection> acceptConnection(int timeout_ms = -1);

private:
  socket_t fd_{INVALID_SOCKET_VALUE};
};

bool dropPrivileges(const std::string &user);

}  // namespace simple_dnsd
