# simple-dnsd — Roadmap Checklist

**Current version:** 0.11.0  
**Overall progress:** Milestones 1–11. Later milestones are the plan, not implemented.
**Honest assessment:** Prefer [PROGRESS_REPORT.md](PROGRESS_REPORT.md).

**Product line:** Production (Apache 2.0) — single-host authoritative DNS (not a recursor). Docker is out of scope.

Each version must `cmake` configure, compile, and pass `ctest` (see [VERSIONING.md](../VERSIONING.md)).

---

## Milestone 1 — Skeleton — v0.1.0

**Status:** Done

### Build system
- [x] CMake 3.16+ (`project(simple-dnsd VERSION 0.1.0)`)
- [x] `CMAKE_CXX_STANDARD` 17
- [x] GNU Make (`GNUmakefile`) and BSD `Makefile` wrapper (`VERSION = 0.1.0`)
- [x] Static library target `simple-dnsd_lib`
- [x] Binaries `simple-dnsd` and `simple-dnsutil` from `main/`

### Packaging and install
- [x] CPack layout (DEB, RPM, PKG, FreeBSD, WiX/NSIS/ZIP)
- [x] FHS destinations (`/etc/simple-dnsd`, `/var/lib/simple-dnsd`, `/var/log/simple-dnsd`)
- [x] systemd unit, sysusers, tmpfiles, logrotate
- [x] launchd plist
- [x] No Docker

### Entry points and version
- [x] `--help` / `--version` / `--config` / `--foreground` / `--test-config`
- [x] `kVersion` in `include/simple-dnsd/version.hpp` is `0.1.0`
- [x] `--daemon` documented as not forking

### Docs (this cut)
- [x] [PROJECT_OVERVIEW.md](../PROJECT_OVERVIEW.md) at 0.1.0
- [x] [VERSIONING.md](../VERSIONING.md) starts at 0.1.0
- [x] [CHANGELOG.md](../CHANGELOG.md) `[0.1.0]` section
- [x] [RELEASING.md](../RELEASING.md) and [RELEASE_CHECKLIST.md](RELEASE_CHECKLIST.md)

### Build verification
- [x] `cmake -B build -DENABLE_TESTS=ON` configures
- [x] `cmake --build build` compiles
- [x] `./build/simple-dnsd --version` prints `0.1.0`

---

## Milestone 2 — Skeleton realign — v0.2.0

**Status:** Done

### Config and logging
- [x] Key/value `DnsConfig` parser (`name = value`, `#` comments)
- [x] `validate` / `validateDetailed`
- [x] Logger levels: debug, info, warning, error, fatal
- [x] Optional `log_file`

### CMake options
- [x] `ENABLE_TESTS`, `ENABLE_PACKAGING`, `ENABLE_SSL`, `ENABLE_JSON`
- [x] `ENABLE_SQLITE`, `ENABLE_POSTGRES`, `ENABLE_MYSQL`, `ENABLE_STATIC_LINKING`

### Tests and templates
- [x] `test_dns_config`
- [x] `config/templates/development.conf` (127.0.0.1:5353)
- [x] `config/templates/production.conf` (0.0.0.0:53, `service_user`)
- [x] `config/README.md` key table

### Build verification
- [x] Version files are `0.2.0`
- [x] Configure, compile, `ctest` (at least `test_dns_config`)
- [x] `./build/simple-dnsd --version` prints `0.2.0`

---

## Milestone 3 — Wire codec — v0.3.0

**Status:** Done

### Names and messages
- [x] `DnsName` parse, wire encode/decode, compression pointers
- [x] Case-insensitive equality, parent/subdomain, wildcard, qualify/relative
- [x] `DnsMessage` / `DnsHeader` / `Question` / `ResourceRecord`

### Types
- [x] Opcodes QUERY, NOTIFY, UPDATE
- [x] Rcodes including NXDOMAIN, NOTIMP, REFUSED, NOTAUTH, NOTZONE
- [x] A, NS, CNAME, SOA, PTR, MX, TXT, AAAA, SRV, NAPTR, CAA
- [x] OPT (EDNS0), TSIG/AXFR/IXFR/ANY type numbers
- [x] RFC 3597 unknown-type content

### Tests
- [x] `test_dns_name`
- [x] `test_dns_codec`

### Build verification
- [x] Version files are `0.3.0`
- [x] Configure, compile, `ctest`
- [x] `./build/simple-dnsd --version` prints `0.3.0`

---

## Milestone 4 — Authoritative engine — v0.4.0

**Status:** Done

### Engine
- [x] AA answers
- [x] NXDOMAIN vs NODATA
- [x] In-zone CNAME
- [x] Wildcards
- [x] RFC 8482-style ANY
- [x] Truncation (`TC`) with TCP fallback
- [x] Delegations with in-zone glue

### Network and backend
- [x] UDP listener
- [x] TCP listener on the same port as UDP
- [x] Memory backend (`launch = memory`)
- [x] `Backend` / `Transaction` / `BackendRouter` interfaces

### Tests
- [x] `test_dns_engine`

### Build verification
- [x] Version files are `0.4.0`
- [x] Configure, compile, `ctest`
- [x] `./build/simple-dnsd --version` prints `0.4.0`

---

## Milestone 5 — Zone files — v0.5.0

**Status:** Done

### Parser and writer
- [x] `$ORIGIN`, `$TTL`, `$INCLUDE`
- [x] Parentheses, quoted TXT, relative names
- [x] Canonical writer
- [x] `named.conf` zone list parse/write

### Backend and CLI plumbing
- [x] Flat-file backend from `bind_config`
- [x] `check-zone` / `rectify-zone` / SOA serial bump
- [x] `exportBind` / `importBind`

### Tests
- [x] `test_dns_zonefile`

### Build verification
- [x] Version files are `0.5.0`
- [x] Configure, compile, `ctest`
- [x] `./build/simple-dnsd --version` prints `0.5.0`

---

## Milestone 6 — SQLite — v0.6.0

**Status:** Done

### gsql
- [x] gsql tables: domains, records, domainmetadata, cryptokeys, tsigkeys, comments
- [x] `schema/sqlite.sql`
- [x] SQLite backend (`sqlite_file`)
- [x] CMake `ENABLE_SQLITE`

### Tests
- [x] `test_dns_backend` (unique temp DB file)

### Build verification
- [x] Version files are `0.6.0`
- [x] Configure, compile, `ctest`
- [x] `./build/simple-dnsd --version` prints `0.6.0`

---

## Milestone 7 — PostgreSQL / MySQL — v0.7.0

**Status:** Done

### Backends
- [x] PostgreSQL via libpq (`postgres_dsn`, `schema/pgsql.sql`)
- [x] MySQL/MariaDB via Connector/C (`mysql_dsn`, `schema/mysql.sql`)
- [x] Missing client libraries skip the backend at configure time

### Router
- [x] `launch =` comma-separated backends
- [x] Longest matching zone wins
- [x] `BackendRouter::copyZone`

### Build verification
- [x] Version files are `0.7.0`
- [x] Configure, compile, `ctest`
- [x] `./build/simple-dnsd --version` prints `0.7.0`

---

## Milestone 8 — CLI — v0.8.0

**Status:** Done

### simple-dnsutil
- [x] `list-zones` / `list-zone`
- [x] `create-zone` / `delete-zone`
- [x] `add-record` / `replace-rrset` / `delete-rrset`
- [x] `check-zone` / `rectify-zone` / `increase-serial`
- [x] `export-bind` / `import-bind`
- [x] `backend-copy`
- [x] `--config` / `--version` / `--help`
- [x] `continuous_export`

### Build verification
- [x] Version files are `0.8.0`
- [x] Configure, compile, `ctest`
- [x] `./build/simple-dnsutil --version` prints `0.8.0`

---

## Milestone 9 — REST API — v0.9.0

**Status:** Done

### HTTP API
- [x] `enable_api` listener on `api_listen`:`api_port`
- [x] `X-API-Key` (`api_key`)
- [x] `GET /api/v1/servers` and `/servers/localhost`
- [x] `GET /api/v1/servers/localhost/statistics`
- [x] `GET|POST /api/v1/servers/localhost/zones`
- [x] `GET|PATCH|DELETE /api/v1/servers/localhost/zones/{zone}`
- [x] `GET .../zones/{zone}/export`
- [x] jsoncpp via `ENABLE_JSON` (501 without JSON)

### Build verification
- [x] Version files are `0.9.0`
- [x] Configure, compile, `ctest`
- [x] `./build/simple-dnsd --version` prints `0.9.0`

---

## Milestone 10 — Zone transfers — v0.10.0

**Status:** Done

### AXFR / NOTIFY / secondary / TSIG
- [x] AXFR out on TCP with `allow_axfr`
- [x] NOTIFY to `also_notify`
- [x] Secondary pull (`ZoneKind::Slave`, `master`)
- [x] TSIG HMAC-SHA256 (`tsig_key_name`, `tsig_key_secret`)
- [x] `test_dns_tsig`
- [x] OpenSSL `ENABLE_SSL`

### Build verification
- [x] Version files are `0.10.0`
- [x] Configure, compile, `ctest`
- [x] `./build/simple-dnsd --version` prints `0.10.0`

---

## Milestone 11 — Performance — v0.11.0

**Status:** Done

### Runtime
- [x] UDP `worker_threads` (`1` is inline)
- [x] Packet cache (`cache_size`, `cache_ttl`; `0` disables)
- [x] Response rate limiting (`rrl_rate`; `0` disables)
- [x] `max_tcp_sessions`, `idle_timeout`, `max_packet_size`
- [x] `ServerStats` counters

### Tests
- [x] `test_dns_server` (live UDP/TCP/API)

### Build verification
- [x] Version files are `0.11.0`
- [x] Configure, compile, `ctest`
- [x] `./build/simple-dnsd --version` prints `0.11.0`

---

## Milestone 12 — Dynamic updates — v0.12.0

**Status:** Planned

### RFC 2136
- [ ] Opcode UPDATE add / delete / replace
- [ ] TSIG and per-zone `allow_update`
- [ ] SOA serial bump on success
- [ ] Packet-cache invalidation after writes

### Build verification
- [ ] Version files are `0.12.0`
- [ ] Configure, compile, `ctest`
- [ ] `./build/simple-dnsd --version` prints `0.12.0`

---

## Milestone 13 — Hardening — v0.13.0

**Status:** Current

### Hardening
- [ ] `service_user` privilege drop after bind (Unix)
- [ ] GitHub Actions `ctest` workflow (`.github/workflows/ci.yml`)
- [ ] Docs pass (README, BUILD_GUIDE, config README, diagrams, project folder)
- [ ] Input limits from 0.11.0 applied

### Build verification
- [ ] Version files are `0.13.0`
- [ ] Configure, compile, `ctest` (all tests)
- [ ] `./build/simple-dnsd --version` prints `0.13.0`

---

## 1.0.0 — Contract cut

**Status:** Not yet — do not bump `kVersion` to 1.0.0 until this cut is requested.

- [ ] Hygiene/docs/packaging contract after 0.13.0
- [ ] README 1.0 contract section
- [ ] No new protocol features in the cut
