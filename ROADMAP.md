# simple-dnsd roadmap

**Honesty note:** Prefer [project/PROGRESS_REPORT.md](project/PROGRESS_REPORT.md) when docs disagree. Item-level checklists: [project/ROADMAP_CHECKLIST.md](project/ROADMAP_CHECKLIST.md). Versions: [VERSIONING.md](VERSIONING.md). Overview: [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md).

## Milestone 1 — Skeleton — v0.1.0

- Template CMake / Makefile / packaging (superseded by 0.2.0)

## Milestone 2 — Skeleton realign — v0.2.0

- Cross-platform CMake / GNU Make build matching simple-ldapd
- FHS packaging for Linux, macOS, Windows, FreeBSD (no Docker)
- Config parser, `--config` / `--foreground` entry points, tests tree

## Milestone 3 — Wire codec — v0.3.0

- RFC 1035 message codec with name compression
- A, AAAA, NS, CNAME, SOA, MX, TXT, PTR, SRV, CAA, RFC 3597 unknown types
- EDNS0 OPT

## Milestone 4 — Authoritative engine — v0.4.0

- UDP and TCP listeners, AA answers
- NXDOMAIN vs NODATA, delegations with glue, in-zone CNAME, wildcards, RFC 8482 ANY
- Truncation with TCP fallback

## Milestone 5 — Zone files — v0.5.0

- Master-file parser (`$ORIGIN`, `$TTL`, `$INCLUDE`) and canonical writer
- Flat-file backend from a `named.conf` zone list
- `simple-dnsutil export-bind` / `import-bind`

## Milestone 6 — SQLite — v0.6.0

- gsql schema and SQLite backend

## Milestone 7 — PostgreSQL and MySQL — v0.7.0

- PostgreSQL and MySQL/MariaDB gsql backends
- `launch=` multi-backend router

## Milestone 8 — CLI — v0.8.0

- `simple-dnsutil` zone/record commands, `backend-copy`, continuous export setting

## Milestone 9 — REST API — v0.9.0

- `/api/v1/servers/localhost/zones` subset with `X-API-Key`

## Milestone 10 — Zone transfers — v0.10.0

- AXFR out with ACL, NOTIFY, secondary pull, TSIG HMAC-SHA256

## Milestone 11 — Performance — v0.11.0

- Worker threads, packet cache, response rate limiting, session limits, stats

## Milestone 12 — Dynamic updates — v0.12.0

- RFC 2136 UPDATE with TSIG and per-zone allow list

## Milestone 13 — Hardening — v0.13.0

- Privilege drop after bind, input limits, CI `ctest`, docs pass

## 1.0.0 cut — production-usable authoritative DNS

Milestones 2–13 ship on the 0.x series. **1.0.0** is a later hygiene and contract cut after **0.13.0**. Do not treat 1.0.0 as the current version.

### Contract (write this into README / docs / CHANGELOG at tag time)

- Authoritative-only UDP/TCP on port 53
- Backends: memory, BIND zone files, SQLite, PostgreSQL, MySQL/MariaDB
- BIND export/import, `simple-dnsutil`, REST API, AXFR/NOTIFY, RFC 2136
- One process, one host; run under systemd / launchd / a Windows service (`--daemon` does not fork)
- DNSSEC online signing, DoT/DoH, and recursion are out of 1.0

### Do not pull into 1.0

Items under **Later** and **Out of scope**.

## Later (not scheduled)

- DNSSEC online signing (NSEC/NSEC3, keys in `cryptokeys`)
- IXFR, DNS over TLS, DNS over HTTPS
- LMDB backend, Lua records, catalog zones (RFC 9432)

## Out of scope (1.0)

- Recursion and forwarding (separate project)
- Docker
