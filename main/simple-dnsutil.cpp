/**
 * @file simple-dnsutil.cpp
 * @brief simple-dnsutil CLI entry point
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#include "simple-dnsd/core/daemon.hpp"

int main(int argc, char *argv[]) { return simple_dnsd::runDnsutil(argc, argv); }
