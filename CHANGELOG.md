# Changelog

## [Unreleased]
Work toward the 1.0.0 contract cut only when requested; no new 0.x features planned.

## [0.13.0] — 2026-10-03

Milestone 13 — Hardening. Last 0.x milestone; **1.0.0 is not cut**.

### Added

- `service_user` privilege drop after bind (Unix)
- GitHub Actions `ctest` workflow
- Docs pass: README, BUILD_GUIDE, config README, project folder

### Notes

- `--daemon` does not fork; use the OS supervisor.
- Not in 0.x: DNSSEC signing, DoT/DoH, recursion, Docker.

## [0.12.0] — 2026-10-03
RFC 2136 UPDATE with allow_update/TSIG, SOA serial bump, cache invalidation, live test.

## [0.11.0] — 2026-10-03
Packet cache wired into query path, RRL, TCP/packet limits, worker threads, ServerStats.

## [0.10.0] — 2026-10-03
AXFR out, NOTIFY to also_notify, secondary pull, TSIG HMAC-SHA256 with test_dns_tsig.

## [0.9.0] — 2026-10-03
REST API subset under `/api/v1/servers/localhost` with `X-API-Key`; live API test.

## [0.8.0] — 2026-10-03
`simple-dnsutil` zone commands.

## [0.7.0] — 2026-10-03
PostgreSQL and MySQL gsql.

## [0.6.0] — 2026-10-03
SQLite gsql.

## [0.5.0] — 2026-10-03
BIND zone files.

## [0.4.0] — 2026-10-03
UDP/TCP engine.

## [0.3.0] — 2026-10-03
RFC 1035 codec.

## [0.2.0] — 2026-10-03
Config, logger, net helpers.

## [0.1.0] — 2026-10-03
Skeleton.
