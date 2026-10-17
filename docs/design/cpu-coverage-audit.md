<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned CPU coverage audit

This inventory is for Tasks 061–062. A passing Fuse vector establishes
reference agreement; a project-owned case additionally needs an independent
expected result and, for the Phase-2 exit gate, exact bus-cycle and
`TIMING_FULL` trace assertions. The checked-in JSON proofs remain local
records after the October 2026 cleanup of GitHub Actions history; their
historical run URLs are no longer retrievable.

| Acceptance category | Current project-owned coverage | Remaining work |
| --- | --- | --- |
| Primary decode and DD/FD prefix chains | [Decode/prefix](../../tests/cpu-decode-prefix.md) checks identity for all primary entries and seven prefix chains with bus/trace checks. | Execute representative primary load, exchange, and I/O forms with independent state and exact bus/trace expectations. |
| Memory load/store | [Fetch/write](../../tests/cpu-fetch-write.md) and [load read](../../tests/cpu-owned-acceptance.md) cover absolute-address read/write. | Cover register, indirect, immediate, and 16-bit load families, including indexed displacement and address wraparound. |
| 8-bit and 16-bit arithmetic flags | [ALU](../../tests/cpu-alu-flags.md), [wide flags](../../tests/cpu-wide-flags.md), and [special flags](../../tests/cpu-special-flags.md) check results and all flags with independent vectors. | Add exact bus/trace assertions for representative memory and indexed arithmetic forms. |
| CB and indexed CB | [Rotates/shifts](../../tests/cpu-rotate-shift.md) exhaust all values and carry inputs for eight operations across five target modes. | Add independent BIT, RES, and SET matrices, including undocumented X/Y behavior and indexed register-copy effects; check exact bus/trace for representative cases. |
| ED instructions | [Block/repeat](../../tests/cpu-block-repeat.md) checks all 16 block forms and 26 exact bus/trace steps. | Add independent control, 16-bit transfer, and single-port I/O cases (including IN flag behavior), with exact bus/trace assertions. |
| Branches and stack | [Branch](../../tests/cpu-branch-trace.md), [condition matrix](../../tests/cpu-condition-matrix.md), and [stack](../../tests/cpu-stack-trace.md) check taken/not-taken conditions, stack order, wraparound, and exact bus/trace. | Check any remaining indirect or indexed branch/stack forms while closing load/prefix coverage. |
| HALT, EI/DI, interrupt entry | [HALT/interrupt](../../tests/cpu-halt-interrupt.md) and [interrupt modes](../../tests/cpu-interrupt-modes.md) check IM 0/1/2, NMI, delayed EI, and exact bus/trace. | Add edge vectors for simultaneous/pending interrupt decisions if not already covered by the machine-level interrupt acceptance. |
| Trace completeness and retention | [Trace replay](../../tests/cpu-trace-replay.md) reconstructs diverse instruction and NMI state from the production ring; [sync](../../tests/cpu-trace-sync.md) checks prolonged HALT; the 8-frame retention proof is pinned separately. | Recheck the integrated Phase-2 gate after the remaining local vectors pass. |

The next implementation target is BIT/RES/SET because the existing local CB
matrix covers only rotate and shift operations. The Phase-2 gate remains open.
