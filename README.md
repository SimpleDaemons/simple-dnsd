# Simple DNS Daemon

**simple-dnsd** is an **authoritative** DNS server. It is not a recursor.

**Current version:** 0.8.0. See [VERSIONING.md](VERSIONING.md).

Backends: memory, BIND files, SQLite, PostgreSQL, MySQL. Manage zones with `simple-dnsutil`. REST API arrives in 0.9.0.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON
cmake --build build -j
./build/simple-dnsd --config config/templates/development.conf --foreground
./build/simple-dnsutil --config config/templates/development.conf create-zone example.com
./build/simple-dnsutil --config config/templates/development.conf add-record example.com www A 192.0.2.1
```

Apache License 2.0.
