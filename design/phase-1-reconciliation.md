<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Phase 1 Reconciliation

Status: complete; Phase 2 has not started.

## Implemented Gate

- Portable fixed-width types, machine profiles, master-tick scheduling, canonical
  little-endian state serialization/deserialization, and deterministic hashing.
- Host-independent headless runner and observational core trace sink.
- Exclusive, fixed 16 MiB per-session trace file with a fixed header, compact
  24-byte circular records, generation/write/recoverable-sequence metadata,
  profile and ROM identity, event mask, periodic absolute synchronization flag,
  freeze behavior, and complete-record recovery.
- Standalone `wz_trace_dump` reader and regression coverage for no-clobber
  creation, wrap, eight-frame density, freeze, exact size, and incomplete commit
  rejection.

## Evidence

- Linux x86-64 remote: baseline CMake smoke passed.
- Linux x86-64 remote: GCC and Clang ASan/UBSan smoke runs passed independently.
- Windows 10 and Windows 11 remotes: parser-gated smoke runs passed.
- Hosted matrix: Ubuntu x86-64, Ubuntu AArch64, Windows x86-64, macOS AArch64,
  and macOS x86-64 passed for the Phase-1 implementation.
- Repository gates passed for every Phase-1 checkpoint.

## Reconciliation Passes

Pass 1 found and corrected stale remote checkouts, insufficient trace density,
missing ROM identity/event-mask metadata, missing synchronization flags, a
compiler warning, stale screenshot-harness documentation, and missing
incomplete-record coverage.

Pass 2 checked the architecture Phase-1 boundary, tasks 021-040, workflow rules,
tracker state, public/private directory rules, build targets, harness behavior,
and evidence surfaces. No new Phase-1 gap was found. Phase 2 remains the next
unstarted phase.
