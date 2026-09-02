<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# CR-0112 Phase-4A Interrupt Smoke Manifest

## Required Coverage

- Maskable interrupt sampling and acceptance at critical edges.
- NMI acceptance and entry timing.
- Interrupt-line assertion and deassertion ordering.
- Canonical CPU, bus, interrupt, and master-tick trace evidence.
- Headless execution without host presentation dependencies.

## Execution Contract

Run the complete `wz_tests` CTest target and pinned conformance corpus on the
configured remote machine. Preserve traces and logs as private test artifacts;
hosted validation must exercise every available platform label.

## Exit Gate

Close only when remote and hosted evidence passes and the upstream zero-gap scan
confirms that every scoped interrupt behavior is covered or assigned explicitly
to a later task.
