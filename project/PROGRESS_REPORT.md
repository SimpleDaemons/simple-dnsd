# Progress report

**Version:** 0.13.0  
**Date:** 2026-10-02

0.13.0 is the last 0.x milestone. The daemon answers authoritatively over UDP and TCP, with pluggable backends (memory, BIND files, SQLite, PostgreSQL, MySQL/MariaDB), BIND export, `simple-dnsutil`, a REST API subset (`/api/v1/servers/localhost/zones`), AXFR/NOTIFY, TSIG HMAC-SHA256, RFC 2136 updates, packet cache/RRL, and Unix privilege drop.

Packaging follows simple-ldapd (FHS, CPack, systemd/launchd, no Docker). `--daemon` does not fork.

`ctest` covers codec, zone files, memory/SQLite backends, the engine, TSIG, and a live UDP/TCP/API path. Release builds keep `assert()` in test binaries (`-UNDEBUG`) so setup calls inside asserts still run.

**1.0.0 is not the current version.** It is a later hygiene/contract cut.

Not in 0.x/1.0: DNSSEC signing, DoT/DoH, recursion.
