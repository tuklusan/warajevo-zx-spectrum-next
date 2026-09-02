<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# CR-0114 Phase-4A Border and Floating-Bus Manifest

## Required Coverage

- Timed port-FE border transitions across raster samples.
- Floating-bus values derived from actual ULA fetch state.
- Canonical raster, ULA, bus, and master-tick event ordering.
- Hash and first-divergence evidence for mismatches.
- Headless execution without host presentation dependencies.

## Execution Contract

Run the complete `wz_tests` CTest target and pinned conformance corpus on the
configured remote machine. Preserve traces and logs as private test artifacts;
hosted validation must exercise every available platform label.

## Exit Gate

Close only when remote and hosted evidence passes and the upstream zero-gap scan
confirms all scoped border and floating-bus behavior is covered or explicitly
assigned to a later task.
