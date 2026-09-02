<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# CR-0109 Phase-4 Acceptance Manifest

This manifest binds the ordered Phase-4 acceptance task to the existing
deterministic test executable and remote validation evidence.

## Required Coverage

- ULA fetch timing and event ordering.
- Raster coordinate and frame-boundary behavior.
- Timed border transitions.
- FLASH phase derived from emulated state.
- Floating-bus values derived from timed ULA activity.
- Canonical state, raster, and event evidence with no host presentation dependency.

## Execution Contract

Run the complete `wz_tests` CTest target and the pinned conformance corpus on
the configured remote machine. Repeat the hosted platform matrix across all
available labels. Record failures with their first-divergence evidence and do
not classify an unexplained divergence as an acceptance pass.

## Exit Gate

The task may close only when remote and hosted executions pass and the final
upstream-reference zero-gap scan confirms that every required Phase-4 behavior
is covered or explicitly assigned to a later task.
