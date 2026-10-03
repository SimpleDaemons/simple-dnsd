# Build guide

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/simple-dnsd --version
```

0.1.0 has no SQL or TLS dependencies. Later minors add SQLite/PostgreSQL/MySQL client libraries when those backends land.

Packaging: `make package` (GNU Make).
