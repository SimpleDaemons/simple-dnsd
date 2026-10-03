# Changelog

All notable changes to simple-dnsd are documented in this file. Versions follow [VERSIONING.md](VERSIONING.md).

## [Unreleased]

Work toward 0.3.0 (RFC 1035 wire codec).

## [0.2.0] — 2026-10-03

Milestone 2 — Skeleton realign.

### Added

- Key/value `DnsConfig` parser with `validate` / `validateDetailed`
- Logger (`debug` / `info` / `warning` / `error` / `fatal`) and optional `log_file`
- UDP/TCP helpers (`utils/net.hpp`)
- `test_dns_config`
- CMake options `ENABLE_SSL`, `ENABLE_JSON`, `ENABLE_SQLITE`, `ENABLE_POSTGRES`, `ENABLE_MYSQL` (backends still later)

## [0.1.0] — 2026-10-03

Milestone 1 — Skeleton.

### Added

- C++17 static library and two binaries (`simple-dnsd`, `simple-dnsutil`)
- `--help` / `--version` / `--config` / `--test-config` entry points
- CMake 3.16+, GNU Make, CPack packaging layout
