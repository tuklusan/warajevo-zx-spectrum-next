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
| Primary decode and DD/FD prefix chains | [Decode/prefix](../../tests/cpu-decode-prefix.md) checks identity for all primary entries and seven prefix chains with bus/trace checks. [Primary load](../../tests/cpu-primary-load.md) executes 83 register, immediate, indirect, and absolute forms with exact bus/trace. [Exchange/I/O](../../tests/cpu-primary-exchange-io.md) covers the four primary exchanges and both immediate-port I/O forms. | Audit remaining indexed primary forms and aliases against the final opcode inventory. |
| Memory load/store | [Fetch/write](../../tests/cpu-fetch-write.md), [load read](../../tests/cpu-owned-acceptance.md), and [primary load](../../tests/cpu-primary-load.md) cover register, immediate, indirect, and 16-bit transfers. [Indexed load](../../tests/cpu-indexed-load.md) covers DD/FD register, memory, and immediate displacement forms including 16-bit wraparound with exact bus/trace. [Indexed state](../../tests/cpu-indexed-state.md) covers index-half, pair, stack, and branch forms with exact bus/trace. | Audit remaining indexed aliases and arithmetic/memory side effects against the final opcode inventory. |
| 8-bit and 16-bit arithmetic flags | [ALU](../../tests/cpu-alu-flags.md), [wide flags](../../tests/cpu-wide-flags.md), and [special flags](../../tests/cpu-special-flags.md) check results and all flags with independent vectors. [Memory/indexed arithmetic](../../tests/cpu-arithmetic-trace.md) checks 76 ALU and INC/DEC cases with exact bus/trace. [Index-half arithmetic](../../tests/cpu-index-half-arithmetic.md) adds 80 cases. [Wide arithmetic trace](../../tests/cpu-wide-arithmetic-trace.md) checks 360 16-bit cases. | Add exact traces for primary register INC/DEC. |
| CB and indexed CB | [Rotates/shifts](../../tests/cpu-rotate-shift.md) exhaust all values and carry inputs for eight operations across five target modes. [BIT/RES/SET](../../tests/cpu-bit-set-reset.md) checks all values, bits, and carries across register, (HL), and indexed targets with exact bus/trace records. | Add any uncovered indexed target variants identified by the final opcode audit. |
| ED instructions | [Block/repeat](../../tests/cpu-block-repeat.md) checks all 16 block forms and 26 exact bus/trace steps. [Single-port I/O](../../tests/cpu-ed-io.md) covers all IN/OUT targets, including undocumented forms, with flags and exact bus/trace. [16-bit transfer](../../tests/cpu-ed-transfer.md) covers all pairs in both directions with exact bus/trace. [Control](../../tests/cpu-ed-control.md) covers NEG, IM, I/R transfers, RETN/RETI, and nibble rotates with exact bus/trace. | Audit ED aliases and unsupported encodings against the final opcode inventory. |
| Branches and stack | [Branch](../../tests/cpu-branch-trace.md), [condition matrix](../../tests/cpu-condition-matrix.md), and [stack](../../tests/cpu-stack-trace.md) check taken/not-taken conditions, stack order, wraparound, and exact bus/trace. | Check any remaining indirect or indexed branch/stack forms while closing load/prefix coverage. |
| HALT, EI/DI, interrupt entry | [HALT/interrupt](../../tests/cpu-halt-interrupt.md) and [interrupt modes](../../tests/cpu-interrupt-modes.md) check IM 0/1/2, NMI, delayed EI, and exact bus/trace. | Add edge vectors for simultaneous/pending interrupt decisions if not already covered by the machine-level interrupt acceptance. |
| Trace completeness and retention | [Trace replay](../../tests/cpu-trace-replay.md) reconstructs diverse instruction and NMI state from the production ring; [sync](../../tests/cpu-trace-sync.md) checks prolonged HALT; the 8-frame retention proof is pinned separately. | Recheck the integrated Phase-2 gate after the remaining local vectors pass. |

## Remaining opcode-family gaps

The decoder identity test visits every primary, CB, and ED entry, while the
Fuse suite exercises the full published reference set. Neither establishes
independent project-owned state and exact bus/trace assertions for every
execution family. The remaining local gaps are:

| Family | Existing independent evidence | Missing local acceptance |
| --- | --- | --- |
| Primary and indexed arithmetic | Exhaustive immediate ALU flags, edge 16-bit flags, 76 memory/indexed ALU and INC/DEC traces, 80 index-half ALU and INC/DEC traces, and 360 wide-arithmetic traces. | Primary register INC/DEC exact traces. |
| Primary special flags and accumulator rotates | Independent result/flag vectors for DAA, CPL, SCF/CCF, and RLCA/RRCA/RLA/RRA. | Exact bus/trace for these instruction families. |
| CB and indexed CB | Full rotate/shift values in five target modes; BIT/RES/SET values and exact traces in five target modes. | Rotate/shift exact traces; FD indexed memory-only and alternate register-copy targets for both CB groups. |
| ED and prefixes | All ED port, transfer, control, and block/repeat execution families have exact-trace samples; decoder statuses cover all ED entries. Seven prefix-chain executions have exact traces. | Explicit execution samples for undocumented ED no-op encodings and ignored DD/FD prefix combinations beyond the seven chain vectors. |
| Interrupt decisions | HALT, EI delay, DI, IM 0/1/2, and NMI entry have exact traces. | Pending/simultaneous interrupt arbitration edge cases. |

These are acceptance gaps, not claims of product defects. Close them with
project-owned vectors before declaring Tasks 061–062 complete.
