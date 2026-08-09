<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Migration Ledger

This ledger records how preserved Warajevo material is treated during the new
implementation.

| Legacy source | Upstream identity | Classification | Migration type | Clearance | Replacement target | Regression evidence | Status |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `WARAJEVO.PAS` main environment entry | `upstream` HEAD `94f69bd8f4acb6c0c320ae34f9b1c3ee29bc5545` | Warajevo-derived reference material | Reimplement in portable C | Reference-only until detailed per-routine provenance is recorded | `src/app/` and command-registry bootstrap | Architecture #1 and #2 plus preserved upstream behavior review | Planned |
| `SPECSIM.ASM` machine execution and timing logic | `upstream` HEAD `94f69bd8f4acb6c0c320ae34f9b1c3ee29bc5545` | Warajevo-derived reference material | Reimplement in portable C | Reference-only until timing evidence and per-routine provenance are frozen | `src/core/`, `src/cpu/`, `src/bus/`, `src/video/` | Timing evidence, Fuse tests, project regressions | Planned |

## Required fields

- Legacy source file or routine
- Upstream identity
- Copyright and license classification
- Migration type
- Clearance state
- Replacement target
- Regression evidence
- Current status

The presence of this file does not authorize direct code copying from any
legacy source. Clearance remains explicit and file-by-file.
