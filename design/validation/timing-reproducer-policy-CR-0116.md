<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# CR-0116 Timing Reproducer Policy

## Required Evidence

- A failing case records canonical CPU, bus, ULA, and master-tick trace context.
- The first divergence is identified before any implementation change.
- The reproducer is minimal while preserving the observed failure.
- The failure is classified against upstream or remains open as unexplained.
- The fix and regression evidence are separate, ordered artifacts.

## Execution Contract

No timing implementation change may be accepted without its deterministic
reproducer. Remote and hosted validation must execute the resulting regression
suite; local activity is limited to static gates and source inspection.

## Exit Gate

Close only when the policy is enforced by tracked evidence, the remote suite
passes, hosted validation passes across all available labels, and the upstream
zero-gap scan confirms no scoped timing failure lacks a reproducer.
