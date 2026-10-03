/**
 * @file net.cpp
 * @brief UDP/TCP helpers
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/utils/net.hpp"
#include "simple-dnsd/utils/logger.hpp"

#ifndef SIMPLE_DNSD_WINDOWS
#include <netdb.h>
#include <poll.h>
#include <pwd.h>
#include <grp.h>
#endif

#include <cstring>

namespace simple_dnsd {

namespace {

bool pollReadable(socket_t fd, int timeout_ms) {
#ifdef SIMPLE_DNSD_WINDOWS
  WSAPOLLFD pfd{};
  pfd.fd = fd;
  pfd.events = POLLIN;
  return WSAPoll(&pfd, 1, timeout_ms) > 0;
#else
  pollfd pfd{};
  pfd.fd = fd;
  pfd.events = POLLIN;
  return ::poll(&pfd, 1, timeout_ms) > 0;
#endif
}

bool setReuse(socket_t fd) {
  int yes = 1;
#ifdef SIMPLE_DNSD_WINDOWS
  return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&yes),
                    sizeof(yes)) == 0;
#else
  return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == 0;
#endif
}

bool makeAddr(const std::string &address, port_t port, sockaddr_storage &ss, socklen_t &len,
              int &family) {
  std::memset(&ss, 0, sizeof(ss));
  if (address.find(':') != std::string::npos && address.find('.') == std::string::npos) {
    auto *in6 = reinterpret_cast<sockaddr_in6 *>(&ss);
    in6->sin6_family = AF_INET6;
    in6->sin6_port = htons(port);
    if (address == "::" || address == "*") {
      in6->sin6_addr = in6addr_any;
    } else if (inet_pton(AF_INET6, address.c_str(), &in6->sin6_addr) != 1) {
      return false;
    }
    family = AF_INET6;
    len = sizeof(sockaddr_in6);
    return true;
  }
  auto *in = reinterpret_cast<sockaddr_in *>(&ss);
  in->sin_family = AF_INET;
  in->sin_port = htons(port);
  if (address == "0.0.0.0" || address == "*" || address.empty()) {
    in->sin_addr.s_addr = htonl(INADDR_ANY);
  } else if (inet_pton(AF_INET, address.c_str(), &in->sin_addr) != 1) {
    return false;
  }
  family = AF_INET;
  len = sizeof(sockaddr_in);
  return true;
}

SockAddr fromStorage(const sockaddr_storage &ss) {
  SockAddr out;
  char host[128] = {};
  if (ss.ss_family == AF_INET6) {
    const auto *in6 = reinterpret_cast<const sockaddr_in6 *>(&ss);
    inet_ntop(AF_INET6, &in6->sin6_addr, host, sizeof(host));
    out.host = host;
    out.port = ntohs(in6->sin6_port);
    out.ipv6 = true;
  } else {
    const auto *in = reinterpret_cast<const sockaddr_in *>(&ss);
    inet_ntop(AF_INET, &in->sin_addr, host, sizeof(host));
    out.host = host;
    out.port = ntohs(in->sin_port);
  }
  return out;
}

port_t getBoundPort(socket_t fd) {
  sockaddr_storage ss{};
  socklen_t len = sizeof(ss);
  if (getsockname(fd, reinterpret_cast<sockaddr *>(&ss), &len) != 0) {
    return 0;
  }
  if (ss.ss_family == AF_INET6) {
    return ntohs(reinterpret_cast<sockaddr_in6 *>(&ss)->sin6_port);
  }
  return ntohs(reinterpret_cast<sockaddr_in *>(&ss)->sin_port);
}

bool recvAll(socket_t fd, uint8_t *data, size_t size) {
  size_t got = 0;
  while (got < size) {
#ifdef SIMPLE_DNSD_WINDOWS
    const int n = ::recv(fd, reinterpret_cast<char *>(data + got),
                         static_cast<int>(size - got), 0);
#else
    const ssize_t n = ::recv(fd, data + got, size - got, 0);
#endif
    if (n <= 0) {
      return false;
    }
    got += static_cast<size_t>(n);
  }
  return true;
}

bool sendAllBytes(socket_t fd, const uint8_t *data, size_t size) {
  size_t sent = 0;
  while (sent < size) {
#ifdef SIMPLE_DNSD_WINDOWS
    const int n = ::send(fd, reinterpret_cast<const char *>(data + sent),
                         static_cast<int>(size - sent), 0);
#else
    const ssize_t n = ::send(fd, data + sent, size - sent, 0);
#endif
    if (n <= 0) {
      return false;
    }
    sent += static_cast<size_t>(n);
  }
  return true;
}

}  // namespace

bool initializeSockets() {
#ifdef SIMPLE_DNSD_WINDOWS
  WSADATA data{};
  return WSAStartup(MAKEWORD(2, 2), &data) == 0;
#else
  return true;
#endif
}

void shutdownSockets() {
#ifdef SIMPLE_DNSD_WINDOWS
  WSACleanup();
#endif
}

UdpSocket::~UdpSocket() { close(); }

UdpSocket::UdpSocket(UdpSocket &&other) noexcept : fd_(other.fd_) {
  other.fd_ = INVALID_SOCKET_VALUE;
}

UdpSocket &UdpSocket::operator=(UdpSocket &&other) noexcept {
  if (this != &other) {
    close();
    fd_ = other.fd_;
    other.fd_ = INVALID_SOCKET_VALUE;
  }
  return *this;
}

void UdpSocket::close() {
  if (fd_ != INVALID_SOCKET_VALUE) {
    CLOSE_SOCKET(fd_);
    fd_ = INVALID_SOCKET_VALUE;
  }
}

bool UdpSocket::isOpen() const { return fd_ != INVALID_SOCKET_VALUE; }

port_t UdpSocket::boundPort() const {
  if (!isOpen()) {
    return 0;
  }
  return getBoundPort(fd_);
}

bool UdpSocket::bindAddress(const std::string &address, port_t port) {
  close();
  sockaddr_storage ss{};
  socklen_t len = 0;
  int family = AF_INET;
  if (!makeAddr(address, port, ss, len, family)) {
    return false;
  }
  fd_ = ::socket(family, SOCK_DGRAM, 0);
  if (fd_ == INVALID_SOCKET_VALUE) {
    return false;
  }
  setReuse(fd_);
  if (::bind(fd_, reinterpret_cast<sockaddr *>(&ss), len) != 0) {
    close();
    return false;
  }
  return true;
}

bool UdpSocket::sendTo(const std::vector<uint8_t> &data, const SockAddr &dest) {
  sockaddr_storage ss{};
  socklen_t len = 0;
  int family = AF_INET;
  if (!makeAddr(dest.host, dest.port, ss, len, family)) {
    return false;
  }
#ifdef SIMPLE_DNSD_WINDOWS
  const int n = ::sendto(fd_, reinterpret_cast<const char *>(data.data()),
                         static_cast<int>(data.size()), 0, reinterpret_cast<sockaddr *>(&ss),
                         len);
#else
  const ssize_t n = ::sendto(fd_, data.data(), data.size(), 0,
                             reinterpret_cast<sockaddr *>(&ss), len);
#endif
  return n == static_cast<int>(data.size());
}

bool UdpSocket::recvFrom(std::vector<uint8_t> &data, SockAddr &src, int timeout_ms,
                         std::size_t max_size) {
  if (timeout_ms >= 0 && !pollReadable(fd_, timeout_ms)) {
    return false;
  }
  data.assign(max_size, 0);
  sockaddr_storage ss{};
  socklen_t len = sizeof(ss);
#ifdef SIMPLE_DNSD_WINDOWS
  const int n = ::recvfrom(fd_, reinterpret_cast<char *>(data.data()),
                           static_cast<int>(data.size()), 0, reinterpret_cast<sockaddr *>(&ss),
                           &len);
#else
  const ssize_t n = ::recvfrom(fd_, data.data(), data.size(), 0,
                               reinterpret_cast<sockaddr *>(&ss), &len);
#endif
  if (n <= 0) {
    return false;
  }
  data.resize(static_cast<std::size_t>(n));
  src = fromStorage(ss);
  return true;
}

bool UdpSocket::sendOnce(const std::string &host, port_t port, const std::vector<uint8_t> &query,
                         std::vector<uint8_t> &reply, int timeout_ms) {
  UdpSocket sock;
  if (!sock.bindAddress("0.0.0.0", 0)) {
    return false;
  }
  SockAddr dest;
  dest.host = host;
  dest.port = port;
  if (!sock.sendTo(query, dest)) {
    return false;
  }
  SockAddr src;
  return sock.recvFrom(reply, src, timeout_ms);
}

TcpConnection::TcpConnection(socket_t fd, std::string peer) : fd_(fd), peer_(std::move(peer)) {}

TcpConnection::TcpConnection(TcpConnection &&other) noexcept
    : fd_(other.fd_), peer_(std::move(other.peer_)) {
  other.fd_ = INVALID_SOCKET_VALUE;
}

TcpConnection &TcpConnection::operator=(TcpConnection &&other) noexcept {
  if (this != &other) {
    close();
    fd_ = other.fd_;
    peer_ = std::move(other.peer_);
    other.fd_ = INVALID_SOCKET_VALUE;
  }
  return *this;
}

TcpConnection::~TcpConnection() { close(); }

bool TcpConnection::valid() const { return fd_ != INVALID_SOCKET_VALUE; }

void TcpConnection::close() {
  if (fd_ != INVALID_SOCKET_VALUE) {
    CLOSE_SOCKET(fd_);
    fd_ = INVALID_SOCKET_VALUE;
  }
}

bool TcpConnection::sendAll(const std::vector<uint8_t> &data) {
  return valid() && sendAllBytes(fd_, data.data(), data.size());
}

bool TcpConnection::recvExact(uint8_t *data, size_t size) {
  return valid() && recvAll(fd_, data, size);
}

bool TcpConnection::recvDnsMessage(std::vector<uint8_t> &msg, size_t max_size) {
  uint8_t lenbuf[2];
  if (!recvExact(lenbuf, 2)) {
    return false;
  }
  const uint16_t len = static_cast<uint16_t>((lenbuf[0] << 8) | lenbuf[1]);
  if (len == 0 || len > max_size) {
    return false;
  }
  msg.resize(len);
  return recvExact(msg.data(), len);
}

bool TcpConnection::sendDnsMessage(const std::vector<uint8_t> &msg) {
  uint8_t lenbuf[2] = {static_cast<uint8_t>((msg.size() >> 8) & 0xff),
                       static_cast<uint8_t>(msg.size() & 0xff)};
  if (!sendAllBytes(fd_, lenbuf, 2)) {
    return false;
  }
  return sendAll(msg);
}

bool TcpConnection::waitReadable(int timeout_ms) const {
  return valid() && pollReadable(fd_, timeout_ms);
}

std::optional<TcpConnection> TcpConnection::connectTo(const std::string &host, port_t port) {
  sockaddr_storage ss{};
  socklen_t len = 0;
  int family = AF_INET;
  if (!makeAddr(host, port, ss, len, family)) {
    addrinfo hints{};
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    addrinfo *res = nullptr;
    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res) != 0 ||
        res == nullptr) {
      return std::nullopt;
    }
    socket_t fd = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == INVALID_SOCKET_VALUE) {
      freeaddrinfo(res);
      return std::nullopt;
    }
    if (::connect(fd, res->ai_addr, static_cast<socklen_t>(res->ai_addrlen)) != 0) {
      CLOSE_SOCKET(fd);
      freeaddrinfo(res);
      return std::nullopt;
    }
    freeaddrinfo(res);
    return TcpConnection(fd, host + ":" + std::to_string(port));
  }
  socket_t fd = ::socket(family, SOCK_STREAM, 0);
  if (fd == INVALID_SOCKET_VALUE) {
    return std::nullopt;
  }
  if (::connect(fd, reinterpret_cast<sockaddr *>(&ss), len) != 0) {
    CLOSE_SOCKET(fd);
    return std::nullopt;
  }
  return TcpConnection(fd, host + ":" + std::to_string(port));
}

TcpListener::~TcpListener() { close(); }

void TcpListener::close() {
  if (fd_ != INVALID_SOCKET_VALUE) {
    CLOSE_SOCKET(fd_);
    fd_ = INVALID_SOCKET_VALUE;
  }
}

bool TcpListener::isOpen() const { return fd_ != INVALID_SOCKET_VALUE; }

port_t TcpListener::boundPort() const {
  if (!isOpen()) {
    return 0;
  }
  return getBoundPort(fd_);
}

bool TcpListener::bindAndListen(const std::string &address, port_t port) {
  close();
  sockaddr_storage ss{};
  socklen_t len = 0;
  int family = AF_INET;
  if (!makeAddr(address, port, ss, len, family)) {
    return false;
  }
  fd_ = ::socket(family, SOCK_STREAM, 0);
  if (fd_ == INVALID_SOCKET_VALUE) {
    return false;
  }
  setReuse(fd_);
  if (::bind(fd_, reinterpret_cast<sockaddr *>(&ss), len) != 0) {
    close();
    return false;
  }
  if (::listen(fd_, 128) != 0) {
    close();
    return false;
  }
  return true;
}

std::optional<TcpConnection> TcpListener::acceptConnection(int timeout_ms) {
  if (timeout_ms >= 0 && !pollReadable(fd_, timeout_ms)) {
    return std::nullopt;
  }
  sockaddr_storage ss{};
  socklen_t len = sizeof(ss);
  socket_t cfd = ::accept(fd_, reinterpret_cast<sockaddr *>(&ss), &len);
  if (cfd == INVALID_SOCKET_VALUE) {
    return std::nullopt;
  }
  const SockAddr peer = fromStorage(ss);
  return TcpConnection(cfd, peer.host + ":" + std::to_string(peer.port));
}

bool dropPrivileges(const std::string &user) {
  if (user.empty()) {
    return true;
  }
#ifdef SIMPLE_DNSD_WINDOWS
  (void)user;
  return true;
#else
  if (geteuid() != 0) {
    return true;
  }
  struct passwd *pw = getpwnam(user.c_str());
  if (pw == nullptr) {
    Logger::instance().error("unknown service user: " + user);
    return false;
  }
  if (setgid(pw->pw_gid) != 0 || setuid(pw->pw_uid) != 0) {
    Logger::instance().error("failed to drop privileges to " + user);
    return false;
  }
  Logger::instance().info("dropped privileges to " + user);
  return true;
#endif
}

}  // namespace simple_dnsd
