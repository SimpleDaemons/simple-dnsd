# Feature audit

**Current version:** 0.13.0. See [ROADMAP_CHECKLIST.md](ROADMAP_CHECKLIST.md).

| Area | Status at 0.13.0 |
|------|------------------|
| CMake / GNU Make / CPack | Implemented |
| Daemon / `simple-dnsutil` | Implemented |
| UDP/TCP 53 (dev 5353) | Implemented |
| Authoritative answers | Implemented |
| Memory backend | Implemented |
| BIND zone-file backend | Implemented |
| SQLite gsql | Implemented |
| PostgreSQL gsql | Compiled when libpq is found |
| MySQL/MariaDB gsql | Compiled when client lib is found |
| BIND export/import | Implemented |
| simple-dnsutil zone commands | Implemented |
| REST API subset | Implemented |
| AXFR / NOTIFY / secondary | Implemented |
| TSIG HMAC-SHA256 | Implemented |
| RFC 2136 UPDATE | Implemented |
| Packet cache / RRL / limits | Implemented |
| Privilege drop | Implemented |
| DNSSEC signing | Not in 0.x / 1.0 |
| DoT / DoH | Not in 0.x / 1.0 |
| Recursion | Out of scope |
