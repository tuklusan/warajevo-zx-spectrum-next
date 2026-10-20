<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Migration and provenance ledger

This ledger follows Core Architecture §§41 and 48. A missing row is not
clearance. An item marked `RESEARCH` is not authorization to translate or copy
the preserved source.

## Source authority and identity

| Field | Recorded value |
|---|---|
| Upstream repository | `https://github.com/tuklusan/warajevo-spectrum-2.50.git` |
| Pinned upstream identity | `94f69bd8f4acb6c0c320ae34f9b1c3ee29bc5545` |
| Imported location | `reference/original-warajevo/source/src/` |
| Import policy | Read-only reference; excluded from all product build targets. |
| Upstream license classification | The official Warajevo source page says the available source is 2.50 and the project was released under GPL in February 2006. Per-file headers and third-party notices still take precedence where present. |
| Project license compatibility | Unresolved repository-wide: the root `LICENSE` is a custom non-commercial license, while several source headers state GPL-2.0-or-later. No compatibility conclusion is implied by a routine row. |

Authority: [official source and license notice](https://worldofspectrum.net/warajevo/Download.html), [official revision history](https://worldofspectrum.net/warajevo/Revision.html). The official history says 2.52 was identical to 2.51 except for the license/source publication, and the official download page says a complete 2.51 source tree was not available.

## Record fields

Each record must identify the upstream repository/commit/blob, source file and
routine, purpose, copyright/license classification, migration type and
clearance, inputs/outputs, emulated and host state effects, timing, external
calls, error behavior, C destination, regression evidence, source comment, and
status. The record's evidence must be pinned to exact source and fixture
identities. `TRANSLATE` requires explicit license-compatible clearance;
uncertain source or license defaults to `RESEARCH` or `EXCLUDE`.

## Routine records

### Z80 NOP — reimplementation sample

| Field | Recorded value |
|---|---|
| Upstream identity | Warajevo 2.50 import from commit `94f69bd8f4acb6c0c320ae34f9b1c3ee29bc5545`; imported Git blob `4555e60aa55e92aa558a7a3adff98895eaaad3c7`; imported-file SHA-256 `20fcdfcd598d7cfbeff5ea01b31390c5874cca96f40e5f5ec418e2acddd8fe5b`. |
| Source routine | `reference/original-warajevo/source/src/spectrum-kernel/Z80.ASM`, label `NOP_`, lines 5207–5211. Its `RF 4` macro accounts for the legacy cycle accumulator and program-counter advance before instruction dispatch. |
| Original purpose | Execute the Z80 no-operation instruction and continue decoding at the following byte. |
| Original copyright/notice | The source file header names Zeljko Juric and Samir Ribic. The official source page classifies the published source as GPL; this row does not clear reuse of other file contents. |
| New material license | `src/core/wz_z80.c` carries a GPL-2.0-or-later source header. Repository-wide compatibility with the root `LICENSE` remains unresolved. |
| Migration type / clearance | `REIMPLEMENT`: the C branch operates on the portable CPU state and advances explicit master ticks; the x86 `RF` macro and register mapping were not translated. Clearance is limited to this routine's standard Z80 behavior and does not authorize copying other assembly. |
| Inputs | Fetched opcode, current program counter, CPU refresh register, and machine master time. |
| Outputs | Program counter advances one byte; refresh state advances through the shared instruction-entry path; master time advances eight half-T-state ticks (four Z80 T-states). |
| Emulated state read/written | Reads decoded opcode and current CPU state; writes PC, refresh register, and master tick. |
| Host state | None. |
| Timing consequences | Four Z80 T-states represented by eight master ticks; no wall-clock or host-width dependency. |
| External calls / error behavior | No NOP-specific external call. Invalid fetch/state errors are returned by the shared instruction-entry path before this branch. |
| C destination | `src/core/wz_z80.c`: `wz_z80_step_impl`, primary opcode decode and `WZ_Z80_PRIMARY_OP_NOP` branch. |
| Regression evidence | Pinned Fuse Z80 conformance passed all 1,356 cases on run `39172660308`, source `2a6e8350388dc69a4c757df7a7f833823c37b5b2`. The `src/core/wz_z80.c` implementation was byte-identical from that tested commit through `efc2cd06921cfd7d7181a734a32583a90d5d4bf3`; only this provenance comment is added by the present change. |
| Provenance comment | Added beside the NOP branch; it points to this ledger and the exact imported source label/revision. |
| Status | This sample is accounted for. The ledger is not a complete inventory of all migrated routines. |

## Explicit gaps

- Routine-level coverage has not yet been completed for the remaining Z80,
  ULA, media, peripheral, machine, and host/UI implementation areas.
- Every source file containing third-party material must be individually
  excluded or cleared before any reuse. The preserved source includes files
  with Borland and TurboPower notices as well as a converter naming a separate
  author.
- Repository-wide license compatibility and missing/legacy notice references
  require owner-level resolution before any claim that the complete project is
  cleared for distribution.
- No entry may be upgraded to `TRANSLATE` based only on matching behavior or
  on the upstream repository's top-level GPL statement.
