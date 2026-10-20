<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Hardware-correctness difference register

This register records verified cases where the new core deliberately does not
preserve a historical Warajevo timing approximation. It is not a complete
routine-by-routine migration or behavior inventory; that remains part of the
open provenance and legacy-compatibility audit.

The preserved Warajevo 2.50 source is the migration baseline. No complete 2.51
or 2.52 source tree is known, and this project has not used an independently
observed 2.51/2.52-only behavior as implementation evidence. See Core
Architecture §2.1.1 for the evidence boundary.

| Legacy behavior | Current behavior | Reason for difference | Existing evidence |
|---|---|---|---|
| `SPECSIM.ASM` declares `EXTRADELAY` as an adjustable multicolor delay and `HACKDELAY` as an automatic delay (`reference/original-warajevo/source/src/spectrum-kernel/SPECSIM.ASM:1001-1002`). `VSETUP` changes `EXTRADELAY` through user key actions (`:6738-6755`). | Raster output follows timed memory writes and ULA fetch positions; software-specific delay is not an input to the raster engine. | A hardware-timed fetch model replaces user-adjusted display timing when the two disagree. | `raster-effects-contract` run `39375065902` verifies timed attribute writes produce distinct multicolor rows and timestamped FE writes produce adjacent-clock border colors. `canonical-core-regression` run `39375065794` covers the current ULA fetch schedule and write/fetch order. |
| The legacy scanline path applies a heuristic correction using attributes, low-RAM/PC state, and `HACKDELAY` (`:7058-7072`); its interrupt path sets `HACKDELAY` to 225 for a recognized program/PC combination (`:7222-7230`). | The core has no title recognition or PC-specific raster offset; scanline values are sampled from timed ULA activity. | Removing application-specific offsets makes output derive from machine state and master time, rather than a compatibility guess. | The same hosted raster-effects and canonical-core proofs above exercise timed fetches without title detection. The architecture's raster and timing contracts define the new source of truth. |

These entries establish two concrete divergences and their behavioral basis.
They do not establish exhaustive Warajevo feature parity, independent physical
hardware correlation, or 2.51/2.52 equivalence. Those remain open under Core
criteria 43, 44, 57, and 62 as applicable.
