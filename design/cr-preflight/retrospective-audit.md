<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Retrospective CR Migration Audit

Audit date: 2026-08-22

## Finding

The completed CR tracker records implementation, advisory-review, and test
evidence, but CR-0001 through CR-0041 predate the mandatory migration
pre-development record. There is no durable proof that each of those CRs had a
complete functional-aspect inventory followed by an independent zero-gap
upstream scan. This is an evidence gap, not proof of missing behavior.

## Scope Inventory

| CR range | Completed migration scope | Primary upstream/reference domains | Audit status |
| --- | --- | --- | --- |
| CR-0001..CR-0016 | Repository, governance, toolchain, harness, private-artifact, and review workflow foundation | Project workflow and preserved-source provenance | Evidence gap: reconstruct per-CR inventory |
| CR-0017..CR-0021 | Portable core, state, trace-ring, and review-gate foundations | `SPECSIM.ASM`, `Z80.ASM`, architecture sections 11, 36 and 44 | Evidence gap: reconstruct per-CR inventory |
| CR-0022..CR-0025 | Z80 state, CPU/bus boundary, fetch/decode and primary opcode table | `Z80.ASM` state, dispatch and timing macros | Evidence gap: reconstruct per-CR inventory |
| CR-0026..CR-0034 | CB, ED, DD/FD, DDCB/FDCB, arithmetic, flags, and block operations | `Z80.ASM` prefixed and ALU handlers | Evidence gap: reconstruct per-CR inventory |
| CR-0035..CR-0041 | Stack, branches, INC/DEC, HALT, EI/DI, interrupt modes and NMI | `Z80.ASM` stack/branch/refresh/IFF handlers and `SPECSIM.ASM` NMI | Evidence gap: reconstruct per-CR inventory |

## CPU CR Reconciliation Queue

| CR | Functional migration slice | Upstream primary and adjacent scan domain | Reconciliation state |
| --- | --- | --- | --- |
| CR-0022 | Register, alternate-register, I/R, IFF, IM and HALT state | `Z80.ASM` state block, reset, IFF, IMODE and HSTATE | Pending detailed inventory |
| CR-0023 | CPU-to-bus request interface | `Z80.ASM` memory/I/O macros, interrupt acknowledge and timing calls | Pending detailed inventory |
| CR-0024 | Fetch/decode skeleton | `Z80.ASM` TABLE1, F/FX macros, refresh and dispatch boundaries | Pending detailed inventory |
| CR-0025 | Primary opcode decoding | TABLE1 plus undocumented/NOP alias entries and prefix handoff | Pending detailed inventory |
| CR-0026 | CB decode and execution | TABLE2, rotate/shift handlers, flag side effects and `(HL)` timing | Pending detailed inventory |
| CR-0027 | ED decode and execution | TABLE3, RETN/RETI, IM, I/R transfer and block-operation neighbors | Pending detailed inventory |
| CR-0028 | DD/FD indexed semantics | PREFIXDD/PREFIXFD, IX/IY tables, displacement fetch and prefix overlap | Pending detailed inventory |
| CR-0029 | DDCB/FDCB indexed-bit operations | indexed CB dispatch, memory read/write, copied-register side effects | Pending detailed inventory |
| CR-0030 | 8-bit ALU and flags | ALU handlers, undocumented X/Y flags, carry/half-carry/overflow neighbors | Pending detailed inventory |
| CR-0031 | 16-bit arithmetic flags | ADD/ADC/SBC pair handlers, MEMPTR and timing side effects | Pending detailed inventory |
| CR-0032 | Flag instructions | DAA, CPL, SCF, CCF and neighboring flag-state paths | Pending detailed inventory |
| CR-0033 | Rotate/shift vectors | CB handlers, SLL compatibility behavior and flags | Pending detailed inventory |
| CR-0034 | Block transfer/search/I/O timing | LDI/LDD/LDIR/LDDR, CPI/CPD/CPIR/CPDR, INI/OUTI family and repeat paths | Pending detailed inventory |
| CR-0035 | Stack/subroutine timing | CALL, RET, RST, PUSH, POP, wraparound and conditional timing paths | Pending detailed inventory |
| CR-0036 | Relative/absolute branch timing | DJNZ, JR, JP, conditional paths, MEMPTR and taken/not-taken timing | Pending detailed inventory |
| CR-0037 | Primary INC/DEC family | register and `(HL)` handlers, carry preservation and X/Y flags | Pending detailed inventory |
| CR-0038 | HALT and refresh | HALT loop, held PC, RCLK/R increment, interrupt release and prefix interaction | Pending detailed inventory |
| CR-0040 | EI/DI delay | DI/EI dispatch, IFF state, delayed recognition and HALT boundary | Pending detailed inventory |
| CR-0041 | IM0/IM1/IM2 and NMI entry | interrupt acknowledge, vectoring, IFF2 preservation, HALT release and stack order | Pending detailed inventory |

## Required Reconciliation

1. Create a `design/cr-preflight/CR-NNNN.md` record for every completed CR.
2. Extract functional aspects from the authoritative task, architecture,
   implementation commit, regression tests, and review evidence.
3. Scan upstream by the implementation symbols plus neighboring state, timing,
   error, and user-visible control paths.
4. Record an explicit disposition for every discovered behavior and a second,
   broader zero-gap scan.
5. Open corrective CRs for any missing behavior; do not relabel a gap as
   complete merely because existing tests pass.

## Current Result

CR-0042 has the first compliant pre-development record. Retrospective records
for earlier CRs are mandatory migration-debt work before the corresponding
Phase-2 exit claim can be accepted.
