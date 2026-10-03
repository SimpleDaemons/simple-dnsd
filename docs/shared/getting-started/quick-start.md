# Quick start

**0.1.0** is a skeleton. The binaries print version and check that a config file is readable. They do not listen on port 53.

```bash
cmake -B build -DENABLE_TESTS=ON
cmake --build build -j
./build/simple-dnsd --version
./build/simple-dnsd --config config/templates/development.conf --test-config
./build/simple-dnsutil --help
```

DNS listen arrives in 0.4.0. Zone CLI commands arrive in 0.8.0.
