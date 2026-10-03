# Technical debt

- Packet cache is a coarse hashmap; no LRU eviction beyond a full clear
- UDP workers are detached `std::thread` objects joined in batches, not a bounded pool
- SQL is concatenated strings (values are quoted); migrate to bound parameters
- AXFR is a single DNS message; very large zones should be chunked
- Secondary refresh polls on a fixed timer rather than SOA retry/expiry
- IPv6 CIDR matching in allow_axfr is prefix-string based, not proper subnet math
- `--daemon` fork is intentionally unimplemented
