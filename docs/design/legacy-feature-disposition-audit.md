<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Legacy feature disposition audit

Task 452 is in progress. Section 2.5 contains 32 explicit dispositions: 20 REQUIRED and 12 deferred/replaced/non-initial. Every REQUIRED row is mapped to its phase implementation area or source and current proof gap. 9 have limited runner evidence; 11 lack criterion-level pinned proof. Source presence alone is not acceptance evidence. The tracked test ledger currently has only canonical-core-regression; the DIZZY4K runner smoke (run 35890238325) covers only the narrow flows identified below.

All deferred/replaced/non-initial rows remain scoped as stated in Section 2.5 and are not promoted into initial release blockers. Historical UI replacements are reconciled separately under task 453. Architecture-#3 implementation remains out of scope.

| Feature | Section 2.5 disposition | Audit | Implementation reference | Evidence or remaining work |
|---|---|---|---|---|
| ZX Spectrum 48K PAL | REQUIRED - initial certified machine | PARTIAL | src/core/wz_machine_profile.c | Four-runner build and DIZZY4K launch/screenshot cover a 48K path, not the complete certified timing suite (runs `35896075757`, `35890238325`). |
| ZX Spectrum 128K PAL | REQUIRED - initial certified machine | OPEN | src/core/wz_machine_profile.c | 128K profile/build exists; no dedicated runtime paging/interrupt/raster proof is pinned. |
| ZX Spectrum +2 | LATER compatibility target; not an initial blocker | SCOPED | — | §2.5 disposition is explicit; keep outside initial implementation/test closure. |
| Timex Sinclair 2068 | LATER compatibility target; preserved source retained | SCOPED | — | §2.5 disposition is explicit; keep outside initial implementation/test closure. |
| DCK/Timex memory expansions | LATER with Timex support | SCOPED | — | §2.5 disposition is explicit; keep outside initial implementation/test closure. |
| Z80 CPU incl. documented/undocumented | REQUIRED | PARTIAL | src/core/wz_z80.c | CPU canonical fingerprint is cross-runner stable; full pinned Fuse coverage and every documented/undocumented opcode remain unproven (runs `35895696032`, `35894160451`). |
| ULA/border/contention/floating bus | REQUIRED | PARTIAL | src/core/wz_bus.c; src/core/wz_raster.c | Canonical raster fingerprint and GUI output pass; timing-smoke, border/contention/floating-bus acceptance is unproven (runs `35895696032`, `35890238325`). |
| 48K keyboard matrix | REQUIRED | PARTIAL | src/core/wz_keyboard_matrix.c | Runner demo uses physical-key commands to start the tape; matrix electrical/equivalence regressions are missing (run `35890238325`). |
| Kempston joystick | REQUIRED initial joystick interface | OPEN | src/core/wz_kempston.c; src/app/wz_kempston_mapping.c | Implementation files are present; normalized input and direct port-read tests are missing. |
| Beeper | REQUIRED | PARTIAL | src/core/audio/wz_beeper.c; src/core/audio/wz_audio_mixer.c | Canonical mixer fingerprint passes; timestamp transitions and hardware-derived beeper behavior are not separately validated (run `35895696032`). |
| AY-3-8912 on 128K | REQUIRED | PARTIAL | src/core/audio/wz_ay.c; src/core/audio/wz_audio_mixer.c | Canonical mixer fingerprint passes; 128K AY register/tone/noise/envelope timing regressions are missing (run `35895696032`). |
| Tape: standard TAP | REQUIRED | PARTIAL | src/core/wz_tape.c | DIZZY4K standard TAP runs through the GUI; the complete TAP loader corpus and timing proof are missing (run `35890238325`). |
| Tape: Warajevo native TAP | REQUIRED compatibility path | OPEN | src/core/wz_tape.c | Parser/decoder implementation is present; format-specific acceptance fixtures and pinned results are missing. |
| Tape: TZX | REQUIRED | OPEN | src/core/wz_tape.c | Parser/decoder implementation is present; format-specific acceptance fixtures and pinned results are missing. |
| Tape: WAV/audio input | REQUIRED deterministic decode path | OPEN | src/core/wz_tape.c | Parser/decoder implementation is present; format-specific acceptance fixtures and pinned results are missing. |
| Live physical cassette capture | LATER; not an initial blocker | SCOPED | — | §2.5 disposition is explicit; keep outside initial implementation/test closure. |
| Snapshots: SNA 48K/128K | REQUIRED | PARTIAL | src/core/wz_state.c | Core canonical state serialize/restore fingerprint matches; SNA 48K/128K format round-trip fixtures are missing (run `35894160451`). |
| Snapshots: Z80 | REQUIRED | OPEN | src/core/wz_state.c | Format implementation is present; Z80 variant/load-save compatibility fixtures and pinned results are missing. |
| Interface 1 | REQUIRED | OPEN | src/core/wz_machine.c; src/core/wz_bus.c | Machine/bus code is present; ROM paging/latch/reference tests are missing. |
| Microdrive / MDR | REQUIRED | OPEN | src/core/wz_microdrive.c | Implementation is present; MDR parsing, read/write, motor timing, and flush regressions are missing. |
| Original Sinclair/ZX Net behavior | REQUIRED | OPEN | src/core/wz_zxnet.c | Device implementation is present; deterministic loopback/reference proof is missing. |
| ZX Printer | REQUIRED legacy peripheral | OPEN | src/core/wz_printer.c; src/app/wz_printer_manager.c | Implementation and manager exist; peripheral timing and required legacy workflows lack pinned regression proof. |
| Built-in monitor/debugger | REQUIRED legacy workflow | OPEN | src/core/wz_debugger.c; src/app/wz_debugger_window.c | Debugger APIs/window exist; function-level legacy inventory and workflow regression proofs are incomplete. |
| Runtime speed control | REQUIRED, redesigned as host pacing | PARTIAL | src/app/wz_host_pacing.c; src/app/wz_speed_policy.c | Runner demo selects 800%; pacing and cassette wall-clock multiplier equivalence are not asserted (run `35890238325`). |
| 128K MIDI interface | LATER; not an initial blocker | SCOPED | — | §2.5 disposition is explicit; keep outside initial implementation/test closure. |
| 128K extended keypad | LATER; not an initial blocker | SCOPED | — | §2.5 disposition is explicit; keep outside initial implementation/test closure. |
| Historical RS-232 host redirection | LATER host-integration work; authentic IF1 state first | SCOPED | — | §2.5 disposition is explicit; keep outside initial implementation/test closure. |
| Historical external plug-in ABI | NOT an initial compatibility requirement | SCOPED | — | §2.5 disposition is explicit; keep outside initial implementation/test closure. |
| ZXCOMP executable-snapshot compiler | LATER/separate tool; not part of initial emulator binary | SCOPED | — | §2.5 disposition is explicit; keep outside initial implementation/test closure. |
| Historical database/shell/help system | REPLACE with modern UI where functionality is retained | SCOPED | — | §2.5 disposition is explicit; keep outside initial implementation/test closure. |
| Historical file-conversion utilities | LATER utility work unless required by a media test | SCOPED | — | §2.5 disposition is explicit; keep outside initial implementation/test closure. |
| DOS/BIOS/video/sound host mechanisms | REPLACE; never compatibility targets themselves | SCOPED | — | §2.5 disposition is explicit; keep outside initial implementation/test closure. |
