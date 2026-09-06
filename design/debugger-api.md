<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Shared Debugger API

Status: FROZEN_CONTRACT_FOR_CR-0243

The debugger is an inspection and control facade over the single live
`wz_machine_t`. It never owns a CPU, memory array, scheduler, peripheral, or
canonical state copy. `src/core/wz_debugger.h` is the public contract.

## Inspection

`wz_debugger_snapshot` returns the CPU register state, canonical master tick,
border, networking mode, and paused state from the live machine. Memory reads
delegate to the existing machine memory boundary and are valid for every
address. Block reads are bounded and reject a null destination when length is
non-zero; address wrap is rejected rather than silently reading a different
region.

Inspection is read-only and is valid while the application is running. Trace
views consume existing `wz_trace_event_t` records and never mutate machine
state.

## Mutation Boundary

The access mode distinguishes read-only inspection from paused mutation. Memory
and CPU mutation are permitted only after the live machine is explicitly
paused; later CRs must preserve atomic failure and validate the complete
candidate state before publishing it. No debugger operation may bypass the
machine bus, state validation, serialization, or trace boundaries.

## Execution and Diagnostics

Breakpoint, continue, single-step, trace export, and register/memory editor
workflows are layered onto this contract by later Phase-11 CRs. They use the
existing scheduler/CPU and diagnostic trace interfaces rather than a debugger
executor. Host windows, files, and screen presentation remain application/UI
concerns.

## Reconciliation

This contract is derived from Architecture 1 sections 41, 44, and 49.2, UI
Architecture 2 section 21, the CR-0241 monitor inventory, and the preserved
`SPECMON.ASM` debugger workflows. Any new operation must first update this
contract and its preflight; undocumented direct access is prohibited.
