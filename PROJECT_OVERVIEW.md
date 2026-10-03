# Simple DNS Daemon — Project Overview

**Project:** `simple-dnsd`  
**Version:** 0.2.0  
**Protocol / role:** Authoritative DNS (not a recursor)  
**Status:** Config parser and logger. The daemon does not answer queries yet.

## Where to look

| Document | Role |
|----------|------|
| [ROADMAP.md](ROADMAP.md) | Public plan and milestones |
| [project/ROADMAP_CHECKLIST.md](project/ROADMAP_CHECKLIST.md) | Item-level checklists per version |
| [project/PROGRESS_REPORT.md](project/PROGRESS_REPORT.md) | Verified “what works” |
| [CHANGELOG.md](CHANGELOG.md) | Release history |
| [VERSIONING.md](VERSIONING.md) | SemVer policy and per-version notes |

## Product line

Production (Apache 2.0) — single-host authoritative DNS. Docker is not used. **1.0.0** is a later contract cut after **0.13.0**.

At 0.2.0: CMake/CPack, config parser, logger, net helpers, `test_dns_config`. Codec 0.3.0, listen loop 0.4.0.

*Last updated: October 2026*
