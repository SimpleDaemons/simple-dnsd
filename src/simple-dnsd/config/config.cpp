/**
 * @file config.cpp
 * @brief Key=value configuration parser
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/config/config.hpp"
#include "simple-dnsd/utils/logger.hpp"

#include <fstream>

namespace simple_dnsd {

DnsConfig::DnsConfig() = default;

bool DnsConfig::loadFromFile(const std::string &path) {
  std::ifstream in(path);
  if (!in) {
    return false;
  }
  std::string line;
  while (std::getline(in, line)) {
    auto comment = line.find('#');
    if (comment != std::string::npos) {
      line = line.substr(0, comment);
    }
    line = trim(line);
    if (line.empty()) {
      continue;
    }
    auto eq = line.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    const std::string key = trim(line.substr(0, eq));
    const std::string value = trim(line.substr(eq + 1));
    if (key == "listen_address") {
      listen_address = value;
    } else if (key == "dns_port" || key == "port") {
      dns_port = static_cast<port_t>(std::stoi(value));
    } else if (key == "launch" || key == "backend") {
      launch = value;
    } else if (key == "sqlite_file") {
      sqlite_file = value;
    } else if (key == "postgres_dsn") {
      postgres_dsn = value;
    } else if (key == "mysql_dsn") {
      mysql_dsn = value;
    } else if (key == "bind_config" || key == "named_conf") {
      bind_config = value;
    } else if (key == "schema_dir") {
      schema_dir = value;
    } else if (key == "export_dir") {
      export_dir = value;
    } else if (key == "export_named_conf") {
      export_named_conf = value;
    } else if (key == "continuous_export") {
      continuous_export = parseBool(value);
    } else if (key == "log_file") {
      log_file = value;
    } else if (key == "log_level") {
      log_level = value;
    } else if (key == "api_listen") {
      api_listen = value;
    } else if (key == "api_port") {
      api_port = static_cast<port_t>(std::stoi(value));
    } else if (key == "api_key") {
      api_key = value;
    } else if (key == "enable_api") {
      enable_api = parseBool(value);
    } else if (key == "cache_ttl") {
      cache_ttl = static_cast<std::uint32_t>(std::stoul(value));
    } else if (key == "cache_size") {
      cache_size = static_cast<std::uint32_t>(std::stoul(value));
    } else if (key == "rrl_rate") {
      rrl_rate = static_cast<std::uint32_t>(std::stoul(value));
    } else if (key == "max_tcp_sessions") {
      max_tcp_sessions = static_cast<std::uint32_t>(std::stoul(value));
    } else if (key == "idle_timeout") {
      idle_timeout = static_cast<std::uint32_t>(std::stoul(value));
    } else if (key == "max_packet_size") {
      max_packet_size = static_cast<std::uint32_t>(std::stoul(value));
    } else if (key == "worker_threads") {
      worker_threads = static_cast<std::uint32_t>(std::stoul(value));
    } else if (key == "edns_bufsize") {
      edns_bufsize = static_cast<std::uint32_t>(std::stoul(value));
    } else if (key == "foreground") {
      foreground = parseBool(value);
    } else if (key == "service_user") {
      service_user = value;
    } else if (key == "allow_axfr") {
      allow_axfr.push_back(value);
    } else if (key == "also_notify") {
      also_notify.push_back(value);
    } else if (key == "tsig_key_name") {
      tsig_key_name = value;
    } else if (key == "tsig_key_secret") {
      tsig_key_secret = value;
    }
  }
  return true;
}

std::vector<std::string> DnsConfig::launchBackends() const {
  auto parts = split(launch, ',');
  std::vector<std::string> out;
  for (auto &p : parts) {
    p = toLower(trim(p));
    if (!p.empty()) {
      out.push_back(p);
    }
  }
  if (out.empty()) {
    out.emplace_back("memory");
  }
  return out;
}

bool DnsConfig::validate() const {
  std::vector<std::string> errors;
  return validateDetailed(errors);
}

bool DnsConfig::validateDetailed(std::vector<std::string> &errors) const {
  errors.clear();
  if (listen_address.empty()) {
    errors.emplace_back("listen_address is required");
  }
  LogLevel level{};
  if (!parseLogLevel(log_level, level)) {
    errors.emplace_back("invalid log_level");
  }
  const auto backends = launchBackends();
  for (const auto &b : backends) {
    if (b != "memory" && b != "bind" && b != "bindfile" && b != "sqlite" &&
        b != "postgres" && b != "pgsql" && b != "mysql" && b != "mariadb" &&
        b != "gsqlite3" && b != "gpgsql" && b != "gmysql") {
      errors.push_back("unknown backend: " + b);
    }
    if ((b == "sqlite" || b == "gsqlite3") && sqlite_file.empty()) {
      errors.emplace_back("sqlite_file is required for sqlite backend");
    }
    if ((b == "postgres" || b == "pgsql" || b == "gpgsql") && postgres_dsn.empty()) {
      errors.emplace_back("postgres_dsn is required for postgres backend");
    }
    if ((b == "mysql" || b == "mariadb" || b == "gmysql") && mysql_dsn.empty()) {
      errors.emplace_back("mysql_dsn is required for mysql backend");
    }
    if ((b == "bind" || b == "bindfile") && bind_config.empty()) {
      errors.emplace_back("bind_config is required for bind backend");
    }
  }
  if (enable_api && api_key.empty()) {
    errors.emplace_back("api_key is required when enable_api is true");
  }
  if (max_packet_size < 512) {
    errors.emplace_back("max_packet_size must be at least 512");
  }
  return errors.empty();
}

}  // namespace simple_dnsd
