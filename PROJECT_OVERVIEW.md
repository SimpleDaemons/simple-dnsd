# Simple DNS Daemon — Project Overview

**Project:** `simple-dnsd`  
**Version:** 0.13.0  
**Protocol / role:** Authoritative DNS (not a recursor)  
**Status:** Hardening complete on the 0.x series. **1.0.0 is not cut.**

## Where to look

| Document | Role |
|----------|------|
| [ROADMAP.md](ROADMAP.md) | Public plan and milestones |
| [project/ROADMAP_CHECKLIST.md](project/ROADMAP_CHECKLIST.md) | Item-level checklists per version |
| [project/PROGRESS_REPORT.md](project/PROGRESS_REPORT.md) | Verified “what works” |
| [project/PROJECT_STATUS.md](project/PROJECT_STATUS.md) | Metrics and health |
| [project/FEATURE_AUDIT.md](project/FEATURE_AUDIT.md) | Implemented vs later |
| [project/TECHNICAL_DEBT.md](project/TECHNICAL_DEBT.md) | Known debt |
| [CHANGELOG.md](CHANGELOG.md) | Release history |
| [RELEASING.md](RELEASING.md) | How to cut a release |
| [VERSIONING.md](VERSIONING.md) | SemVer policy and per-version notes |

## Product line

Production (Apache 2.0) — single-host authoritative DNS. Docker is not used. **1.0.0** is a later contract cut after **0.13.0**.

At 0.13.0: UDP/TCP, memory/BIND/SQLite/PostgreSQL/MySQL backends, `simple-dnsutil`, REST API, AXFR/NOTIFY, RFC 2136, cache/RRL, privilege drop.

## Portfolio

This daemon is part of [SimpleDaemons](https://github.com/SimpleDaemons). Portfolio-wide status lives in the monorepo `PROJECTS_OVERVIEW.md`.

*Last updated: October 2026*
