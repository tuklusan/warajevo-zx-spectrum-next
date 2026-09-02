<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# CR-0110 Phase-4 Gate Audit

## Evidence Map

- ULA timing and fetch ordering: `tests/wz_tests_placeholder.c`, remote CTest.
- Raster coordinates and boundaries: `tests/wz_tests_placeholder.c`, remote CTest.
- Border transitions and FLASH phase: `tests/wz_tests_placeholder.c`, remote CTest.
- Floating bus: `tests/wz_tests_placeholder.c`, remote CTest.
- Canonical raster hashes and event traces: `src/core/wz_raster_evidence.c` and remote CTest.
- Pixel-level diagnostics and invalid-state rejection: `src/core/wz_raster_diagnostic.c`, `src/core/wz_raster.c`, and remote CTest.

## Gate Rules

Every scoped criterion must have executable evidence. Host presentation is not
part of the deterministic core, and no criterion may be satisfied by a prose
assertion alone. Any divergence must be classified against upstream or remain
an open defect.

## Exit Criteria

Close only after CODE and DOCUMENTATION review PASS, remote Phase-4 acceptance
passes, hosted validation passes on all available labels, and the final upstream
zero-gap scan passes.
