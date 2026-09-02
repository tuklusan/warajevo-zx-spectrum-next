<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# CR-0111 Phase-4A Timing Smoke Manifest

## Required Coverage

- Instruction boundary and master-tick progression.
- CPU bus-cycle phase and wait-state timing.
- Canonical timing, CPU, and bus event ordering.
- Trace evidence suitable for first-divergence comparison.
- Headless execution without host presentation dependencies.

## Execution Contract

Run the complete `wz_tests` CTest target and pinned conformance corpus on the
configured remote machine. Preserve returned traces and logs as private test
artifacts. Hosted validation must exercise every available platform label.

## Exit Gate

Close only when executable remote and hosted evidence passes and the upstream
zero-gap scan confirms that every scoped timing behavior is covered or explicitly
assigned to a later task.
