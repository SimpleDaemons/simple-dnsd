# Simple DNS Daemon - Project Status

**Version:** 0.13.0  
**Status:** Last 0.x milestone (hardening). **1.0.0 is not cut.**

## Overview

simple-dnsd is an **authoritative** DNS server (not a recursor) with pluggable backends, BIND export, CLI, REST API, AXFR/NOTIFY, and RFC 2136 updates. Packaging matches simple-ldapd. Docker is not used.

## Completed at 0.13.0

- CMake / GNU Make / CPack (DEB, RPM, PKG, FreeBSD, MSI)
- RFC 1035 codec, EDNS0, TSIG HMAC-SHA256
- UDP/TCP authoritative engine (AA, NXDOMAIN/NODATA, CNAME, wildcards, ANY, truncation)
- Backends: memory, BIND files, SQLite, PostgreSQL, MySQL/MariaDB
- `simple-dnsutil`, REST API, AXFR, NOTIFY, secondary pull, RFC 2136
- Packet cache, RRL, privilege drop, session limits
- `ctest` suite (codec, backends, engine, live server) — 8/8 passing in Release

## Not in 0.x / 1.0

- DNSSEC online signing
- DNS over TLS / HTTPS
- Recursion / forwarding
- Docker
