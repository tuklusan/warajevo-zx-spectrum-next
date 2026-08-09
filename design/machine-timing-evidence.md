<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Machine Timing Evidence

This document is the Phase-0 evidence scaffold required by the core/system
architecture and the ordered backlog.

## Evidence buckets

| Topic | Current status | Required before |
| --- | --- | --- |
| 48K PAL master clock and master-tick ratio | Scaffold created | Phase 1 |
| Frame geometry and interrupt timing | Scaffold created | Phase 1 |
| Contention evidence tables | Scaffold created | Phase 3 |
| Floating-bus evidence | Scaffold created | Phase 4 |
| Same-master-tick event ordering | Scaffold created | Phase 1 |
| Raster-racing smoke cases | Scaffold created | Phase 4A |

## Initial freeze checklist

1. Record authoritative hardware references for 48K PAL timing.
2. Freeze the master-tick relationship used by the deterministic core.
3. Record the same-edge ordering rules required by the scheduler.
4. Attach named evidence for each timing constant before implementation.

## Notes

The present repository baseline creates the canonical file and its review
categories. It does not claim that the timing constants are frozen yet.
