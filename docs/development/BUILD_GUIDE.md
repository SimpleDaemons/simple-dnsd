# Build guide

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

On macOS with MacPorts, point CMake at `/opt/local` if OpenSSL or SQLite are not found:

```bash
cmake -B build -DCMAKE_PREFIX_PATH=/opt/local -DOPENSSL_ROOT_DIR=/opt/local
```

Optional backends: `ENABLE_POSTGRES`, `ENABLE_MYSQL` (on if the client library is found).

Packaging: `make package` (GNU Make). macOS PKG is rebuilt with `packaging/macos/pkg/rebuild-from-cpack.sh`.
