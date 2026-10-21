<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Legacy feature disposition audit

Task 452 audit is complete. Section 2.5 contains 32 explicit dispositions: 20 REQUIRED and 12 deferred/replaced/non-initial. Every REQUIRED row is mapped to its phase implementation area or source and current proof gap. 13 have limited runner evidence; 7 lack criterion-level pinned proof. Source presence alone is not acceptance evidence. The four-runner model-switch regression verifies 128K fixed and paged RAM behavior plus normalized Kempston input and direct port reads (run `36563713245`; [proof](../../test-results/model-switch.json)).

All deferred/replaced/non-initial rows remain scoped as stated in Section 2.5 and are not promoted into initial release blockers. Historical UI replacements are reconciled separately under task 453. Architecture-#3 implementation remains out of scope.

| Feature | Section 2.5 disposition | Audit | Implementation reference | Evidence or remaining work |
|---|---|---|---|---|
| ZX Spectrum 48K PAL | REQUIRED - initial certified machine | PARTIAL | src/core/wz_machine_profile.c | Four-runner build and DIZZY4K launch/screenshot cover a 48K path, not the complete certified timing suite (runs `35896075757`, `35890238325`). |
| ZX Spectrum 128K PAL | REQUIRED - initial certified machine | PARTIAL | src/core/wz_machine_profile.c; src/core/wz_machine.c | Hosted model-switch contract verifies fixed RAM banks, bank selection, independent paged-bank contents, screen/ROM bank bits, and paging lock on all four runner families (run `36560521477`; [proof](../../test-results/model-switch.json)). Dedicated 128K interrupt/raster behavior and hardware correlation remain open. |
| ZX Spectrum +2 | LATER compatibility target; not an initial blocker | SCOPED | — | §2.5 disposition is explicit; keep outside initial implementation/test closure. |
| Timex Sinclair 2068 | LATER compatibility target; preserved source retained | SCOPED | — | §2.5 disposition is explicit; keep outside initial implementation/test closure. |
| DCK/Timex memory expansions | LATER with Timex support | SCOPED | — | §2.5 disposition is explicit; keep outside initial implementation/test closure. |
| Z80 CPU incl. documented/undocumented | REQUIRED | PARTIAL | src/core/wz_z80.c | All 1,356 cases in the pinned Fuse Z80 suite passed on current-source hosted run `36667321892` at `6202c6040d45080a0848eccae97064c9633a7b2e` ([proof](../../test-results/fuse-z80-conformance.json)); the suite does not demonstrate independent coverage of every documented and undocumented opcode. |
| ULA/border/contention/floating bus | REQUIRED | PARTIAL | src/core/wz_bus.c; src/core/wz_raster.c | Canonical raster fingerprint and GUI output pass; timing-smoke, border/contention/floating-bus acceptance is unproven (runs `35895696032`, `35890238325`). |
| 48K keyboard matrix | REQUIRED | PARTIAL | src/core/wz_keyboard_matrix.c | Runner demo uses physical-key commands to start the tape; matrix electrical/equivalence regressions are missing (run `35890238325`). |
| Kempston joystick | REQUIRED initial joystick interface | PARTIAL | src/core/wz_kempston.c; src/app/wz_kempston_mapping.c | Four-runner regression verifies normalized controls, reset on profile change, and direct bus port reads (run `36563713245`; [proof](../../test-results/model-switch.json)); GUI input-mapping behavior remains open. |
| Beeper | REQUIRED | PARTIAL | src/core/audio/wz_beeper.c; src/core/audio/wz_audio_mixer.c | Canonical mixer fingerprint passes; timestamp transitions and hardware-derived beeper behavior are not separately validated (run `35895696032`). |
| AY-3-8912 on 128K | REQUIRED | PARTIAL | src/core/audio/wz_ay.c; src/core/audio/wz_audio_mixer.c | Deterministic AY register/tone/noise/envelope timing passed 67 assertions on all five native targets from run `39621225694`; hosted proof processing completed in run `39622601174` ([proof](../../test-results/ay-timing-contract.json)). Analog waveform and physical audio equivalence remain open. |
| Tape: standard TAP | REQUIRED | PARTIAL | src/core/wz_tape.c | DIZZY4K standard TAP runs through the GUI; the complete TAP loader corpus and timing proof are missing (run `35890238325`). |
| Tape: Warajevo native TAP | REQUIRED compatibility path | OPEN | src/core/wz_tape.c | Parser/decoder implementation is present; format-specific acceptance fixtures and pinned results are missing. |
| Tape: TZX | REQUIRED | PARTIAL | src/core/wz_tape.c | Exact-source GUI acceptance mounted checksummed synthetic TZX media and showed parsed standard-data blocks in the production Tape Manager (run `39548310544`, proof: [gui-tape-formats.json](../../test-results/gui-tape-formats.json)); broad block-type, playback, and malformed-input coverage remains open. |
| Tape: WAV/audio input | REQUIRED deterministic decode path | PARTIAL | src/core/wz_tape.c | Exact-source GUI acceptance mounted checksummed synthetic WAV media and showed decoded EAR signal segments in the production Tape Manager (run `39548310544`, proof: [gui-tape-formats.json](../../test-results/gui-tape-formats.json)); playback timing, broader waveform coverage, and malformed-input cases remain open. |
| Live physical cassette capture | LATER; not an initial blocker | SCOPED | — | §2.5 disposition is explicit; keep outside initial implementation/test closure. |
| Snapshots: SNA 48K/128K | REQUIRED | PARTIAL | src/core/wz_state.c | Core canonical state serialize/restore fingerprint matches; SNA 48K/128K format round-trip fixtures are missing (run `35894160451`). |
| Snapshots: Z80 | REQUIRED | OPEN | src/core/wz_state.c | Format implementation is present; Z80 variant/load-save compatibility fixtures and pinned results are missing. |
| Interface 1 | REQUIRED | PARTIAL | src/core/wz_machine.c; src/core/wz_bus.c | Five-target synthetic 8 KiB ROM paging contract passed in run `39519638077` at source `d284d5409f68f00e58d43faa42006e50185ebe27` ([proof](../../test-results/interface1-paging.json)); authentic-ROM compatibility, full latch/device behavior, and hardware/reference correlation remain open. |
| Microdrive / MDR | REQUIRED | PARTIAL | src/core/wz_microdrive.c | Five-target MDR parsing/trailer contract (`39523045347`), complete 9-sector stream and motor-selection contract (`39524634277`), write-protection/flush contract (`39526865899`), sanitized transition fuzz (`39535192591`), and Interface-1 timing trace (`39533151050`) are pinned in `test-results/`. Hardware/ROM correlation and full real-media acceptance remain open. |
| Original Sinclair/ZX Net behavior | REQUIRED | PARTIAL | src/core/wz_zxnet.c | The exact-source five-runner block-loopback contract passed eight assertions per runner in run `39517679760` at source `7f10b95b85148571eeb102246f5aac2288ac3677` ([proof](../../test-results/zxnet-behavior.json)). It checks all 256 receive bytes, framing rotation, busy/free countdowns, accepted/rejected claims, a 256-byte transmit payload, failed-callback preservation/retry, and readback of the committed block. Independent hardware/ROM-reference comparison and Interface-1 serial signal integration remain open. |
| ZX Printer | REQUIRED legacy peripheral | PARTIAL | src/core/wz_printer.c; src/app/wz_printer_manager.c | The [migration ledger](migration-ledger.md) now inventories `ZXP_IN`, `ZXP_OUT`, `Z_IN_FAR`, `Z_OUT_FAR`, `INT17`, and `SETASCII` with dispositions and regression strategies. The printer command/settings and manager contract passes five assertions (run `38318157503`, proof: [printer-settings.json](../../test-results/printer-settings.json)); direct port/status/pixel/flush regression proof, printer timing, paper output, and full legacy workflow remain unverified. |
| Built-in monitor/debugger | REQUIRED legacy workflow | PARTIAL | src/core/wz_debugger.c; src/app/wz_debugger_window.c | The debugger window contract (run `38323997617`) and shared Snapshot Inspector contract (run `38319824115`) pass; complete instruction stepping, breakpoint, memory editing, and legacy workflow inventory remain open. Proofs: [debugger-window-contract.json](../../test-results/debugger-window-contract.json) and [snapshot-inspector.json](../../test-results/snapshot-inspector.json). |
| Runtime speed control | REQUIRED, redesigned as host pacing | PARTIAL | src/app/wz_host_pacing.c; src/app/wz_speed_policy.c | All seven policy values share production GUI/toolbar/Telnet command semantics in four-runner equivalence proof `36393985044`; cassette speed invariants pass in `36518961917`, and Normal/Instant UI selection is verified in `39548674403`. Exact wall-clock pacing accuracy across every speed and host remains open. |
| 128K MIDI interface | LATER; not an initial blocker | SCOPED | — | §2.5 disposition is explicit; keep outside initial implementation/test closure. |
| 128K extended keypad | LATER; not an initial blocker | SCOPED | — | §2.5 disposition is explicit; keep outside initial implementation/test closure. |
| Historical RS-232 host redirection | LATER host-integration work; authentic IF1 state first | SCOPED | — | §2.5 disposition is explicit; keep outside initial implementation/test closure. |
| Historical external plug-in ABI | NOT an initial compatibility requirement | SCOPED | — | §2.5 disposition is explicit; keep outside initial implementation/test closure. |
| ZXCOMP executable-snapshot compiler | LATER/separate tool; not part of initial emulator binary | SCOPED | — | §2.5 disposition is explicit; keep outside initial implementation/test closure. |
| Historical database/shell/help system | REPLACE with modern UI where functionality is retained | SCOPED | — | §2.5 disposition is explicit; keep outside initial implementation/test closure. |
| Historical file-conversion utilities | LATER utility work unless required by a media test | SCOPED | — | §2.5 disposition is explicit; keep outside initial implementation/test closure. |
| DOS/BIOS/video/sound host mechanisms | REPLACE; never compatibility targets themselves | SCOPED | — | §2.5 disposition is explicit; keep outside initial implementation/test closure. |


## Backlog crosswalk for Core criterion 54

The frozen machine scope and every legacy disposition are represented in the
ordered Phase 0–16 backlog in [04-wzsn-architectures-1-2-developer-tasks.md](04-wzsn-architectures-1-2-developer-tasks.md):

| Architecture requirement | Backlog phase |
| --- | --- |
| ZX Spectrum 48K PAL certification | Phase 4 and Gate 4A; regression work continues in Phase 14 |
| ZX Spectrum 128K PAL certification | Phase 9 |
| Z80 documented and undocumented instructions | Phase 2 |
| ULA, border, contention, floating bus | Phases 3–4 and Gate 4A |
| Keyboard matrix and Kempston | Phase 5; UI mapping in Phase 12 |
| Beeper | Phase 6 |
| AY-3-8912 | Phase 9 |
| Standard TAP, Warajevo native TAP, TZX, WAV | Phase 7; file workflows in Phase 12D |
| SNA and Z80 snapshots | Phase 8; file workflows in Phase 12C |
| Interface 1, Microdrive, original ZX Net | Phase 10 |
| ZX Printer and monitor/debugger | Phase 11; manager UI in Phase 12E |
| Runtime speed control | Phase 5; optimization/regression checks in Phase 16 |
| Historical replacements retained by modern UI | Phases 12A–12G |

The audit table above accounts for all 20 REQUIRED Section 2.5 rows and all 12
LATER, REPLACE, NOT-required, or future-scope rows. Every REQUIRED row has a
named implementation area and its remaining acceptance evidence is itemized
in that row. Deferred rows remain explicitly scoped and are not promoted into
the initial milestone. There is no umbrella "remaining compatibility" backlog
item. Architecture-#3 routed MIC/EAR implementation remains excluded.
