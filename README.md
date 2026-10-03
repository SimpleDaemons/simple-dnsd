# Simple DNS Daemon

**simple-dnsd** is an **authoritative** DNS server. It is not a recursor.

**Current version:** 0.3.0 (wire codec). See [VERSIONING.md](VERSIONING.md). **1.0.0** is a later hygiene cut, not this version.

0.3.0 encodes and decodes RFC 1035 messages. It does not listen on port 53 yet. The listen loop arrives in 0.4.0.

Known limits that stay for the 0.x series: no DNSSEC online signing, no DoT/DoH, no recursion, no Docker. `--daemon` does not fork.

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/simple-dnsd --version
./build/simple-dnsutil --version
```

Or `make` / `gmake` (GNU Make; see `GNUmakefile`).

## License

Apache License 2.0. Copyright 2026 SimpleDaemons.
