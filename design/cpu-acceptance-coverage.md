<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Phase-2 CPU Acceptance Coverage

This inventory is the project-owned evidence map required by Phase-2 task 061.
It supplements, and never replaces, the complete pinned Fuse Z80 corpus.
Each listed fixture is deterministic and is executed by `wz_tests` on approved
remote or hosted test platforms.

| Acceptance category | Direct project-owned evidence | Coverage boundary |
| --- | --- | --- |
| Decoder classification | `tests/wz_tests_placeholder.c` primary table checks at lines 227-285, CB checks at 568-595, and ED checks at 603-655 | deterministic classification of all primary, CB, and ED opcodes plus required undocumented status |
| Primary execution and flags | lines 784-992 and 1573-1686 | load/store, I/O, ALU, 16-bit flags, DAA, rotates, CPL, SCF, CCF, and undocumented X/Y sources |
| CB and ED execution | lines 996-1019, 1691-2341 | prefix fetch, rotate/shift, BIT/RES/SET, block operations, I/O, RRD/RLD, pair transfers, RETN/RETI, and repeat paths |
| DD/FD and DDCB/FDCB behavior | lines 1041-1557 | repeated and ignored prefixes, IX/IY substitution, index bytes, signed displacement, indexed-memory/register writeback, ALU, and stack pairs |
| Stack and branch timing | lines 2356-2785 | PUSH/POP/CALL/RET/RST wrap and byte order, conditional paths, JR/JP/DJNZ timing, and bus cycles |
| HALT and interrupt boundaries | lines 2789-3139 | halted refresh behavior, EI delay, DI, IM0 injected primary/CB/ED/DD/FD/RST execution, IM1, IM2, NMI, and acknowledge ordering |
| Canonical state | lines 3160-3190 | serialization, deserialization, hashing, interrupt fields, and MEMPTR preservation |
| Timing trace reconstruction | lines 679-782, 2886-3139, and 3227-3355 | instruction/register synchronization, opcode recovery, bus and interrupt order, fixed-size ring recovery, incomplete-record rejection, and state-delta reconstruction |
| Full CPU conformance corpus | `tools/harness/run_fuse_ed_platform.py` with `tools/harness/fuse-unresolved-baseline.json` | 1,356 pinned Fuse vectors; baseline is empty as of CR-0071 |

## Reference Boundary

The preserved Warajevo Z80 reference dispatches primary, ED, DD, FD, and CB
families from `reference/original-warajevo/source/src/spectrum-kernel/Z80.ASM`.
Its `HALT`, prefix, and interrupt routines are migration/reference inputs for
the fixtures above. Differential scenarios compare compatible registers,
memory, and timing-visible outcomes. A discrepancy is investigated; when
hardware evidence disproves a legacy approximation, hardware behavior wins and
the difference is recorded.

## Deferred Categories

Contention, scheduled frame interrupts, ULA fetch ordering, floating-bus
behavior, memory-map routing, device I/O effects, real-hardware timing
measurement, and full-machine compatibility are not CPU-complete claims. They
remain owned by the named Phase-3 and later tasks in
`design/wzsn-architectures-1-2-developer-tasks.md`.
