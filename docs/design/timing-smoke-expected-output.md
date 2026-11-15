<!--
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
-->

# Phase 4A timing-smoke expected output

This is the frozen software expected-output source for the eight early timing
smoke cases named by Architecture #1 §49.  Each row identifies the executable
assertions, the expected observable result, and the tracked proof that records
the result.  A passing row means that the implementation produced the listed
software output; it does not claim correlation with physical hardware.

| Case | Required behavior | Expected output / invariant | Authoritative implementation source | Tracked proof |
| --- | --- | --- | --- | --- |
| TS-01 instruction/bus-cycle timing | CPU instruction and externally visible bus operations carry deterministic master ticks and ordering. | Trace-dump marker `PASS trace dump fields, same-tick ordering, reconstructed CPU state, and border transitions`; the bus-context contract reconstructs CPU, bus, interrupt, ULA, and border records. | `tests/canonical-core-regression.c`; `ci/test/timing-trace-dump/trace-dump-fixture.c`; `ci/test/timing-trace-bus-context/timing-trace-bus-context.c` | `test-results/timing-trace-dump.json` (12 cases); `test-results/timing-trace-bus-context.json` (7 cases) |
| TS-02 interrupt entry/edge timing | Interrupt assertion, sampling, acceptance, vectoring, and stack effects are represented in the same master-tick timeline. | IM 0/1/2 and NMI project-owned contracts pass with exact bus/trace order; the timing trace contains one ordered interrupt assert/sample/accept sequence. | `ci/test/cpu-interrupt-modes/`; `ci/test/cpu-interrupt-priority/`; `ci/test/timing-trace-bus-context/timing-trace-bus-context.c` | `test-results/cpu-interrupt-modes.json`; `test-results/cpu-interrupt-priority.json`; `test-results/timing-trace-bus-context.json` |
| TS-03 contention | 48K and 128K memory/I/O contention derives from profile timing and preserves line/frame cadence. | 48K/128K wait patterns, first-contention position, 228-T-state line cadence, frame wrap, and even-port timing match the frozen contract. | `ci/test/contention-contract/contention-contract.c`; `src/core/wz_bus.c`; `src/core/wz_machine_profile.c` | `test-results/contention-contract.json` |
| TS-04 border transition timing | Port-FE writes change only the border samples at their timestamped raster positions. | Canonical markers `PASS border_event_timing` and `PASS full_border_raster_projection`; 448x312 output has the expected left/right/top/bottom border spans and no active-display overwrite. | `tests/canonical-core-regression.c` (`verify_border_event_timing`, `verify_full_border_raster_projection`); `ci/test/timing-trace-dump/trace-dump-fixture.c` | `test-results/canonical-core-regression.json`; `test-results/timing-trace-dump.json` |
| TS-05 ULA fetch versus CPU write | Bitmap/attribute fetches observe the frozen same-tick CPU-before-ULA rule. | Canonical marker `PASS raster_write_fetch_order`; first cell remains bitmap/attribute `1`, next cell reflects same-tick writes (`0`, `2`); full-frame proof recovers all 12,288 timed fetches. | `tests/canonical-core-regression.c` (`verify_raster_write_fetch_order`); `ci/test/ula-fetch-trace/ula-fetch-trace-fixture.c`; `ci/test/ula-full-frame-trace/ula-full-frame-trace-fixture.c` | `test-results/ula-fetch-trace.json` (28 checks); `test-results/ula-full-frame-trace.json` (24 checks) |
| TS-06 floating bus | Floating-bus reads expose the byte most recently placed on the timed ULA fetch bus and return the idle value between fetch boundaries. | Canonical marker `PASS ula_fetch_schedule`; first fetch returns bitmap `A5`, attribute fetch returns `16`, and a non-boundary master tick returns `FF`; one-frame address/value checks cover every fetch cell. | `tests/canonical-core-regression.c` (`verify_ula_fetch_schedule`); `src/core/wz_machine.c` | `test-results/ula-fetch-trace.json`; `test-results/ula-full-frame-trace.json` |
| TS-07 raster-racing/overscan | Timed writes immediately before raster fetches and adjacent border clocks are rendered without application-specific detection. | Raster-effects marker `PASS timed_attribute_multicolor_and_border_rainbow`; eight scanline attribute writes appear at the following active samples and adjacent border clocks render `11`, `12`, `13`; full projection remains 448x312. | `ci/test/raster-effects-contract/raster-effects-contract.c`; `tests/canonical-core-regression.c` (`verify_full_border_raster_projection`) | `test-results/raster-effects-contract.json`; `test-results/canonical-core-regression.json` |
| TS-08 multicolor timing | Attribute/bitmap writes at exact ULA positions change the sampled cell color, including BRIGHT semantics, without presentation-layer substitution. | Raster-effects marker above; canonical marker `PASS flash_bright_semantics`; rows map to the eight frozen attributes and BRIGHT/FLASH transitions occur only at the frame phase boundaries. | `ci/test/raster-effects-contract/raster-effects-contract.c`; `tests/canonical-core-regression.c` (`verify_flash_bright_semantics`, `verify_ay_envelope_noise_and_mixer`) | `test-results/raster-effects-contract.json`; `test-results/canonical-core-regression.json` |

## Frozen interpretation

The expected values are implementation-level outputs of the named contracts and
their source fixtures.  The corpus intentionally uses deterministic synthetic
memory/register writes so it remains headless, reproducible, and independent of
host wall-clock behavior.  Fuse remains a software comparison reference; no row
is a physical-hardware or analog-audio certification.  Historical NIRVANA and
other private-media torture cases belong to Phase 14 and are not silently
promoted into this Phase 4A smoke gate.

