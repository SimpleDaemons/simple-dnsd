# Simple DNS Daemon - Project Status

**Version:** 0.1.0  
**Status:** Skeleton. **Does not serve DNS yet.**

## Overview

simple-dnsd will be an **authoritative** DNS server (not a recursor). Packaging matches simple-ldapd. Docker is not used.

## Completed at 0.1.0

- CMake / GNU Make / CPack (DEB, RPM, PKG, FreeBSD, MSI)
- `simple-dnsd` and `simple-dnsutil` binaries with `--help` / `--version`
- `--config` / `--test-config` check that a file is readable (no parser yet)
- systemd / launchd install layout

## Next

- 0.2.0 config parser and logger
- 0.3.0 wire codec
- 0.4.0 UDP/TCP listen and memory backend
