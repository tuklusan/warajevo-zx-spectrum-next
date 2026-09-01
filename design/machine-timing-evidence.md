<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Machine Timing Evidence

This document is the Phase-0 evidence scaffold and Phase-1 timing authority
required by the core/system architecture and the ordered backlog.

## Frozen Phase-1 48K PAL baseline

The following values are frozen for the deterministic Phase-1 core domain:

| Constant | Frozen value | Authority and rationale |
| --- | ---: | --- |
| CPU clock baseline | 3.5 MHz nominal | World of Spectrum 48K technical reference |
| Master tick frequency | 7 MHz derived integer domain | Two master ticks per CPU T-state; this is the project’s exact integer representation of the half-T-state raster relationship |
| Master ticks per CPU T-state | 2 | Derived from the selected master domain |
| CPU T-states per line | 224 | World of Spectrum 48K technical reference |
| Lines per frame | 312 | World of Spectrum 48K technical reference |
| CPU T-states per frame | 69,888 | `224 * 312`, also stated by the reference |
| Raster clocks per line | 448 | Two half-T-state raster positions per CPU T-state |
| Active width / height | 256 / 192 | Spectrum display geometry used by the reference timing description |

Primary evidence:

- https://worldofspectrum.org/faq/reference/48kreference.htm

The reference states that a 48K frame is `(64+192+56)*224 = 69888` T-states,
that each line takes 224 T-states, and that the pixel timing is one pixel per
half T-state. The 7 MHz master domain is therefore an exact project-domain
derivation, not an additional independent hardware clock claim.

## Same-tick ordering baseline

The deterministic scheduler orders events by:

1. ascending `master_tick`;
2. frozen device priority (`CPU`, `ULA`, `BUS`, then external/test events);
3. monotonically increasing insertion sequence for otherwise equal events.

This is an implementation ordering baseline for Phase 1. Hardware-visible
same-edge ordering for CPU bus activity, ULA fetches, contention, and interrupt
edges remains subject to the dedicated Phase 3/4 evidence tables and must not
be inferred solely from C call order.

## Frozen 48K interrupt sampling order

For the project-owned 48K boundary model, an interrupt edge at a sampled
master tick is observed in this order:

1. ULA interrupt-line assertion or deassertion;
2. CPU maskable-interrupt sample decision;
3. CPU interrupt acceptance decision and acknowledge bus cycle, if accepted;
4. subsequent acknowledge, refresh, stack, and vector bus phases.

The line is sampled only at the CPU boundary represented by the explicit
sampling API. A line that deasserts before that boundary is not accepted.
This ordering is an emulation contract for deterministic traces; later ULA
fetch/raster evidence may refine other same-edge device interactions.

## Evidence buckets

| Topic | Current status | Required before |
| --- | --- | --- |
| 48K PAL master clock and master-tick ratio | Frozen for Phase-1 integer domain | Phase 1 complete; revisit only with new evidence |
| Frame geometry and interrupt timing | 48K frame boundary and 32-T-state interrupt pulse frozen | Phase 3 complete; revisit only with new evidence |
| Contention evidence tables | Scaffold created | Phase 3 |
| Floating-bus evidence | Scaffold created | Phase 4 |
| Same-master-tick event ordering | Phase-1 scheduler ordering frozen; hardware visibility details remain open | Phase 3/4 |
| Raster-racing smoke cases | Scaffold created | Phase 4A |

## Frozen 48K PAL contention tables

The 48K reference defines screen-memory contention for addresses
`0x4000..0x7fff` while the ULA is drawing screen data. Relative to the ULA
interrupt, the first contended cycle is T-state `14335`; the repeating delay
pattern is:

| Cycle phase modulo 8 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Delay in T-states | 6 | 5 | 4 | 3 | 2 | 1 | 0 | 0 |

Contention is active during the screen-drawing windows, not while the ULA is
drawing the border. An even port-FE access is explicitly contended because the
ULA supplies its result; an odd port is not ULA-selected. Port accesses use
the reference four-case pattern:

| Port address high byte in `0x40..0x7f` | Address bit 0 | Pattern |
| --- | --- | --- |
| No | 0 | no delay for 1 T-state, then 3 T-states |
| No | 1 | 4 T-states without ULA delay |
| Yes | 0 | one contended cycle, then 3 T-states |
| Yes | 1 | four contended cycles |

Authority: [48K ZX Spectrum Technical Information](https://worldofspectrum.org/faq/reference/48kreference.htm), sections “Contended Memory” and “Contended Input/Output”.

## Initial freeze checklist

1. Record authoritative hardware references for 48K PAL timing. **Complete for
   the Phase-1 baseline.**
2. Freeze the master-tick relationship used by the deterministic core.
   **Complete for the Phase-1 baseline.**
3. Record the same-edge ordering rules required by the scheduler. **Complete
   for the Phase-1 abstraction; hardware-specific visibility remains later
   work.**
4. Attach named evidence for each timing constant before implementation.
   **Complete for the constants listed above.**

## Notes

The frozen values above authorize the implemented timing slices. They do not
certify a particular ULA revision or complete floating-bus, raster-fetch, or
same-edge hardware evidence gates.
