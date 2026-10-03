# Configuration files for simple-dnsd

Use `config/templates/` for shipped defaults and `config/examples/` for documented samples.

Key/value syntax (`name = value`). Comments start with `#`.

| Key | Default | Notes |
|-----|---------|-------|
| listen_address | 0.0.0.0 | UDP/TCP bind address |
| dns_port | 53 | Authoritative DNS port (5353 in development) |
| launch | memory | Comma-separated backends: `memory`, `bind`, `sqlite`, `postgres`, `mysql` |
| sqlite_file | | Required when `launch` includes sqlite |
| postgres_dsn | | libpq connection string |
| mysql_dsn | | `host=;user=;password=;database=;port=` |
| bind_config | | `named.conf` listing zone files |
| export_dir | | Directory for continuous BIND export |
| export_named_conf | | `named.conf` include written by export |
| continuous_export | false | Rewrite zone files on every write |
| log_file | | Optional log path |
| log_level | info | `debug`, `info`, `warning`, `error`, `fatal` |
| enable_api | false | HTTP API |
| api_listen | 127.0.0.1 | API bind address |
| api_port | 8081 | API port |
| api_key | | Required when `enable_api` is true (`X-API-Key`) |
| cache_size | 10000 | Packet cache entries; `0` disables |
| rrl_rate | 0 | Responses per second per client IP; `0` disables |
| max_tcp_sessions | 128 | Concurrent TCP clients; `0` unlimited |
| idle_timeout | 120 | TCP idle seconds |
| max_packet_size | 4096 | Max TCP DNS message |
| worker_threads | 4 | UDP worker threads (`1` is inline) |
| edns_bufsize | 1232 | Advertised EDNS UDP size |
| foreground | true | Stay in the foreground |
| service_user | | Drop privileges to this user after bind |
| allow_axfr | | Repeatable. Client IPs allowed to AXFR |
| also_notify | | Repeatable. Extra NOTIFY targets |
| tsig_key_name | | TSIG key name |
| tsig_key_secret | | Base64 HMAC-SHA256 secret |
