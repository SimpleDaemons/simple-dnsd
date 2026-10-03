# Quick start

```bash
cmake -B build -DENABLE_TESTS=ON
cmake --build build -j
./build/simple-dnsd --config config/templates/development.conf --foreground
```

In another terminal:

```bash
./build/simple-dnsutil --config config/templates/development.conf create-zone example.com
./build/simple-dnsutil --config config/templates/development.conf add-record example.com www A 192.0.2.1
dig @127.0.0.1 -p 5353 www.example.com A
```

Memory backend does not persist across process restarts. Use `launch = sqlite` and `sqlite_file` for durability.
