/**
 * @file config.hpp
 * @brief Daemon configuration
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include "simple-dnsd/utils/platform.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace simple_dnsd {

class DnsConfig {
public:
  DnsConfig();

  bool loadFromFile(const std::string &path);
  bool validate() const;
  bool validateDetailed(std::vector<std::string> &errors) const;

  std::vector<std::string> launchBackends() const;

  std::string listen_address{"0.0.0.0"};
  port_t dns_port{kDnsDefaultPort};
  std::string launch{"memory"};
  std::string sqlite_file;
  std::string postgres_dsn;
  std::string mysql_dsn;
  std::string bind_config;
  std::string schema_dir;
  std::string export_dir;
  std::string export_named_conf;
  bool continuous_export{false};
  std::string log_file;
  std::string log_level{"info"};
  std::string api_listen{"127.0.0.1"};
  port_t api_port{8081};
  std::string api_key;
  bool enable_api{false};
  std::uint32_t cache_ttl{0};
  std::uint32_t cache_size{10000};
  std::uint32_t rrl_rate{0};
  std::uint32_t max_tcp_sessions{128};
  std::uint32_t idle_timeout{120};
  std::uint32_t max_packet_size{4096};
  std::uint32_t worker_threads{4};
  std::uint32_t edns_bufsize{static_cast<std::uint32_t>(kDnsEdnsDefaultSize)};
  bool foreground{true};
  std::string service_user;
  std::vector<std::string> allow_axfr;
  std::vector<std::string> also_notify;
  std::string tsig_key_name;
  std::string tsig_key_secret;
};

}  // namespace simple_dnsd
