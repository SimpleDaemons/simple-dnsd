# Versioning

simple-dnsd uses [Semantic Versioning](https://semver.org/). Each completed [roadmap](ROADMAP.md) milestone is a **minor** bump on the 0.x series. **1.0.0** is a later hygiene and contract cut after milestone **0.13.0**; it is **not** the current version.

| Component | Meaning |
|-----------|---------|
| **MAJOR** | Incompatible protocol, on-disk schema, or config contract |
| **MINOR** | Backward-compatible features (one roadmap milestone on 0.x) |
| **PATCH** | Fixes and docs inside a milestone |

Git tags and GitHub Releases use the `vMAJOR.MINOR.PATCH` form and point at the commit that finished that version. Tags are not cut from this document; follow [RELEASING.md](RELEASING.md) when publishing.

## Current version

**0.13.0** — service_user privilege drop after bind, GitHub Actions ctest workflow, docs pass. See [CHANGELOG.md](CHANGELOG.md) and [project/ROADMAP_CHECKLIST.md](project/ROADMAP_CHECKLIST.md).

## Milestone map

| Version | Milestone | What it means | Status |
|---------|-----------|---------------|--------|
| **0.1.0** | 1 — Skeleton | Template CMake / GNU Make / packaging / CLI stubs | Done |
| **0.2.0** | 2 — Skeleton realign | ldapd-style CMake/CPack, config parser, tests tree | Done |
| **0.3.0** | 3 — Wire codec | RFC 1035 + EDNS0 + DnsName | Done |
| **0.4.0** | 4 — Authoritative engine | UDP/TCP, memory backend, AA answers | Done |
| **0.5.0** | 5 — Zone files | BIND parser/writer, flat-file backend, export/import | Done |
| **0.6.0** | 6 — SQLite | gsql schema + SQLite | Done |
| **0.7.0** | 7 — PostgreSQL / MySQL | Remote SQL backends, `launch=` router | Done |
| **0.8.0** | 8 — CLI | `simple-dnsutil` zone/record/export commands | Done |
| **0.9.0** | 9 — REST API | `/api/v1/servers/localhost/zones` subset | Done |
| **0.10.0** | 10 — Zone transfers | AXFR, NOTIFY, secondary, TSIG HMAC-SHA256 | Done |
| **0.11.0** | 11 — Performance | Workers, packet cache, RRL, limits, stats | Done |
| **0.12.0** | 12 — Dynamic updates | RFC 2136 UPDATE | Done |
| **0.13.0** | 13 — Hardening | Privilege drop, CI, docs | Current |
| **1.0.0** | Contract cut | Hygiene release after 0.13.0 | Not yet |

## Rules

- Do not retcon a released tag. If 0.2.0 already shipped, a bugfix is **0.2.1**, not another 0.2.0.
- `include/simple-dnsd/version.hpp` (`kVersion`) and `CMakeLists.txt` (`project(... VERSION ...)`) stay in lockstep. The BSD `Makefile` `VERSION` line matches.
- `CHANGELOG.md` has a section per version. Work toward the next milestone lives under **Unreleased** until that version is bumped.
- Each 0.x increment must **configure, compile, and pass `ctest`** on at least one primary platform (macOS or Linux).
- Cutting a published release: update version files and changelog, commit, `git tag -a vX.Y.Z`, push the tag, create the GitHub Release from that tag.

## How to verify a version builds

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/simple-dnsd --version
./build/simple-dnsutil --version
```

Optional CMake flags: `ENABLE_SQLITE`, `ENABLE_POSTGRES`, `ENABLE_MYSQL`, `ENABLE_JSON`, `ENABLE_SSL`. Missing client libraries disable the matching backend at configure time rather than failing the build.

---

## 0.1.0 — Skeleton

**Date:** 2024 (template); version files locked to **0.1.0** for this series.
**Milestone:** 1 — Skeleton.
**What builds:** `simple-dnsd` and `simple-dnsutil` binaries, CMake 3.16+, GNU Make (`GNUmakefile`), CPack packaging layout, `--help` / `--version` / `--config` / `--test-config` entry points.

### Added

- C++17 static library (`simple-dnsd_lib`) and two runtime binaries (`main/simple-dnsd.cpp`, `main/simple-dnsutil.cpp`).
- Cross-platform CMake project: Linux, macOS (deployment target 12.0), Windows, FreeBSD.
- GNU Make wrapper (`Makefile` to `gmake`) and `GNUmakefile` targets (`build`, `test`, `package`, `install`).
- Packaging tree under `packaging/` (DEB, RPM, PKG, FreeBSD, WiX/NSIS/ZIP) and FHS install destinations (`/etc/simple-dnsd`, `/var/lib/simple-dnsd`, `/var/log/simple-dnsd`).
- Service files: systemd unit, sysusers, tmpfiles, logrotate, launchd plist.
- Apache License 2.0, README, ROADMAP, VERSIONING, CHANGELOG, RELEASING, `project/` planning docs.
- Config templates directory (`config/templates/`, `config/examples/`).
- Schema placeholders directory (`schema/`) for later gsql SQL files.
- Version constants in `include/simple-dnsd/version.hpp` (`kVersion`, `kProjectName`, `kDescription`).

### Changed

- Product identity is **authoritative DNS** (not a recursor). Recursion and Docker are out of scope for the 0.x series.

### Notes

- `--daemon` does not fork; stay in the foreground or use the OS supervisor.
- 0.1.0 is the template cut. Feature work starts at 0.2.0.
- Tests: configure-time `ENABLE_TESTS` is on; codec/engine tests land in later minors.

### Files that must compile

- `CMakeLists.txt` `project(simple-dnsd VERSION 0.1.0)`
- `include/simple-dnsd/version.hpp` `kVersion = "0.1.0"`
- `Makefile` `VERSION = 0.1.0`

---

## 0.2.0 — Skeleton realign

**Milestone:** 2 — Match simple-ldapd build/packaging/config conventions.

### Added

- Key/value config parser (`DnsConfig::loadFromFile`) with `validate` / `validateDetailed`.
- Logger (`debug` / `info` / `warning` / `error` / `fatal`) with optional `log_file`.
- UDP/TCP helpers (`utils/net.hpp`) wrapping POSIX and Winsock.
- CMake options: `ENABLE_TESTS`, `ENABLE_PACKAGING`, `ENABLE_SSL`, `ENABLE_JSON`, `ENABLE_SQLITE`, `ENABLE_POSTGRES`, `ENABLE_MYSQL`, `ENABLE_STATIC_LINKING`.
- Unit test `test_dns_config` (load templates, reject empty `launch`).
- Development template on 127.0.0.1:5353; production template on 0.0.0.0:53 with `service_user`.
- Install of configs, schemas, LICENSE, README, CHANGELOG, and `docs/`.

### Config keys introduced

`listen_address`, `dns_port`, `launch`, `log_file`, `log_level`, `foreground`, `service_user`.

### Notes

- No Docker. Packaging follows ldapd: CPack DEB/RPM/PKG/FreeBSD/MSI, FHS, systemd/launchd.
- `CMAKE_INSTALL_PREFIX` defaults to `/usr` on Linux only when the caller did not set it.

---

## 0.3.0 — Wire codec

**Milestone:** 3 — RFC 1035 message codec.

### Added

- `DnsName`: presentation parse, wire encode/decode with RFC 1035 compression pointers, case-insensitive equality, parent/subdomain, wildcard, qualify/relative.
- `DnsMessage` / `DnsHeader` / `Question` / `ResourceRecord`.
- Opcodes: QUERY, NOTIFY, UPDATE. Rcodes including NXDOMAIN, NOTIMP, REFUSED, NOTAUTH, NOTZONE.
- RR types: A, NS, CNAME, SOA, PTR, MX, TXT, AAAA, SRV, NAPTR, OPT, DS, RRSIG, NSEC, DNSKEY, TSIG, IXFR, AXFR, ANY, CAA.
- RFC 3597 unknown-type content via `contentToRdata` / `rdataToContent`.
- EDNS0 OPT pseudo-RR (`edns_bufsize`, default 1232).
- Unit tests: `test_dns_name`, `test_dns_codec`.

### Notes

- TSIG wire type exists; HMAC verification lands in 0.10.0.
- Codec is authoritative-side only (no recursor cache semantics).

---

## 0.4.0 — Authoritative engine

**Milestone:** 4 — Live UDP/TCP server with in-memory zones.

### Added

- `AuthoritativeEngine`: AA answers, NXDOMAIN vs NODATA, in-zone CNAME, wildcards (`*.example.com.`), RFC 8482-style ANY, truncation (`TC`) with TCP fallback.
- Delegations with glue when NS names are in-zone.
- UDP listener and TCP listener bound to the **same** port (TCP uses `udp.boundPort()`).
- In-memory backend (`makeMemoryBackend`) and `Backend` / `Transaction` / `BackendRouter` interfaces.
- `test_dns_engine` (in-process answers) and live path prepared for later integration tests.

### Config keys

`edns_bufsize`.

### Notes

- Default `launch = memory`. Zones do not persist across process restart.
- `dig @127.0.0.1 -p 5353` against a created zone is the acceptance check.

---

## 0.5.0 — Zone files

**Milestone:** 5 — BIND master files as a backend and as export/import.

### Added

- Master-file parser: `$ORIGIN`, `$TTL`, `$INCLUDE`, parentheses, quoted TXT, relative names.
- Canonical writer (`writeZoneText` / `writeZoneFile`).
- `named.conf`-style zone list parser (`parseNamedConf` / `writeNamedConf`).
- Flat-file backend (`makeBindFileBackend`) from `bind_config`.
- `check-zone` / `rectify-zone` helpers; SOA serial bump.
- `exportBind` / `importBind`.
- Unit test `test_dns_zonefile`.

### Config keys

`bind_config`, `export_dir`, `export_named_conf`, `continuous_export`.

---

## 0.6.0 — SQLite (gsql)

**Milestone:** 6 — gsql SQL schema on SQLite.

### Added

- Shared gsql layer (`backend/gsql.hpp`) mapping tables: `domains`, `records`, `domainmetadata`, `cryptokeys`, `tsigkeys`, `comments`.
- SQLite backend (`makeSqliteBackend`); schema file `schema/sqlite.sql`.
- CMake `ENABLE_SQLITE` (default ON); links SQLite3 when found.
- `test_dns_backend` (create zone, add A, lookup, unique temp DB file).

### Config keys

`sqlite_file`, `schema_dir`.

### Notes

- SQL is concatenated quoted strings in this series (see [project/TECHNICAL_DEBT.md](project/TECHNICAL_DEBT.md)); bound parameters are later work.
- `cryptokeys` is stored for compatibility; online DNSSEC signing is out of 0.x.

---

## 0.7.0 — PostgreSQL and MySQL/MariaDB

**Milestone:** 7 — Remote SQL backends and multi-backend `launch=`.

### Added

- PostgreSQL backend (`makePostgresBackend`) via libpq; `schema/pgsql.sql`.
- MySQL/MariaDB backend (`makeMysqlBackend`) via MariaDB Connector/C (or MySQL client); `schema/mysql.sql`.
- `launch = sqlite, bind` router: backends tried in order; longest matching zone wins.
- `backend-copy` plumbing on `BackendRouter::copyZone`.
- CMake `ENABLE_POSTGRES` / `ENABLE_MYSQL`; missing libraries skip the backend instead of failing configure.

### Config keys

`postgres_dsn` (libpq connection string), `mysql_dsn` (`host=;user=;password=;database=;port=`).

---

## 0.8.0 — CLI (`simple-dnsutil`)

**Milestone:** 8 — simple-dnsutil operator tool.

### Added

- Commands: `list-zones`, `list-zone`, `create-zone`, `delete-zone`, `add-record`, `replace-rrset`, `delete-rrset`, `check-zone`, `rectify-zone`, `increase-serial`, `export-bind`, `import-bind`, `backend-copy`.
- `--config` / `--version` / `--help`.
- Continuous BIND export when `continuous_export = true` (rewrite zone files on writes).

---

## 0.9.0 — REST API

**Milestone:** 9 — HTTP API subset.

### Added

- HTTP listener on `api_listen`:`api_port` when `enable_api = true`.
- `X-API-Key` required when `api_key` is set (401 otherwise).
- Endpoints:
  - `GET /api/v1/servers`
  - `GET /api/v1/servers/localhost`
  - `GET /api/v1/servers/localhost/statistics`
  - `GET|POST /api/v1/servers/localhost/zones`
  - `GET|PATCH|DELETE /api/v1/servers/localhost/zones/{zone}`
  - `GET /api/v1/servers/localhost/zones/{zone}/export` (master-file text)
- CMake `ENABLE_JSON` (jsoncpp). Without JSON, mutating endpoints return 501.

### Config keys

`enable_api`, `api_listen`, `api_port`, `api_key`.

---

## 0.10.0 — Zone transfers

**Milestone:** 10 — AXFR, NOTIFY, secondary pull, TSIG.

### Added

- AXFR out (TCP) gated by `allow_axfr` and per-zone allow lists.
- NOTIFY to `also_notify` after SOA serial changes.
- Secondary (`ZoneKind::Slave`) pull of AXFR from `master`.
- TSIG HMAC-SHA256 (`tsig_key_name`, `tsig_key_secret` Base64); `test_dns_tsig`.
- OpenSSL `ENABLE_SSL` for HMAC.

### Config keys

`allow_axfr` (repeatable), `also_notify` (repeatable), `tsig_key_name`, `tsig_key_secret`.

### Notes

- AXFR is a single DNS message in this series; very large zones should be chunked later.
- IXFR is not implemented.

---

## 0.11.0 — Performance

**Milestone:** 11 — Workers, cache, RRL, limits, stats.

### Added

- UDP worker threads (`worker_threads`; `1` is inline).
- Packet cache (`cache_size`; `0` disables) and `cache_ttl`.
- Response rate limiting per client IP (`rrl_rate`; `0` disables).
- `max_tcp_sessions`, `idle_timeout`, `max_packet_size`.
- `ServerStats` counters (queries, answers, nxdomain, refused, axfr, updates, cache hits/misses, truncated, tcp sessions).
- Integration test `test_dns_server` (live UDP/TCP/API on an ephemeral port).

### Config keys

`cache_size`, `cache_ttl`, `rrl_rate`, `max_tcp_sessions`, `idle_timeout`, `max_packet_size`, `worker_threads`.

---

## 0.12.0 — Dynamic updates

**Milestone:** 12 — RFC 2136 UPDATE.

### Added

- Opcode UPDATE: add / delete / replace RRsets against a writable backend.
- TSIG and per-zone `allow_update` gating.
- SOA serial bump on successful update.
- Packet-cache invalidation after writes.

---

## 0.13.0 — Hardening

**Milestone:** 13 — Privilege drop, CI, documentation.

### Added

- `service_user`: drop privileges after bind (Unix).
- GitHub Actions `ctest` workflow (`.github/workflows/ci.yml`) on macOS with OpenSSL, jsoncpp, SQLite.
- Docs pass: README, BUILD_GUIDE, config README, diagrams, project checklists.
- Input limits already applied from 0.11.0 (`max_packet_size`, TCP idle/session caps).

### Notes

- **0.13.0 is the last 0.x milestone.** 1.0.0 is a later contract cut, not this version.
- Known limits that stay: no DNSSEC online signing, no DoT/DoH, no recursion, no `--daemon` fork.

---

## 1.0.0 — Contract cut (not yet)

Hygiene release after 0.13.0: packaging/docs contract, no new protocol features. Do not bump `kVersion` to 1.0.0 until that cut is explicitly requested.

