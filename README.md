# Simple DNS Daemon

**simple-dnsd** is an **authoritative** DNS server. It is not a recursor.

**Current version:** 0.13.0 (last 0.x milestone). See [VERSIONING.md](VERSIONING.md) and [project/ROADMAP_CHECKLIST.md](project/ROADMAP_CHECKLIST.md). **1.0.0** is a later hygiene cut, not this version.

At 0.13.0 the tree includes UDP/TCP authoritative answers, backends (memory, BIND files, SQLite, PostgreSQL, MySQL/MariaDB), BIND export/import, `simple-dnsutil`, a REST API subset (`/api/v1/servers/localhost/zones`), AXFR/NOTIFY, TSIG HMAC-SHA256, RFC 2136 UPDATE, packet cache/RRL, and Unix privilege drop. `--daemon` does not fork.

Known limits that stay: no DNSSEC online signing, no DoT/DoH, no recursion, no Docker.

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/simple-dnsd --version
```

Or `make` / `gmake` (GNU Make; see `GNUmakefile`).

Optional CMake flags: `ENABLE_SQLITE`, `ENABLE_POSTGRES`, `ENABLE_MYSQL`, `ENABLE_JSON`, `ENABLE_SSL`.

## Configure

Key/value file, typically `/etc/simple-dnsd/simple-dnsd.conf`. See [config/README.md](config/README.md).

```
listen_address = 0.0.0.0
dns_port = 53
launch = sqlite
sqlite_file = /var/lib/simple-dnsd/zones.sqlite
```

`launch = sqlite, bind` tries backends in order (longest matching zone wins).

## CLI

```bash
simple-dnsutil --config /etc/simple-dnsd/simple-dnsd.conf create-zone example.com
simple-dnsutil --config /etc/simple-dnsd/simple-dnsd.conf add-record example.com www A 192.0.2.1
simple-dnsutil --config /etc/simple-dnsd/simple-dnsd.conf export-bind --out /var/lib/simple-dnsd/bind-export --named-conf named.conf
```

## License

Apache License 2.0. Copyright 2026 SimpleDaemons.
