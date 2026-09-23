<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Core/System acceptance evidence audit

Task 450 audit is complete; architecture acceptance is not. `PASS` means the cited evidence directly covers the full criterion; `PARTIAL` records narrower evidence; `OPEN` means no adequate tracked proof was found. Source inspection and general builds do not count as behavioral proof. On commit `b8fe44d`, the test-ledger gate passed (run `35895696009`), the four-runner clean Release build passed (run `35895695999`), four-runner project tests passed (run `35895696032`), and the pathname check passed (run `35895695983`). The tracked test ledger contains only `canonical-core-regression`. Every OPEN criterion is therefore an unresolved proof or implementation gap, not an implied pass.

Four-runner baseline and determinism evidence: run `35894280373`. Four-runner clean Release build evidence: run `35894280436`. Four-runner GUI/Telnet DIZZY4K smoke evidence: run `35890238325`. Headless canonical core proof: run `35894160451`, pinned at `test-results/canonical-core-regression.json`.

| # | Core §50 criterion | Status | Evidence or outstanding proof |
|---:|---|---|---|
| 1 | the emulation core is portable C; | PASS | C11 Release build on all four matrix runners; run `35894280436`. |
| 2 | the core runs headlessly without Sokol; | PASS | Headless `wz_core` CPU/raster/audio/tape/snapshot run with Sokol disabled; run `35894160451`. |
| 3 | the deterministic core has no host/Sokol/socket dependency; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 4 | application/orchestration code bridges core output/input to host services; | PARTIAL | GUI/Telnet DIZZY4K flow reached a Spectrum screenshot on all four runners; run `35890238325`. |
| 5 | Sokol code is compiled into release program binaries; | PARTIAL | GUI host build and binary packaging passed on all four runners; run `35890238325`; dependency inventory remains open. |
| 6 | there is no project-supplied multimedia shared library required at runtime; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 7 | supported platform builds use the same deterministic core; | PASS | Same `wz_core` target produced matching CPU/raster/audio/tape/snapshot fingerprints on four runners; run `35894280373`. |
| 8 | canonical emulated time is an integer model-specific master tick; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 9 | CPU T-state/phase is derived from master time rather than defining universal machine time; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 10 | deterministic state hashes match across supported CPU architectures; | PASS | CPU canonical state fingerprint matched across four hosted runners; run `35894280373`. |
| 11 | deterministic state hashes match across supported compilers; | PARTIAL | Matching state fingerprints on AppleClang and MSVC runners; the complete supported compiler set is not covered; run `35894280373`. |
| 12 | Z80 memory and I/O operations can occur at exact intra-instruction master ticks; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 13 | contention is applied by the bus/machine timing model; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 14 | same-master-edge hardware ordering is explicit and evidence-based; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 15 | the ULA fetches memory on an explicit raster schedule; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 16 | CPU writes become visible according to real bus ordering; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 17 | full border/raster timing is represented; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 18 | the 256 x 192 bitmap is not treated as the entire display; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 19 | FLASH and BRIGHT are emulated as Spectrum semantics; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 20 | floating-bus behavior is derived from the timed ULA model where supported; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 21 | overscan/rainbow/multicolor effects do not require application-specific hacks; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 22 | NIRVANA/NIRVANA+ class timing can be represented by the core; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 23 | BIFROST-class timing can be represented by the core; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 24 | beeper transitions are master-timestamped; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 25 | AY state advances from emulated master time; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 26 | canonical audio is deterministic before Sokol conversion; | PASS | Canonical mixer fingerprint matched the frozen fixture without Sokol conversion on four runners; run `35894280373`. |
| 27 | Sokol audio does not drive CPU execution; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 28 | host sound is enabled only from 0.5x through 2.0x inclusive and is muted outside that range without stopping emulated beeper/AY progression; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 29 | audio buffering cannot silently force the emulator away from the selected runtime speed; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 30 | runtime speed control changes host pacing without changing internal Spectrum timing relationships; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 31 | runtime speed can change while running without discontinuity in the master-tick timeline; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 32 | emulator-controlled cassette playback and SAVE output automatically follow the runtime speed multiplier in wall-clock time while retaining canonical emulated timing; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 33 | Normal cassette loading is the default loading mode; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 34 | Normal loading executes the real emulated loader against timed EAR edges; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 35 | Instant/Trap loading is optional and explicitly selectable; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 36 | a supported Instant/Trap load reaches the same Spectrum-observable post-load canonical machine state as the corresponding Normal load from the same initial state and tape data; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 37 | unsupported, uncertain, or non-equivalent trap cases fall back safely to Normal loading; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 38 | local and Telnet source ownership is resolved outside the Spectrum core; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 39 | the Spectrum keyboard core receives only normalized physical matrix transitions, never source identity; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 40 | simultaneous row selection follows authentic matrix-selection behavior; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 41 | the complete Telnet transport/keyboard acceptance contract in Section 55.20 passes and the companion UI Telnet-control acceptance contract passes; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 42 | snapshots are portable across x86-64 and AArch64; | PASS | Snapshot serialization/state-round-trip fingerprints matched on x64 and ARM64 hosted runners; run `35894280373`. |
| 43 | original Warajevo media/peripheral behavior is regression-tested; | PARTIAL | DIZZY4K is one original-media integration case; full legacy media/peripheral regression inventory is absent; run `35890238325`. |
| 44 | known differences from Warajevo 2.50, and from observable 2.51/2.52 behavior where used as evidence, are documented when hardware correctness intentionally supersedes historical approximation; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 45 | Windows x86-64 passes the release test suite; | PARTIAL | Clean Release build and DIZZY4K run passed on Windows x64; complete release test suite is not present; runs `35894280436`, `35890238325`. |
| 46 | Linux x86-64 passes the release test suite; | PARTIAL | Core regression passed on Linux x64; complete release test suite is not present; run `35894160451`. |
| 47 | Linux AArch64 passes the release test suite; | OPEN | No Linux AArch64 release test run is recorded. |
| 48 | macOS Apple Silicon passes the release test suite; | PARTIAL | Clean Release build and DIZZY4K run passed on macOS ARM64; complete release test suite is not present; runs `35894280436`, `35890238325`. |
| 49 | release dependency inspection confirms the single-program-binary policy; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 50 | ROM/firmware redistribution status is explicit and legally separated from emulator architecture. | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 51 | the complete pinned Fuse Z80 unit-test suite passes with no unexplained skipped or failing applicable cases; | OPEN | No pinned Fuse suite source, driver, or passing proof exists in the tracked test ledger. |
| 52 | Phase 0 machine-timing evidence and ROM-hash baselines are frozen before dependent timing implementation is accepted; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 53 | the mandatory Phase 4A early timing-smoke gate passes before host/UI work is allowed to mask foundational timing defects; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 54 | the machine support matrix in Section 9.1 and legacy-feature disposition in Section 2.5 are reflected in the backlog with no unspecified "remaining" compatibility work; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 55 | `design/media-format-support.md` is frozen to the required block/variant detail before Phase 7/8 media tickets are accepted; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 56 | the private development-only `WZSN-PRIVATE-TEST-MEDIA` regression run has completed with no unexplained failures before full difficult-media or hardware-hack compatibility is claimed; the private files themselves are not repository or distribution artifacts; | OPEN | No private-media runner proof is recorded. |
| 57 | every migrated legacy routine has provenance, copyright/license classification, derivation clearance, C destination, and regression evidence; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 58 | the Phase-10 Interface1/Microdrive/ZXNet companion specification is frozen before those implementation tickets are accepted; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 59 | every implementation phase satisfies its Section 49.1 exit gate; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 60 | Telnet Control-Port probing/bind-family policy, Telnet framing/negotiation, keyboard command vocabulary, hold interval, scheduling rule, and second-client behavior conform to Section 55, while application-control grammar and command-registry projection conform to the UI architecture; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 61 | implementation tickets satisfy the Section 49.2 derivation contract and do not contain unresolved architecture decisions; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 62 | real-hardware/reference certification covers every item required by Section 40 before the architecture-complete milestone; | OPEN | No real-hardware/reference certification record is recorded. |
| 63 | every mandatory initial target/compiler group in Section 35 executes the required deterministic regression suite; | PARTIAL | Current four-runner deterministic fingerprint matrix covers Windows/macOS x64 and ARM64, not every target/compiler group in Core §35; run `35894280373`. |
| 64 | Kempston joystick emulation uses normalized joystick state rather than keyboard shortcuts inside the core, and direct port-read tests pass; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 65 | every required UI workflow, stable command ID, GUI/Telnet/test projection, remote-permission rule, screenshot workflow, and menu-state rule defined by `design/warajevo-zx-spectrum-next-ui-architecture.md` passes its companion acceptance contract; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 66 | concurrent WZSN processes allocate Control Ports by the Section-55.2 first-free 30740-32787 contract without duplicate numeric ownership, and full range exhaustion is nonfatal; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 67 | the core networking-mode state is exactly `NONE`, `INTERFACE1`, or `EAR_MIC` and cannot represent simultaneous Interface-1/Ear+Mic activation; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 68 | `NONE`/`INTERFACE1` cold-reconfiguration destroys prior RAM/hook/device state, preserves only application run/pause state, and resolves dirty Microdrive media without silent data loss before leaving Interface-1; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 69 | before Architecture #3 is implemented, `EAR_MIC` cannot falsely expose a working routed network; when implemented it is selectable only on an Architecture-#3-certified ZX Spectrum 48K Issue-2 profile/variant, and its actual fidelity/stack/bootstrap acceptance is owned by that downstream document; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 70 | concurrent WZSN processes satisfy the Section-28.7 host-data safety rules for settings, writable media, snapshots/exports/conversions, temporary files, and Telnet screenshots; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 71 | every developer/debug WZSN process can own an independent binary circular timing trace file whose total size never exceeds 16 MiB; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 72 | the trace records canonical master ticks, same-tick ordering, CPU/bus/ULA/ contention/interrupt state deeply enough to reconstruct timing failures and `TIMING_FULL` retains at least eight complete 48K frames under the Phase-2 retention measurement; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 73 | trace wrap, freeze, file-I/O failure, and trace enable/disable do not alter canonical machine state, hashes, or deterministic event ordering; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 74 | concurrent WZSN processes cannot share or overwrite one another's trace files, and a standalone reader can recover complete records from a wrapped or abruptly terminated trace while rejecting incomplete trailing data; | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
| 75 | the circular trace backend is implemented and tested during Phase 1 and is available before Phase-2 CPU/timing implementation begins. | OPEN | No criterion-specific tracked test or pinned proof has been recorded. |
