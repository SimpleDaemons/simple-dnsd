# Quick start

**0.3.0** encodes DNS messages in process. It does not listen on port 53.

```bash
cmake -B build -DENABLE_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/simple-dnsd --version
```

DNS listen arrives in 0.4.0.
