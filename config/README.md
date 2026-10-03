# Configuration files for simple-dnsd

Use `config/templates/` for shipped defaults.

Key/value syntax (`name = value`). Comments start with `#`.

The 0.1.0 daemon checks that `--config FILE` is readable. Parsing keys starts in 0.2.0.

| Key | Default | Notes |
|-----|---------|-------|
| listen_address | 0.0.0.0 | Bind address (used from 0.4.0) |
| dns_port | 53 | Authoritative DNS port (5353 in development) |
| foreground | true | Stay in the foreground |
| log_file | | Optional log path (used from 0.2.0) |
| log_level | info | Used from 0.2.0 |
