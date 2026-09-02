Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.

# Phase 3 Reconciliation

Status: ready for gate review

This reconciliation closes the Phase-3 scope defined by architecture section
49.1 and backlog tasks 063-078. It does not claim raster, floating-bus,
audio, media, host/UI, or model-specific peripheral completeness.

## Authority and Upstream Scan

The preserved reference identity is `94f69bd8f4acb6c0c320ae34f9b1c3ee29bc5545`.
The scoped source scan covered:

- `reference/original-warajevo/source/src/spectrum-kernel/SPECSIM.ASM` for
  ULA output, timer/interrupt support, and legacy simulator boundaries.
- `reference/original-warajevo/source/src/spectrum-kernel/Z80.ASM` for
  instruction-level memory, I/O, interrupt-acknowledge, stack, and timing
  phases.
- `design/warajevo-zx-spectrum-next-architecture.md` sections 12-15, 22,
  36, 39, 44, 49.1, and 51.
- `design/machine-timing-evidence.md` and the ordered task ledger.

The source scan found no additional scoped behavior without a disposition.
Legacy host timer/keyboard/DOS mechanisms are not silently imported: they are
replaced by the deterministic core and host-boundary contracts defined by the
architecture.

## Task and Evidence Matrix

| Tasks | Project disposition | Evidence / owner |
| --- | --- | --- |
| 063 | 48K ROM/RAM bus routing implemented | CR-0074, CR-0076, CR-0077; `src/core/wz_machine.c` |
| 064 | Certified external ROM identity implemented | CR-0077; canonical state tests |
| 065 | Partial I/O decode and fixture separation implemented | CR-0078, CR-0079; `src/core/wz_bus.c` |
| 066 | 48K keyboard matrix reads implemented | CR-0080; `src/core/wz_machine.c` |
| 067 | ULA port-FE output latches implemented | CR-0081; bus and state fixtures |
| 068 | Frozen 48K PAL contention implemented | CR-0082; `design/machine-timing-evidence.md` |
| 069 | Contention ownership isolated to bus timing | CR-0083; source-level review |
| 070 | Profile-driven interrupt line scheduling implemented | CR-0084; interrupt trace fixtures |
| 071 | CPU interrupt sampling and edge ordering implemented | CR-0085; interrupt trace fixtures |
| 072 | Timed bus data-source boundary implemented | CR-0086; `wz_bus.c` |
| 073 | Deterministic bus trace classifications implemented | CR-0087; request observer fixtures |
| 074 | Total unmapped/unsupported I/O contract implemented | CR-0088; exhaustive address fixtures |
| 075 | Memory/I/O safety and sanitizer coverage implemented | CR-0089; remote sanitizer evidence |
| 076 | Non-raster differential scenarios implemented | CR-0090; `tests/differential-scenarios.json` |
| 077 | Combined bus/contention/interrupt evidence passed | CR-0091; `tests/phase3-evidence-scenarios.json` |
| 078 | This gate reconciliation | CR-0092 |

## Boundary and Ownership Check

- CPU execution emits typed, intra-instruction bus requests and does not own
  Spectrum contention tables.
- Machine/bus dispatch owns memory routing, I/O routing, contention delay,
  source/direction classification, and observer-visible ordering.
- Profile data owns the 48K PAL frame and interrupt-line timing constants.
- CPU state owns interrupt eligibility, sampling, HALT exit, vector selection,
  and stack effects; the machine line remains an external level.
- Keyboard rows and ULA output latches are canonical machine state and survive
  serialization where required.
- Trace records expose master ticks, bus classifications, contention, and
  interrupt events without host timestamps or pointers.
- Differential results are normalized and bound to the preserved reference;
  hardware-authority disagreements must be classified, not hidden.

## Exit Evidence

- CR-0074 through CR-0090 are closed with their individual review, remote, and
  hosted evidence recorded in `issues/change-requests.json`.
- CR-0091 is closed at `cd7017b`; its Linux R5 run passed build and all five
  CTest targets, and hosted run `33573047207` passed five platform jobs.
- Final CODE and DOCUMENTATION reviews for CR-0091 passed with no confirmed
  findings.
- Repository static gates and publication checks pass on the current branch.

## Deferred Scope

The following are intentionally owned by later tasks and are not Phase-3 gaps:

- Tasks 079-095: timed ULA raster coordinates, fetches, border, floating bus,
  canonical raster, FLASH, and video timing torture.
- Tasks 097-104: mandatory Phase-4A early timing-smoke gate.
- Phase-5 host presentation, host input arbitration, Kempston, pacing, and
  screenshots.
- Phase-6 audio; Phase-7/8 tape and snapshot formats; Phase-9 128K paging/AY;
  Phase-10 Interface-1/Microdrive/ZX Net; and later UI/Telnet work.
- Model-specific peripherals and any real-hardware certification required by
  architecture section 40.

## Zero-Gap Result

Two successive, separately recorded source/document scans covered bus
dispatch, memory and I/O routing, contention tables and boundaries, ULA output,
interrupt assertion and sampling, HALT interaction, frame edges, trace
serialization, canonical state, differential normalization, and each deferred
boundary. No scoped behavior was found without implementation, evidence,
classification, or a named later task.

| Pass | Recorded time (local) | Scope and result |
| --- | --- | --- |
| 1 | 2026-09-01 20:03 | `git status --short` clean; `py -3 tools/validate_project_gates.py` passed; source/task/CR mapping reviewed with no new scoped gap. |
| 2 | 2026-09-01 20:04 | Same complete scope rescanned without repository changes; `git status --short` clean and `py -3 tools/validate_project_gates.py` passed; no new scoped gap. |

The Phase-3 gate is therefore eligible for closure pending its advisory review
and publication gates.
