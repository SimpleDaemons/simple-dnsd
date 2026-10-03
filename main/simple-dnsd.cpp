/**
 * @file simple-dnsd.cpp
 * @brief Authoritative DNS daemon entry point (skeleton)
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/utils/platform.hpp"
#include "simple-dnsd/version.hpp"

#include <fstream>
#include <iostream>
#include <string>

namespace {

void printUsage() {
  std::cout
      << "Usage: simple-dnsd [OPTIONS]\n\n"
      << "Options:\n"
      << "  --help, -h           Show this help message\n"
      << "  --version, -v        Show version information\n"
      << "  --config, -c FILE    Configuration file (not parsed in 0.1.0)\n"
      << "  --foreground, -f     Run in the foreground\n"
      << "  --daemon, -d         Run as a daemon (not implemented)\n"
      << "  --test-config        Check that --config FILE is readable and exit\n";
}

void printVersion() {
  std::cout << simple_dnsd::kProjectName << " " << simple_dnsd::kVersion << std::endl;
  std::cout << simple_dnsd::kDescription << std::endl;
  std::cout << "Platform: " << simple_dnsd::platformName() << std::endl;
}

bool fileReadable(const std::string &path) {
  std::ifstream in(path);
  return static_cast<bool>(in);
}

}  // namespace

int main(int argc, char *argv[]) {
  std::string config_path;
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
      if (i + 1 >= argc) {
        std::cerr << "missing configuration file" << std::endl;
        return 1;
      }
      config_path = argv[++i];
    } else if (arg == "--foreground" || arg == "-f") {
      continue;
    } else if (arg == "--daemon" || arg == "-d") {
      std::cerr << "--daemon does not fork; use the OS supervisor" << std::endl;
    } else if (arg == "--test-config") {
      test_config = true;
    } else if (!arg.empty() && arg[0] != '-') {
      std::cerr << "command '" << arg << "' is not implemented in 0.1.0" << std::endl;
      return 2;
    }
  }

  if (test_config) {
    if (config_path.empty()) {
      std::cerr << "--test-config requires --config FILE" << std::endl;
      return 1;
    }
    if (!fileReadable(config_path)) {
      std::cerr << "failed to read configuration: " << config_path << std::endl;
      return 1;
    }
    std::cout << "configuration file is readable (parser arrives in 0.2.0)" << std::endl;
    return 0;
  }

  if (!config_path.empty() && !fileReadable(config_path)) {
    std::cerr << "failed to read configuration: " << config_path << std::endl;
    return 1;
  }

  std::cout << simple_dnsd::kProjectName << " " << simple_dnsd::kVersion
            << " skeleton; DNS listen arrives in 0.4.0" << std::endl;
  return 0;
}
