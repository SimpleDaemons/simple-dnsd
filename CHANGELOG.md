# Changelog

All notable changes to simple-dnsd are documented in this file. Versions follow [VERSIONING.md](VERSIONING.md).

## [Unreleased]

Work toward 0.2.0 (config parser, logger, tests).

## [0.1.0] — 2026-10-03

Milestone 1 — Skeleton.

### Added

- C++17 static library and two binaries (`simple-dnsd`, `simple-dnsutil`)
- `--help` / `--version` / `--config` / `--test-config` entry points
- CMake 3.16+, GNU Make, CPack packaging layout
- systemd / launchd / sysusers / tmpfiles / logrotate
- Config templates and empty `schema/` for later SQL backends

### Notes

- This cut does **not** answer DNS queries. Codec 0.3.0, listen loop 0.4.0.
- `--daemon` does not fork.
