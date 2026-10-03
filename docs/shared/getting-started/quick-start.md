# Quick start

```bash
cmake -B build -DENABLE_TESTS=ON
cmake --build build -j
./build/simple-dnsd --config config/templates/development.conf --foreground
```

Zones in the memory backend do not persist. Load records via a later `simple-dnsutil` (0.8.0) or wait for BIND/SQL backends.
