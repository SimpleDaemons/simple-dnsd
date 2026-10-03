# Changelog

All notable changes to simple-dnsd are documented in this file. Versions follow [VERSIONING.md](VERSIONING.md).

## [Unreleased]

Work toward 0.4.0 (UDP/TCP engine and memory backend).

## [0.3.0] — 2026-10-03

Milestone 3 — Wire codec.

### Added

- `DnsName` presentation and wire form with RFC 1035 compression
- `DnsMessage` / header / question / RR codec
- A, AAAA, NS, CNAME, SOA, MX, TXT, PTR, SRV, CAA, RFC 3597 unknown types
- EDNS0 OPT (`edns_bufsize`)
- Unit tests `test_dns_name` and `test_dns_codec`

## [0.2.0] — 2026-10-03

Config parser, logger, net helpers, `test_dns_config`.

## [0.1.0] — 2026-10-03

Skeleton: CMake/CPack and `--help` / `--version`.
