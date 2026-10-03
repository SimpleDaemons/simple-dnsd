# Configuration files for simple-dnsd

Key/value syntax (`name = value`). Comments start with `#`.

| Key | Default | Notes |
|-----|---------|-------|
| listen_address | 0.0.0.0 | UDP/TCP bind address |
| dns_port | 53 | Authoritative DNS port (5353 in development) |
| launch | memory | `memory` in 0.4.0; bind/sqlite/postgres/mysql later |
| log_file | | Optional log path |
| log_level | info | `debug`, `info`, `warning`, `error`, `fatal` |
| foreground | true | Stay in the foreground |
| edns_bufsize | 1232 | Advertised EDNS UDP size |
| worker_threads | 4 | UDP workers (`1` is inline) |
| allow_axfr | | Repeatable. Client IPs allowed to AXFR |
| also_notify | | Repeatable. Extra NOTIFY targets |
| tsig_key_name | | TSIG key name |
| tsig_key_secret | | Base64 HMAC-SHA256 secret |
