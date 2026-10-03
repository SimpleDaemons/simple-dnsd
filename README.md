# Simple DNS Daemon

**simple-dnsd** is an **authoritative** DNS server. It is not a recursor.

**Current version:** 0.4.0. See [VERSIONING.md](VERSIONING.md). **1.0.0** is a later hygiene cut.

0.4.0 answers authoritatively over UDP and TCP from the in-memory backend. BIND files, SQL, REST, and zone CLI land in later 0.x minors.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/simple-dnsd --config config/templates/development.conf --foreground
```

Apache License 2.0. Copyright 2026 SimpleDaemons.
