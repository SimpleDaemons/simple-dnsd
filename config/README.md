# Configuration files for simple-dnsd

Use `config/templates/` for shipped defaults.

Key/value syntax (`name = value`). Comments start with `#`.

The 0.2.0 daemon parses and validates this file. DNS listen starts in 0.4.0.

| Key | Default | Notes |
|-----|---------|-------|
| listen_address | 0.0.0.0 | Bind address (used from 0.4.0) |
| dns_port | 53 | Authoritative DNS port (5353 in development) |
| launch | memory | Backend list (memory until 0.5.0+) |
| log_file | | Optional log path |
| log_level | info | `debug`, `info`, `warning`, `error`, `fatal` |
| foreground | true | Stay in the foreground |
| service_user | | Drop privileges (used from 0.13.0) |
