/**
 * @file simple-dnsutil.cpp
 * @brief simple-dnsutil CLI entry point (skeleton)
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/utils/platform.hpp"
#include "simple-dnsd/version.hpp"

#include <iostream>
#include <string>

namespace {

void printUsage() {
  std::cout
      << "Usage: simple-dnsutil [OPTIONS]\n\n"
      << "Options:\n"
      << "  --help, -h        Show this help message\n"
      << "  --version, -v     Show version information\n\n"
      << "Zone commands arrive in 0.8.0.\n";
}

}  // namespace

int main(int argc, char *argv[]) {
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      printUsage();
      return 0;
    }
    if (arg == "--version" || arg == "-v") {
      std::cout << "simple-dnsutil " << simple_dnsd::kVersion << std::endl;
      std::cout << simple_dnsd::kDescription << std::endl;
      std::cout << "Platform: " << simple_dnsd::platformName() << std::endl;
      return 0;
    }
  }
  printUsage();
  return 0;
}
