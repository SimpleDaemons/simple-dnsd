# Changelog

All notable changes to simple-dnsd are documented in this file. Versions follow [VERSIONING.md](VERSIONING.md).

## [Unreleased]

Work toward 0.5.0 (BIND zone files).

## [0.4.0] — 2026-10-03

Milestone 4 — Authoritative engine.

### Added

- UDP and TCP listeners on the same port
- Memory backend and `BackendRouter`
- AA answers, NXDOMAIN vs NODATA, CNAME, wildcards, RFC 8482-style ANY
- Live test `test_dns_server` plus `test_dns_engine` / `test_dns_backend`

## [0.3.0] — 2026-10-03

RFC 1035 codec and DnsName.

## [0.2.0] — 2026-10-03

Config parser, logger, net helpers.

## [0.1.0] — 2026-10-03

Skeleton packaging and CLI stubs.
