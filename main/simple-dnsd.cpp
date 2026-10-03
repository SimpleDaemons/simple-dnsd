/**
 * @file simple-dnsd.cpp
 * @brief Authoritative DNS daemon entry point (config/logger; no listen yet)
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/config/config.hpp"
#include "simple-dnsd/utils/logger.hpp"
#include "simple-dnsd/utils/platform.hpp"
#include "simple-dnsd/version.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace {

void printUsage() {
  std::cout
      << "Usage: simple-dnsd [OPTIONS]\n\n"
      << "Options:\n"
      << "  --help, -h           Show this help message\n"
      << "  --version, -v        Show version information\n"
      << "  --config, -c FILE    Configuration file\n"
      << "  --foreground, -f     Run in the foreground\n"
      << "  --daemon, -d         Run as a daemon (not implemented)\n"
      << "  --test-config        Validate configuration and exit\n";
}

void printVersion() {
  std::cout << simple_dnsd::kProjectName << " " << simple_dnsd::kVersion << std::endl;
  std::cout << simple_dnsd::kDescription << std::endl;
  std::cout << "Platform: " << simple_dnsd::platformName() << std::endl;
}

void applyLogger(const simple_dnsd::DnsConfig &config) {
  simple_dnsd::LogLevel level{};
  if (simple_dnsd::parseLogLevel(config.log_level, level)) {
    simple_dnsd::Logger::instance().setLevel(level);
  }
  if (!config.log_file.empty()) {
    simple_dnsd::Logger::instance().setLogFile(config.log_file);
  }
}

int printConfigErrors(const simple_dnsd::DnsConfig &config) {
  std::vector<std::string> errors;
  if (config.validateDetailed(errors)) {
    return 0;
  }
  for (const auto &err : errors) {
    std::cerr << err << std::endl;
  }
  return 1;
}

}  // namespace

int main(int argc, char *argv[]) {
  simple_dnsd::DnsConfig config;
  bool have_config = false;
  bool test_config = false;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      printUsage();
      return 0;
    }
    if (arg == "--version" || arg == "-v") {
      printVersion();
      return 0;
    }
    if (arg == "--config" || arg == "-c") {
      if (i + 1 >= argc || !config.loadFromFile(argv[++i])) {
        std::cerr << "failed to load configuration" << std::endl;
        return 1;
      }
      have_config = true;
    } else if (arg == "--foreground" || arg == "-f") {
      config.foreground = true;
    } else if (arg == "--daemon" || arg == "-d") {
      std::cerr << "--daemon does not fork; use the OS supervisor" << std::endl;
    } else if (arg == "--test-config") {
      test_config = true;
    } else if (!arg.empty() && arg[0] != '-') {
      std::cerr << "command '" << arg << "' is not implemented in 0.2.0" << std::endl;
      return 2;
    }
  }

  applyLogger(config);

  if (test_config) {
    if (!have_config) {
      std::cerr << "--test-config requires --config FILE" << std::endl;
      return 1;
    }
    if (printConfigErrors(config) != 0) {
      return 1;
    }
    std::cout << "configuration is valid" << std::endl;
    return 0;
  }

  if (have_config && printConfigErrors(config) != 0) {
    return 1;
  }

  simple_dnsd::Logger::instance().info(
      std::string(simple_dnsd::kProjectName) + " " + simple_dnsd::kVersion +
      " config loaded; DNS listen arrives in 0.4.0");
  return 0;
}
