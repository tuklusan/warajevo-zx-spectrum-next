<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Phase 2 Reconciliation

Status: complete; Phase 3 is the next architecture phase.

## Gate Evidence

- The complete pinned Fuse Z80 corpus passes: 1,356 of 1,356 cases, zero
  unresolved cases, silent skips, and unexpected failures (CR-0071).
- Project-owned deterministic CPU acceptance coverage maps decoder, execution,
  flags, prefix, stack, branch, HALT, interrupt, state, and trace categories to
  direct fixtures (CR-0072).
- Exact CPU bus-cycle, prefix, interrupt, and state-trace behavior is covered
  by the CPU and timing-trace fixtures.
- The fixed 16 MiB `TIMING_FULL` ring retains at least eight complete 48K PAL
  frames under a 160,000-instruction CPU/bus workload (CR-0073).
- Linux remote and five-platform hosted validation passed for the final Phase-2
  evidence revisions. The local Windows SSH exceptions are infrastructure-only;
  hosted Windows validation passed.

## Boundary Check

The Phase-2 exit covers CPU correctness and diagnostic observability only. 48K
ROM/RAM mapping, contention, scheduled interrupts, ULA fetches, floating bus,
and device I/O behavior remain Phase-3 and later work; none is claimed here.

Two successive reconciliation passes over tasks 041-062, the CPU test suite,
the pinned corpus evidence, trace requirements, and closed CR records found no
new Phase-2 gap.
