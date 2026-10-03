/**
 * @file platform.hpp
 * @brief Platform types and DNS constants
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef SIMPLE_DNSD_WINDOWS
#define SIMPLE_DNSD_WINDOWS
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#if defined(__APPLE__)
#define SIMPLE_DNSD_MACOS
#elif defined(__FreeBSD__)
#define SIMPLE_DNSD_FREEBSD
#elif defined(__linux__)
#define SIMPLE_DNSD_LINUX
#elif defined(__OpenBSD__) || defined(__NetBSD__)
#define SIMPLE_DNSD_BSD
#else
#error "Unsupported platform"
#endif
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace simple_dnsd {

#ifdef SIMPLE_DNSD_WINDOWS
using socket_t = SOCKET;
#define SOCKET_ERROR_CODE WSAGetLastError()
#define CLOSE_SOCKET closesocket
#define INVALID_SOCKET_VALUE INVALID_SOCKET
#else
using socket_t = int;
#define SOCKET_ERROR_CODE errno
#define CLOSE_SOCKET ::close
#define INVALID_SOCKET_VALUE (-1)
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#endif

using port_t = uint16_t;

inline constexpr port_t kDnsDefaultPort = 53;
inline constexpr port_t kDnsDevelopmentPort = 5353;
inline constexpr std::size_t kDnsUdpMinSize = 512;
inline constexpr std::size_t kDnsEdnsDefaultSize = 1232;
inline constexpr std::size_t kDnsMaxPacket = 65535;

std::string platformName();

std::string trim(std::string value);
std::string toLower(std::string value);
std::vector<std::string> split(const std::string &value, char delim);
std::vector<std::string> splitWs(const std::string &value);
bool parseBool(const std::string &value);
std::string join(const std::vector<std::string> &parts, const std::string &sep);

}  // namespace simple_dnsd
