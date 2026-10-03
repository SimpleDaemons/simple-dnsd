# Simple DNS Daemon — Project Overview

**Project:** `simple-dnsd`  
**Version:** 0.1.0  
**Protocol / role:** Authoritative DNS (not a recursor)  
**Status:** Skeleton. Binaries build; the daemon does not answer queries yet.

## Where to look

| Document | Role |
|----------|------|
| [ROADMAP.md](ROADMAP.md) | Public plan and milestones |
| [project/ROADMAP_CHECKLIST.md](project/ROADMAP_CHECKLIST.md) | Item-level checklists per version |
| [project/PROGRESS_REPORT.md](project/PROGRESS_REPORT.md) | Verified “what works” |
| [project/PROJECT_STATUS.md](project/PROJECT_STATUS.md) | Metrics and health |
| [CHANGELOG.md](CHANGELOG.md) | Release history |
| [VERSIONING.md](VERSIONING.md) | SemVer policy and per-version notes |

## Product line

Production (Apache 2.0) — single-host authoritative DNS. Docker is not used. **1.0.0** is a later contract cut after **0.13.0**.

At 0.1.0: CMake/CPack, `simple-dnsd` / `simple-dnsutil` `--help`/`--version`. Codec, listen loop, backends, and CLI commands are later minors.

## Portfolio

This daemon is part of [SimpleDaemons](https://github.com/SimpleDaemons).

*Last updated: October 2026*
