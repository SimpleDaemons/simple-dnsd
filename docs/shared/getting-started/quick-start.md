# Quick start

**0.2.0** parses config. The daemon does not listen on port 53 yet.

```bash
cmake -B build -DENABLE_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/simple-dnsd --version
./build/simple-dnsd --config config/templates/development.conf --test-config
```

DNS listen arrives in 0.4.0.
