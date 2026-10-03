# Progress report

**Version:** 0.12.0

UPDATE opcode adds/deletes/replaces RRsets, gated by per-zone `allow_update` or a TSIG key, bumps the SOA serial, and invalidates the packet cache. `test_dns_server` performs a live UPDATE and re-queries the new name.

**1.0.0 is not the current version.**
