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
| CR-0001..CR-0016 | Repository, governance, toolchain, harness, private-artifact, and review workflow foundation | Project workflow and preserved-source provenance | Reconciled: per-CR records added |
| CR-0017..CR-0021 | Portable core, state, trace-ring, and review-gate foundations | `SPECSIM.ASM`, `Z80.ASM`, architecture sections 11, 36 and 44 | Reconciled: per-CR records added |
| CR-0022..CR-0025 | Z80 state, CPU/bus boundary, fetch/decode and primary opcode table | `Z80.ASM` state, dispatch and timing macros | Reconciled: per-CR records added |
| CR-0026..CR-0034 | CB, ED, DD/FD, DDCB/FDCB, arithmetic, flags, and block operations | `Z80.ASM` prefixed and ALU handlers | Reconciled: per-CR records added |
| CR-0035..CR-0041 | Stack, branches, INC/DEC, HALT, EI/DI, interrupt modes and NMI | `Z80.ASM` stack/branch/refresh/IFF handlers and `SPECSIM.ASM` NMI | Reconciled: per-CR records added |

## CPU CR Reconciliation Queue

| CR | Functional migration slice | Upstream primary and adjacent scan domain | Reconciliation state |
| --- | --- | --- | --- |
| CR-0022..CR-0041 | CPU state, decode, execution, flags, timing, and interrupt behavior | `Z80.ASM` and `SPECSIM.ASM` domains listed above | Reconciled in individual records |

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

CR-0042 has the first contemporaneous compliant pre-development record.
CR-0001 through CR-0041 now have retrospective records that preserve their
actual scope, known corrective history, and explicit deferred behavior without
claiming historical pre-approval. The remaining Phase-2 acceptance requirement
is current implementation and validation evidence, including the active CRs.
