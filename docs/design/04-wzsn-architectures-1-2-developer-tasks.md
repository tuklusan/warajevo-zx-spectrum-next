<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Warajevo ZX Spectrum Next — Developer Task List for Architectures #1 and #2

This is the ordered implementation backlog for:

- `design/warajevo-zx-spectrum-next-architecture.md` — Architecture #1, Core/System Architecture.
- `design/warajevo-zx-spectrum-next-ui-architecture.md` — Architecture #2, UI Architecture.

Architecture #3 (`zx48-mic-ear-router-network-architecture.md`) is explicitly out of scope. The only Architecture-#3-facing work permitted here is the upstream `EAR_MIC` networking-mode reservation, disabled-state plumbing, cassette-ownership boundary, and cold-reconfiguration contract already required by Architectures #1 and #2. No MIC/EAR router protocol, resident network stack, Raspberry Pi router, physical electronics, or Architecture-#3 implementation belongs in this backlog.

## Execution rules

- Execute tasks in numeric order unless a task explicitly states that it can proceed in parallel.
- A phase-gate task blocks all tasks in later phases that depend on it.
- When a task touches preserved Warajevo code, complete the migration/provenance record before committing implementation code.
- Every implementation task must add or update automated tests appropriate to its behavior unless the task is itself exclusively a specification/test task.
- Deterministic-core changes must be testable headlessly without Sokol, GUI, sockets, or host wall-clock dependence.
- No task may silently resolve a deferred design decision that the architecture requires to be frozen first.
- No task may implement an Architecture #3 feature merely because the upstream enum or UI option exists.

---

## Phase 0 — Evidence, repository, build, provenance, and specification freeze

001. [P0][Core §§2,4,33,48,49] Create the canonical repository directory skeleton (`src`, `tests`, `design`, `third_party`, `reference`, `cmake/toolchains`) without placeholder runtime dependencies — done when CMake configures an empty project cleanly on Windows and Linux.
002. [P0][Core §§3,33] Set the project language baseline to ISO C11 and enable high-warning developer builds — done when one trivial C target compiles as C11 under GCC, Clang, MSVC, and clang-cl where available.
003. [P0][Core §§7,33,35] Add common compiler-warning and sanitizer options without introducing compiler-specific behavior into the core — done when debug/sanitizer configurations configure and build on at least GCC and Clang.
004. [P0][Core §§48,54] Add the current `LICENSE.txt`, `NOTICE.md`, and the two canonical architecture documents to the repository root/design tree — done when a clean checkout contains the required legal/design authorities.
005. [P0][Core §§2.1,41,48] Import the preserved Warajevo 2.50 source as read-only reference material under `reference/original-warajevo/` or an equivalent immutable reference mechanism — done when the exact upstream commit/source identity is recorded and build code cannot accidentally compile it into WZSN.
006. [P0][Core §§2.1.1,41,48] Create `design/migration-ledger.md` with fields for source file/function, upstream identity, copyright/license classification, migration type, clearance, replacement target, and regression evidence — done when one sample legacy routine can be fully represented.
007. [P0][Core §§2.1.1,41,48] Classify every legacy source file that may be referenced during initial implementation as migrate/reimplement/research-only/excluded — done when no Phase-1 or Phase-2 source file lacks a disposition.
008. [P0][Core §§2.1.1,48] Record the known separately licensed/unclear legacy materials and enforce a “no migration before clearance” rule — done when the migration ledger marks them non-default for reuse.
009. [P0][Core §§2.2,2.3,10,13,14,15,49,51] Create `design/machine-timing-evidence.md` and freeze the 48K PAL master-clock/master-tick relationship, frame geometry, interrupt timing, contention evidence, floating-bus evidence, and same-edge ordering sources — done when every Phase-1–4 timing constant has an authority entry.
010. [P0][Core §§10.4,14,49,51] Freeze the 48K same-master-tick event-order table covering CPU bus activity, ULA fetches, writes, contention, interrupt edges, and externally visible state — done when test cases can reference a named ordering rule rather than implicit code order.
011. [P0][Core §§9,29,49] Record certification ROM identities/hashes for the required 48K and 128K profiles without redistributing ROM bytes — done when tests can validate the selected ROM externally.
012. [P0][Core §§36.1,49] Pin the exact Fuse Z80 test-suite revision and add acquisition/integration instructions — done when the test runner can report the pinned revision even before the CPU implementation passes it.
013. [P0][Core §§36,44,49,51] Freeze the versioned binary timing-trace/checkpoint schema, including the 16 MiB per-process circular-file header, compact record framing, wrap/generation markers, periodic absolute synchronization records, same-tick ordering field, and `TIMING_FULL` event taxonomy — done when the format can represent CPU instruction/register state, every externally visible bus operation, contention, interrupt decisions, ULA/floating-bus/raster events, and later peripheral edges without host timestamps becoming machine time.
014. [P0][Core §§36,49] Select and document the early timing-smoke cases required for Phase 4A, including instruction/bus timing, interrupt edge, contention, border transition, floating bus, one raster-racing case, and one multicolor case — done when each case has an expected evidence source.
015. [P0][Core §§36.2,50] Add support for optional `WZSN_PRIVATE_TEST_MEDIA` discovery in the repository-root `WZSN-PRIVATE-TEST-MEDIA/` directory, with Git configured to publish only its public guidance files and with no subdirectory, naming, metadata, hash, provenance, or licensing requirements for private local media — done when absence is reported as private corpus unavailable rather than a public repository-validation failure.
016. [P0][Core §§36.2,50] Add a repository guard/test that prevents ignored files from the configured private test-media directory from being copied into source archives or release staging — done when a deliberately placed sentinel private file never appears in packaging output.
017. [P0][Core §§33,35,49] Create initial CMake toolchain/CI definitions for Windows x86-64, Linux x86-64, Linux AArch64, macOS AArch64, and secondary macOS x86-64 — done when each target has a documented configure path even if hardware runners are not yet all online.
018. [P0][Core §§33,35] Create the logical build targets `wz_core`, `wz_headless`, `wz_tests`, and the eventual GUI application target with only stubs where necessary — done when dependency direction prevents `wz_core` from linking host/UI libraries.
019. [P0][Core §§5,6,26,30] Add an automated dependency-boundary check that fails if deterministic-core sources include Sokol, socket, OS GUI, or host-clock headers — done when a synthetic forbidden include is caught by CI.
020. [P0][Core §§49,49.1] Complete the Phase-0 review checklist and freeze all Phase-0 decisions in the design files before opening Phase-1 implementation tickets — done when timing evidence, ROM IDs, migration rules, Fuse pin, trace schema, and smoke-corpus list are review-approved.

## Phase 1 — Portable core types, machine state, master timeline, serialization, hashing

021. [P1][Core §§7,8] Define project fixed-width integer aliases, explicit boolean conventions, endian helpers, and safe bit/shift helpers — done when unit tests cover boundary values and sanitizers report no UB.
022. [P1][Core §7] Add compile-time/static assertions for integer widths and required representation assumptions — done when unsupported hosts fail configuration with a clear diagnostic rather than compiling incorrectly.
023. [P1][Core §§8.1,9.2] Define the initial `wz_machine_t` ownership model with explicit CPU, memory, bus, ULA, input, media/peripheral placeholders, and profile reference — done when a zeroed/initialized machine can be constructed and destroyed headlessly.
024. [P1][Core §§9.1,9.2] Define the machine-profile descriptor for 48K PAL and placeholder-safe 128K PAL fields without implementing 128K behavior yet — done when profiles are selectable data, not scattered conditionals.
025. [P1][Core §§10.1,10.2] Implement the integer canonical master-tick type and monotonic machine-time counter — done when tests advance across large values without float use or signed overflow.
026. [P1][Core §§10.2,10.3] Implement profile-defined conversion from master tick to derived CPU T-state and phase — done when round-trip/reference tests match the frozen 48K ratios at frame boundaries.
027. [P1][Core §§10.4,14] Implement the scheduler’s deterministic same-tick ordering abstraction without committing to a performance-optimized container — done when synthetic equal-tick events execute in frozen priority/order.
028. [P1][Core §§8.2,25] Define canonical versioned internal state-serialization primitives with explicit byte order and no raw-struct writes — done when a small sample state serializes identically across two compilers.
029. [P1][Core §§8.3,36] Implement canonical state hashing over explicitly selected deterministic fields — done when host-only fields can be changed without altering the machine hash.
030. [P1][Core §§8.4,6] Add tests proving host pointers, filenames, window state, wall-clock values, thread IDs, and socket state are excluded from canonical machine state — done when perturbing them leaves hashes unchanged.
031. [P1][Core §§31,45] Create the headless core runner loop skeleton with one owner thread for mutable machine state — done when it can advance master time without any Sokol dependency.
032. [P1][Core §§44,45] Add the narrow structured trace-sink interface at the deterministic-core boundary; core code emits trace events but performs no host file I/O — done when a null sink and an in-memory test sink receive identical event semantics without changing canonical hashes.
033. [P1][Core §§4,44.1] Implement one exclusive per-process circular trace file, capped at exactly 16 MiB total including header, with unique no-clobber session naming and startup path reporting — done when concurrent WZSN processes create different files and repeated wrapping never grows either file beyond 16,777,216 bytes.
034. [P1][Core §§44.1,44.2] Implement the versioned compact binary trace header/record codec with explicit record lengths, monotonically increasing event sequence, master-tick/same-tick ordering, wrap generation, periodic absolute synchronization records, and safe oldest-record overwrite — done when a standalone parser can decode a ring after multiple wraps without needing overwritten history.
035. [P1][Core §§44.2,44.3] Wire `TIMING_FULL`, developer-marker, and freeze controls through the trace sink and instrument every Phase-1 timing primitive available so far; trace enable/disable/freeze/write failure must remain observational only — done when trace-on/off/frozen/write-failure runs produce identical canonical state and hashes.
036. [P1][Core §§44.1–44.3,45,49.1] Build the standalone trace-dump/recovery test utility and regression suite for wrap, abrupt termination/incomplete trailing record, freeze-on-first-failure, concurrent-instance separation, and synthetic worst-case record density — done when all complete pre-crash records are recoverable, the file never exceeds 16 MiB, and a synthetic event stream representative of at least eight 48K frames fits the ring; the real `TIMING_FULL` retention measurement is repeated at the Phase-2 gate once CPU/bus instrumentation exists.
037. [P1][Core §§46] Define core error/result types for initialization, invalid profile/state, serialization failure, and unsupported operation without host UI strings leaking into core logic — done when negative unit tests return stable categories.
038. [P1][Core §§7.9,35,49.1] Run UB/sanitizer smoke tests on the Phase-1 core under at least two materially different compilers — done when no sanitizer finding is unexplained.
039. [P1][Core §§8,36] Add serialization round-trip tests and cross-build golden-byte/hash tests for the Phase-1 state subset — done when bytes/hashes are identical across the selected compiler pair.
040. [P1][Core §§44,49.1] Close the Phase-1 gate — done when canonical state, master time, serialization, hashing, headless execution, C11/sanitizer requirements, and the per-process 16 MiB circular trace wrap/freeze/recovery/trace-isolation tests all pass before Phase-2 implementation begins.

## Phase 2 — Z80 decoder, execution, flags, interrupts, and bus-cycle-capable CPU

041. [P2][Core §§11,36.1] Define the Z80 register/state structure including alternate registers, IX/IY, I/R, IFF state, interrupt mode, HALT state, and undocumented-visible fields required by tests — done when state can be initialized/serialized/hashed.
042. [P2][Core §§11.3,12] Define the CPU-to-bus request interface for memory read/write, I/O read/write, interrupt acknowledge, and timing-visible internal phases — done when a mock bus can record exact requests.
043. [P2][Core §§11.2,11.3] Implement the instruction fetch/decode skeleton so external bus effects can occur at intra-instruction master ticks — done when a NOP and simple load expose the expected fetch/read/write trace.
044. [P2][Core §11] Implement the base opcode decoder table with explicit illegal/undocumented handling — done when every 0x00–0xFF primary opcode maps deterministically.
045. [P2][Core §11] Implement CB-prefixed decode/execution and flag behavior — done when Fuse/project CB tests pass.
046. [P2][Core §11] Implement ED-prefixed decode/execution and documented/undocumented behavior required by the certified core — done when applicable Fuse ED tests pass.
047. [P2][Core §11] Implement DD/FD index-prefix semantics, including ignored/repeated prefixes — done when prefix-chain tests match reference behavior.
048. [P2][Core §11] Implement DDCB/FDCB indexed-bit operations with correct memory/register side effects — done when bus traces and Fuse tests pass.
049. [P2][Core §11] Implement 8-bit arithmetic/logic flags including undocumented flag bits — done when exhaustive/sampled flag tests pass.
050. [P2][Core §11] Implement 16-bit arithmetic flags and carry/half-carry semantics — done when edge-vector tests pass.
051. [P2][Core §11] Implement DAA, CPL, SCF, CCF and other historically error-prone flag instructions with dedicated regression vectors — done when differential/reference tests pass.
052. [P2][Core §11] Implement rotates/shifts, including undocumented SLL behavior where required — done when full instruction vectors pass.
053. [P2][Core §11] Implement block transfer/search/I/O instructions with repeat timing exposed cycle-by-cycle — done when repeat and terminating-path traces match evidence.
054. [P2][Core §§11,12] Implement CALL/RET/RST/PUSH/POP and stack bus timing — done when memory-write/read order and SP transitions match trace expectations.
055. [P2][Core §11] Implement JR/JP/DJNZ conditional timing with taken/not-taken paths — done when exact cycle traces pass.
056. [P2][Core §11] Implement HALT semantics and refresh/register behavior — done when interrupt/no-interrupt HALT tests pass.
057. [P2][Core §11] Implement EI/DI delayed-interrupt acceptance semantics — done when one-instruction EI delay tests pass.
058. [P2][Core §11] Implement IM 0, IM 1, IM 2 interrupt entry and NMI handling with exact externally visible bus phases — done when interrupt trace tests pass.
059. [P2][Core §§11,36,44.2] Complete `TIMING_FULL` CPU instrumentation in the existing circular trace: instruction boundary/full register snapshot, opcode/prefix bytes, every externally visible M-cycle/bus operation, interrupt sample/accept/acknowledge, and same-tick ordering — done when a failing opcode can be reconstructed from the 16 MiB trace without GUI instrumentation.
060. [P2][Core §36.1] Integrate the complete pinned Fuse Z80 suite into `wz_tests` and map any inapplicable case to an explicit reviewed reason — done when no silent skip exists.
061. [P2][Core §§36.1,39] Add project-owned opcode/flag/interrupt tests for behavior not adequately asserted by Fuse or needed for Warajevo differential comparison — done when all CPU acceptance categories have local coverage.
062. [P2][Core §§44.2,49.1] Close the Phase-2 gate — done when the complete applicable Fuse suite passes, project CPU tests pass, exact bus-cycle traces are demonstrated, and measured `TIMING_FULL` retention in the fixed 16 MiB ring is at least eight complete 48K frames.

## Phase 3 — Memory bus, I/O bus, contention, interrupt scheduling

063. [P3][Core §§12,13] Implement the 48K memory map and RAM/ROM access routing through the machine bus — done when reads/writes and ROM protection pass boundary tests.
064. [P3][Core §§12,29] Add external ROM-byte injection/loading to the machine profile without embedding copyrighted ROMs — done when a wrong hash is rejected by certification tests.
065. [P3][Core §§12,15] Implement port decode infrastructure with a dedicated path for ULA port-FE behavior — done when partial-address decode tests pass.
066. [P3][Core §§12,22] Implement keyboard-matrix reads through the I/O bus using normalized matrix state — done when row-selection tests pass without host-key knowledge in core.
067. [P3][Core §§12,20] Implement ULA port-FE output latches for border, beeper, and MIC state at exact bus/master ticks — done when trace tests show same-edge ordering.
068. [P3][Core §§13,51] Implement the frozen 48K contention model from evidence tables — done when contended/uncontended memory and I/O timing vectors pass.
069. [P3][Core §§13,14] Apply contention in the bus/machine timing layer rather than inside opcode implementations — done when CPU code has no Spectrum-specific contention table.
070. [P3][Core §§10,14] Implement scheduled maskable interrupt assertion/deassertion at profile-defined master ticks — done when frame-boundary interrupt timing tests pass.
071. [P3][Core §§11,14] Verify CPU interrupt sampling relative to same-edge bus/ULA ordering — done when edge-case traces match the frozen event-order table.
072. [P3][Core §§12,15.6] Add the bus-visible data-source mechanism required for floating-bus emulation without yet finalizing ULA fetch values — done when a synthetic device can supply timed bus data.
073. [P3][Core §§36] Add deterministic memory/I/O trace fixtures that record address, value, direction, master tick, contention delay, and source — done when two hosts produce identical traces.
074. [P3][Core §§46] Add controlled behavior for unmapped/unsupported I/O paths according to the 48K bus contract — done when random-port fuzz tests cannot crash or invoke UB.
075. [P3][Core §§7,36] Fuzz memory/I/O access helpers for bounds, overflow, and invalid-state safety — done when sanitizer runs are clean.
076. [P3][Core §§39] Differential-test a small set of non-raster programs against preserved Warajevo 2.50 while treating real-hardware evidence as authority on disagreement — done when divergences are classified.
077. [P3][Core §§49.1] Run the Phase-3 bus/contention/interrupt evidence suite — done when no unexplained timing divergence remains.
078. [P3][Core §49.1] Close the Phase-3 gate.

## Phase 4 — 48K ULA, raster, border, floating bus, canonical video

079. [P4][Core §§15.1,15.2] Implement the 48K ULA raster coordinate/timing state over the full frame, not just 256×192 active pixels — done when line/frame counters match evidence at boundary ticks.
080. [P4][Core §§15.3,15.5] Implement scheduled ULA bitmap/attribute memory fetches at exact master ticks — done when fetch-address traces match the frozen timing table.
081. [P4][Core §§15.5,14] Resolve CPU write versus ULA fetch visibility using the frozen same-edge ordering — done when targeted same-tick tests produce the expected pixel/attribute result.
082. [P4][Core §§15.4,17.2] Implement border-color changes at timed port-FE write points across the full raster — done when mid-line border transitions occur at the expected samples.
083. [P4][Core §§15.6] Implement floating-bus values from actual timed ULA fetch/bus state rather than program-specific rules — done when floating-bus test software matches evidence.
084. [P4][Core §§16,51] Freeze the logical raster sample encoding before completing raster implementation — done when encoding is documented and tests no longer depend on implementation-private structs.
085. [P4][Core §§16.1,16.2] Implement the canonical native raster/event buffer independently of Sokol/GPU presentation — done when headless tests can retrieve a complete frame.
086. [P4][Core §§16.3,17] Implement active-pixel decoding from bitmap/attribute fetches into canonical Spectrum color semantics — done when known character/pattern fixtures match expected color indices.
087. [P4][Core §17.1] Implement INK/PAPER/BRIGHT interpretation — done when all attribute combinations pass table-driven tests.
088. [P4][Core §17.3] Implement FLASH phase using emulated frame/time state — done when host wall-clock changes cannot change flash phase.
089. [P4][Core §17.4] Define and test the host-independent canonical Spectrum palette indices separately from later RGB presentation mapping — done when core hashes contain no host RGB format.
090. [P4][Core §18] Add overscan/rainbow timing tests that exercise border and active-display transitions without software recognition hacks — done when behavior derives solely from ULA/bus timing.
091. [P4][Core §18] Add an early multicolor/NIRVANA-style memory-write-versus-fetch regression case — done when the correct attribute changes appear at exact raster positions.
092. [P4][Core §§36,37] Add canonical raster hashes and event traces to deterministic regression output — done when identical runs match across compilers.
093. [P4][Core §37] Add pixel/sample-level diagnostic dump support for failed raster tests without altering emulated state — done when a mismatch reports first differing master tick/sample.
094. [P4][Core §§46] Add malformed/invalid raster-state assertions and controlled failure paths for developer builds — done when corrupted test state fails loudly rather than overrunning buffers.
095. [P4][Core §49.1] Run 48K ULA fetch, border, raster, FLASH, and floating-bus tests.
096. [P4][Core §49.1] Close the Phase-4 gate.

## Phase 4A — Mandatory early timing validation

097. [P4A][Core §§36,49] Run the frozen instruction/bus-cycle timing smoke set and capture canonical traces — done when all cases match evidence.
098. [P4A][Core §§36,49] Run interrupt-entry/edge timing smoke tests at critical frame positions — done when no unexplained edge mismatch exists.
099. [P4A][Core §§13,36,49] Run contention and ULA-fetch-versus-CPU-write smoke tests — done when event ordering matches the frozen table.
100. [P4A][Core §§15,36,49] Run border-transition and floating-bus smoke tests — done when both pass without special-case software detection.
101. [P4A][Core §§18,36,49] Run at least one raster-racing/overscan case and one multicolor case from public/synthetic/private evidence as appropriate — done when both produce expected canonical output.
102. [P4A][Core §44] For every failure, add a minimal deterministic trace-based regression before fixing implementation code — done when no timing bug is fixed without a reproducer.
103. [P4A][Core §§36,49.1] Repeat the full timing-smoke suite after all fixes on at least two compilers.
104. [P4A][Core §49.1] Close Phase 4A; do not begin host/UI expansion until this gate is clean.

## Phase 5 — Sokol host shell, presentation, keyboard, Kempston, speed, pacing

105. [P5][Core §§27.1,27.3,51] Select and pin the exact Sokol revision before host implementation — done when source origin/license/revision are recorded and vendored/pinned.
106. [P5][Core §§27.4,51] Freeze the exact initial Linux graphics backend within the architecture’s X11 baseline — done when the decision is documented before Linux host code lands.
107. [P5][Core §§27,33] Implement the minimal Sokol application/window shell without moving any machine logic into Sokol callbacks — done when a window opens around a headless-driven core.
108. [P5][Core §§27.5] Add the macOS host translation unit in the required Objective-C compilation mode while exposing a C boundary to WZSN — done when core headers remain Objective-C-free.
109. [P5][Core §§19.1,19.2] Upload/present the canonical raster through Sokol without modifying core raster contents — done when presentation-on/off produces identical headless hashes.
110. [P5][Core §§19.3,27.4] Abstract host pixel upload/backend details so D3D/Metal/Linux backend selection cannot affect canonical raster generation — done when backend code is outside core/video truth.
111. [P5][Core §§19.4,51] Freeze the default host-visible crop/border presentation — done when screenshot/UI tests can reference a stable presentation rectangle.
112. [P5][Core §§19.4] Implement integer/nearest-neighbor default scaling behavior appropriate for Spectrum pixels — done when resize tests preserve canonical source pixels.
113. [P5][Core §§19.5] Keep CRT/analog shader effects disabled/not required; add explicit separation if placeholder hooks exist — done when no presentation effect changes screenshot canonical source.
114. [P5][Core §§22.1,22.2] Implement the host input arbiter with independent source ownership slots and no host key codes entering the core — done when local keyboard events normalize to Spectrum keys.
115. [P5][Core §§22.3] Implement the complete 40-key Spectrum keyboard matrix model — done when direct matrix-scan tests cover every row/key.
116. [P5][Core §§22.4] Implement local input timestamp assignment at orchestrator drain/current-master-tick boundaries — done when repeated scripted input produces deterministic traces.
117. [P5][Core §§22.5] Implement focus-loss handling so local held keys cannot remain stuck — done when focus transition tests release/preserve keys according to policy.
118. [P5][Core §§22.6,51] Research/freeze keyboard ghosting/electrical behavior beyond multiple-row selection before Phase-5 exit — done when implemented or explicitly ruled out from evidence.
119. [P5][Core §22.6] Freeze Kempston decode and bit values before implementation — done when the port/bit contract is documented.
120. [P5][Core §22.6] Implement deterministic Kempston joystick state in the core, separate from the keyboard matrix — done when machine-code port reads see RIGHT/LEFT/DOWN/UP/FIRE correctly.
121. [P5][Core §22.6] Implement host-to-Kempston mapping in host/input code — done when remapping host controls leaves the core joystick API unchanged.
122. [P5][Core §§10.7–10.9] Implement runtime speed values 25/50/100/200/400/800/Unlimited as host pacing policy, not machine overclocking — done when master-tick hardware timing is unchanged across settings.
123. [P5][Core §§32] Implement monotonic-host-clock pacing and sleep/yield logic outside the core — done when 100% tracks real time within a documented tolerance without feeding host time into machine state.
124. [P5][Core §§10.8] Implement speed changes while running without discontinuity in emulated master time — done when scripted speed switches preserve deterministic state.
125. [P5][Core §§10.9] Implement Unlimited pacing with no artificial sleep while preserving host responsiveness — done when the GUI/event loop remains usable under stress.
126. [P5][Core §§28.7] Add interprocess-safe preferences/config write primitives using atomic replacement/locking policy — done when two test processes cannot corrupt one settings file.
127. [P5][Core §§36,49.1] Run Windows plus one Unix-like host smoke tests for raster presentation, keyboard, Kempston, speed, pacing, and headless-hash equivalence.
128. [P5][Core §49.1] Close the Phase-5 gate.

## Phase 6 — Beeper, mixer, audio timeline, Sokol push audio

129. [P6][Core §§20.2] Implement timestamped beeper state changes from port-FE output at canonical master ticks — done when edge traces match bus writes.
130. [P6][Core §§20.2] Preserve MIC output state separately from audible beeper contribution even when both share port-FE writes — done when tape/network ownership tests can observe MIC without host audio assumptions.
131. [P6][Core §§20.5,51] Freeze the canonical internal audio sample rate before mixer implementation — done when it is documented as deterministic machine output policy.
132. [P6][Core §§20.4,51] Freeze the fixed-point mixer numeric representation and saturation/rounding rules — done when cross-compiler output can be bit-identical.
133. [P6][Core §§20.4,51] Freeze the initial AY analog-mixing approximation/model for later 128K use — done when Phase-9 AY implementation has a defined mixer target.
134. [P6][Core §§20.4,20.5] Implement deterministic beeper-to-canonical-audio sampling/mixing — done when PCM/hash output matches across compilers.
135. [P6][Core §§21] Implement Sokol **push** audio delivery only; do not let an audio callback drive emulation — done when core/audio production continues headlessly.
136. [P6][Core §§10.10,21] Implement host-audio enable policy for 0.5× through 2.0× inclusive and host mute outside that range while emulated audio state continues — done when speed tests verify mute without state discontinuity.
137. [P6][Core §§51] Select/freeze the host resampling algorithm for the audible 0.5×–2.0× range — done when the decision is documented and host-only.
138. [P6][Core §§21,32] Implement bounded host audio buffering/backpressure that never changes machine event order or selected speed — done when overflow/underflow tests cannot mutate canonical hashes.
139. [P6][Core §38] Add canonical beeper-transition and PCM-hash regression tests before Sokol conversion.
140. [P6][Core §38] Add host-delivery smoke tests for WASAPI, ALSA, and CoreAudio where runner hardware permits, clearly separated from core correctness.
141. [P6][Core §§10.10] Add tests for speed changes crossing audible/muted boundaries while emulation continues.
142. [P6][Core §§36,49.1] Run deterministic mixer tests on at least two compilers/architectures where available.
143. [P6][Core §49.1] Close the Phase-6 gate.

## Phase 7 — Tape subsystem: TAP, Warajevo TAP, TZX, WAV, normal and instant modes

144. [P7][Core §§23.12,49.2] Complete/freeze `design/media-format-support.md` tape block/variant matrix before parser implementation — done when every relevant TZX block is classified and TAP/WAV semantics are explicit.
145. [P7][Core §§23.1] Implement the tape object/state model independent of host file handles — done when an in-memory synthetic tape can be mounted headlessly.
146. [P7][Core §§23.2] Implement normal cassette playback as timed EAR level transitions on canonical master time — done when ROM-loader-visible edges match synthetic fixtures.
147. [P7][Core §§23.3] Verify tape emulated-time behavior is invariant across runtime speed settings while wall-clock playback scales with host pacing.
148. [P7][Core §§23.4] Implement SAVE/MIC tape output as emulated-time transitions with host recording/export outside the core — done when ROM SAVE produces deterministic pulse traces.
149. [P7][Core §§23.12] Implement standard Spectrum TAP parsing with strict truncation/checksum/error handling from the frozen matrix.
150. [P7][Core §§23.12] Implement standard TAP writing for supported Spectrum SAVE output — done when write/read round trips preserve defined block semantics.
151. [P7][Core §§23.12,41] Implement Warajevo-native TAP compatibility parsing only after migration-ledger clearance of relevant reference behavior — done when known native samples load through controlled code paths.
152. [P7][Core §§23.12] Implement Warajevo-native TAP writing if required by the frozen format matrix — done when round-trip tests pass without conflating it with standard TAP.
153. [P7][Core §§23.12] Implement TZX container/block parser according to the frozen supported/ignored/unsupported classification — done when unsupported blocks produce controlled errors.
154. [P7][Core §§23.12] Implement required TZX timing/control block execution into the normal tape timeline — done when timing fixtures reproduce expected EAR transitions.
155. [P7][Core §§23.12] Implement deterministic WAV/PCM decode into tape edges for the accepted container/sample forms — done when identical WAV input yields identical edges across hosts.
156. [P7][Core §§23.5,23.9] Implement explicit Normal versus Instant/Trap loading-mode state with Normal as default.
157. [P7][Core §§23.5,23.9] Implement trap eligibility detection only for frozen safe loader paths — done when custom/unsupported loaders fall back rather than being guessed.
158. [P7][Core §§23.6] Implement instant-load state mutation so supported successful loads match the Normal path’s required post-load Spectrum-visible state.
159. [P7][Core §§23.6,23.7] Add equivalence tests for loaded bytes, registers/return state, tape position, and defined error outcomes, while documenting intentionally transient differences.
160. [P7][Core §§23.8] Preserve a path for accelerated execution of the authentic loader when exact transient behavior is needed instead of unsafe trap shortcuts.
161. [P7][Core §§23.10] Implement trap failure/fallback semantics so unsupported detection never partially mutates machine/tape state.
162. [P7][Core §§23.4.1,24.1] Add cassette-socket ownership arbitration that supports normal Tape now and reserved `EAR_MIC` ownership later, with no simultaneous EAR/MIC drivers.
163. [P7][Core §§23.4.1] Add tests proving ordinary Tape transport is unavailable when the upstream networking mode is `EAR_MIC`, without implementing Architecture #3 behavior.
164. [P7][Core §§46] Fuzz TAP/TZX/WAV parsers for malformed lengths, overflows, invalid control flow, and truncation under sanitizers.
165. [P7][Core §§23.13] Run Normal-mode ROM-loader, border/loading, error, and multiple-runtime-speed regressions.
166. [P7][Core §§23.13] Run Instant/Trap equivalence and safe-fallback regressions.
167. [P7][Core §§36.2] Run any available private difficult tape/TZX media as development regression without imposing metadata requirements.
168. [P7][Core §49.1] Close the Phase-7 gate.

## Phase 8 — Snapshot subsystem

169. [P8][Core §§25.3] Complete/freeze the byte-level SNA/Z80 variant matrix in `design/media-format-support.md` before snapshot implementation.
170. [P8][Core §§25] Implement a validated temporary snapshot-state object separate from the live machine — done when parsers can fail without touching live state.
171. [P8][Core §§25.1] Implement SNA 48K loading according to the frozen byte-level semantics.
172. [P8][Core §§25.1] Implement SNA 48K saving with exact defined state representation.
173. [P8][Core §§25.1] Implement SNA 128K loading/saving scaffolding so the parser can represent paging state, with full behavior validated after Phase 9.
174. [P8][Core §§25.1] Implement Z80 v1-class loading.
175. [P8][Core §§25.1] Implement Z80 v2-class loading and the canonical initial v2-class writer.
176. [P8][Core §§25.1] Implement Z80 v3-class loading for frozen supported variants without making it the default writer.
177. [P8][Core §§25.2] Implement atomic snapshot commit: validate structure/ranges/model/state first, then replace live machine in one controlled step.
178. [P8][Core §§24.1,25] Serialize/validate `networking_mode` in WZSN-native state so illegal Interface-1/Ear+Mic combinations cannot exist.
179. [P8][Core §§25] Add controlled errors when historical formats cannot represent active WZSN-specific deterministic state rather than silently discarding it.
180. [P8][Core §§46] Fuzz SNA/Z80 parsers for malformed compression, lengths, paging fields, and unsupported variants.
181. [P8][Core §§25,36] Add cross-host load/save/atomic-failure regression tests including wrong-model and truncated inputs.
182. [P8][Core §49.1] Close the Phase-8 gate.

## Phase 9 — 128K PAL profile, paging, ULA differences, AY-3-8912

183. [P9][Core §§9] Freeze/verify the required 128K PAL timing/profile evidence needed for implementation while preserving the 48K master-tick architecture.
184. [P9][Core §§9,12] Implement 128K memory banks and paging state through the machine-profile/bus model.
185. [P9][Core §§9] Implement 128K paging-port decode and lock semantics from evidence.
186. [P9][Core §§15] Implement 128K ULA timing/profile differences without duplicating the raster engine.
187. [P9][Core §§10] Verify master-tick/T-state/frame relationships for the 128K profile.
188. [P9][Core §§20.3] Implement AY-3-8912 register select/write timing as deterministic machine events.
189. [P9][Core §§20.3] Implement AY tone generators.
190. [P9][Core §§20.3] Implement AY noise generator.
191. [P9][Core §§20.3] Implement AY envelope generator and register edge cases.
192. [P9][Core §§20.4] Integrate AY channels into the already frozen deterministic mixer/analog model.
193. [P9][Core §§20.5] Add AY internal state to serialization/hashing and canonical audio regression.
194. [P9][Core §§25] Complete 128K SNA snapshot load/save validation against the implemented paging/AY state.
195. [P9][Core §§25] Complete 128K Z80 snapshot variant validation against the implemented profile.
196. [P9][Core §§36] Add deterministic 128K paging/interrupt/raster test fixtures.
197. [P9][Core §§38] Add deterministic AY register-write and PCM-hash tests.
198. [P9][Core §§39] Differential-test representative 128K state behavior against preserved Warajevo where applicable and hardware/reference evidence where it differs.
199. [P9][Core §49.1] Close the Phase-9 gate.

## Phase 10 — Networking-mode arbitration, Interface 1, Microdrive, original ZX Net

200. [P10][Core §§24.6,49.2] Complete/freeze `design/interface1-microdrive-zxnet.md` with I/O decode, ROM paging, registers/latches, MDR interpretation/timing, ROM variants, ZX Net state transitions, serialization fields, and regression authorities.
201. [P10][Core §§51] Resolve Interface 1 ROM test/redistribution handling before Interface-1 implementation code depends on ROM bytes.
202. [P10][Core §§24.1] Implement the single deterministic networking-mode enum/state `NONE`, `INTERFACE1`, `EAR_MIC` with no independent booleans.
203. [P10][Core §§24.1] Implement validation rules that prohibit simultaneous Interface-1 and Ear+Mic state structurally.
204. [P10][Core §§24.1] Implement upstream `EAR_MIC` as a reserved/unavailable mode until Architecture #3 certifies the active 48K Issue-2 profile; do not implement router behavior.
205. [P10][Core §§24.1,26] Implement the cold machine-reconfiguration operation used when switching networking modes so old RAM/hooks/device state are discarded while application pause/run state is preserved.
206. [P10][Core §§24.1] Add transition tests for NONE↔INTERFACE1 and rejected EAR_MIC activation, proving no partial state mutation on failure.
207. [P10][Core §§24.2,24.4] Implement Interface 1 ROM-facing paging/latch behavior through the deterministic bus.
208. [P10][Core §§24.4] Implement Interface 1 machine-visible registers/state required by the frozen companion spec.
209. [P10][Core §§24.3] Implement Microdrive cartridge image parsing/validation for the frozen MDR interpretation.
210. [P10][Core §§24.3] Implement deterministic Microdrive motor/cartridge position and sector/header/data visibility.
211. [P10][Core §§24.3] Implement Microdrive write-protect and write behavior with application-controlled flush semantics.
212. [P10][Core §§24.3] Implement atomic/controlled MDR image save/flush so failures cannot silently corrupt the original image.
213. [P10][Core §§28.7] Implement cross-process exclusive writer ownership for writable media images, including MDR — done when a second process is refused or safely read-only according to policy.
214. [P10][Core §§24.5] Implement original ZX Net deterministic device/state-machine behavior independent of any future host multi-instance transport.
215. [P10][Core §§24.5] Build a deterministic single-process/loopback ZX Net harness so network-device correctness does not depend on host packet timing.
216. [P10][Core §§24.5] Leave any future cross-process ZX Net transport outside the core behind an orchestrator normalization boundary; do not implement unsynchronized host-time mutation.
217. [P10][Core §§8,24] Add Interface 1, Microdrive, ZX Net, and networking-mode fields to canonical serialization/hashing.
218. [P10][Core §§24.1] Add dirty-Microdrive guard semantics to networking-mode changes so a mode switch cannot silently discard unflushed writable media state.
219. [P10][Core §§36] Add deterministic Interface-1 paging, MDR round-trip, and ZX Net loopback regression tests.
220. [P10][Core §§39] Differential-test preserved Warajevo Interface-1/Microdrive behavior where valid, with hardware/reference evidence winning on conflicts.
221. [P10][Core §§46] Fuzz malformed MDR images and invalid device transitions under sanitizers.
222. [P10][Core §49.1] Close the Phase-10 gate.

## Phase 11 — Monitor/debugger, ZX Printer, remaining REQUIRED legacy workflows

223. [P11][Core §§41,49.2] Inventory every historical monitor/debugger function at function level and classify REQUIRED/LATER/REPLACE/DROP with regression strategy before implementation tickets proceed.
224. [P11][Core §§41,49.2] Inventory every historical ZX Printer function at function level and classify REQUIRED/LATER/REPLACE/DROP with regression strategy.
225. [P11][Core §§44] Define the shared debugger/inspection API over one live machine implementation; do not create a second CPU/memory model.
226. [P11][Core §§44] Implement read-only CPU register inspection.
227. [P11][Core §§44] Implement controlled register mutation through debugger/state APIs with deterministic traceability.
228. [P11][Core §§44] Implement memory inspection/read tools using bus/state boundaries appropriate for debugging.
229. [P11][Core §§44] Implement controlled memory mutation with explicit paused/safe-state policy.
230. [P11][Core §§44] Implement breakpoint support at the minimum architecture-required monitor level.
231. [P11][Core §§44] Implement single-step/continue behavior using the existing CPU/scheduler rather than a debugger-specific executor.
232. [P11][Core §§44] Implement trace/log export required for regression/debugging without host data entering canonical machine state.
233. [P11][Core §§2.5] Implement the ZX Printer deterministic peripheral state and Spectrum-visible timing/behavior required by the frozen inventory.
234. [P11][Core §§26] Implement host presentation/export of captured printer output outside the deterministic peripheral core.
235. [P11][Core §§49.1] Add regression tests for every REQUIRED monitor/debugger and ZX Printer workflow identified by the inventories.
236. [P11][Core §49.1] Close the Phase-11 gate and verify no REQUIRED legacy item from Core §2.5 remains unimplemented or unclassified.

## Phase 12A — UI implementation gate and command-registry foundation

237. [P12][UI §49][Core §51] Select and freeze the exact C-compatible UI toolkit/revision before UI implementation begins.
238. [P12][UI §49] Freeze the per-platform integration approach for Windows, Linux/X11, and macOS.
239. [P12][UI §49] Freeze font/text rendering strategy.
240. [P12][UI §49] Freeze native-versus-in-window menu presentation per platform.
241. [P12][UI §49] Freeze file-dialog implementation.
242. [P12][UI §§43,49] Freeze/document accessibility support supplied by the selected toolkit and any project-owned gaps.
243. [P12][UI §§44,49] Freeze window/panel persistence approach including interprocess-safe settings storage.
244. [P12][UI §§4,49] Freeze the exact application command-registry C API.
245. [P12][UI §§4,49] Freeze the exact command result/error representation used by GUI, tests, and later Telnet projection.
246. [P12][UI §49] Record all Phase-12 gate decisions in the design/implementation notes and block UI coding until the gate is review-approved.
247. [P12][UI §§4.1–4.3] Implement the shared command registry with stable lowercase dotted IDs, metadata, labels, descriptions, parameter schemas, result schemas, handlers, and permission classes.
248. [P12][UI §§4.4] Implement separation of semantic command execution from GUI parameter acquisition/file dialogs.
249. [P12][UI §§4.5,40] Implement serialized dispatch for state-changing application commands on the machine-owning thread.
250. [P12][UI §§4.6] Add command-equivalence tests proving GUI/test invocation routes to the same semantic handler.
251. [P12][UI §§4.7] Implement menu/action nodes that bind fixed arguments to parameterized commands rather than duplicating handlers.
252. [P12][UI §7.1] Register all required top-level menu-node IDs.
253. [P12][UI §7.1] Register all required File/Machine action IDs.
254. [P12][UI §7.1] Register all required Tape/Microdrive/ZX Printer action IDs.
255. [P12][UI §7.1] Register all required View/Tools/Compatibility action IDs.
256. [P12][UI §7.1] Register all required Settings/Help action IDs.
257. [P12][UI §7.1] Register all required non-menu semantic IDs including pause/resume/model/speed/networking/tape-mode/MDR/screenshot-temp.
258. [P12][UI §5] Implement the six remote permission classes as command metadata even though Telnet transport is Phase 15.
259. [P12][UI §5.7] Implement discoverability metadata independently from remote executability so denied commands remain describable later.
260. [P12][UI §§40] Implement common enabled/disabled predicates and stable disabled reasons in the registry.
261. [P12][UI §§46] Expose a direct application-test projection over the registry without Sokol/Telnet dependency.
262. [P12][UI §§47] Add stable-ID uniqueness and required-ID completeness tests.
263. [P12][UI §§47] Add tests that fixed menu actions and parameterized commands converge on one handler.
264. [P12][UI §§48] Complete the command-registry subset of UI acceptance before building higher-level surfaces.

## Phase 12B — Main window, menus, toolbar, status, basic machine commands

265. [P12][UI §§6,7] Implement the main application layout with menu bar/platform equivalent, compact toolbar, Spectrum viewport, status area, and on-demand panels.
266. [P12][UI §7] Implement canonical top-level menus exactly as File, Machine, Media, View, Tools, Settings, Help.
267. [P12][UI §§7.2] Implement Help and About actions with platform-standard relocation allowed without changing semantic IDs.
268. [P12][UI §§8.5] Implement local `File > Quit` bound to `application.quit`, including orderly host shutdown and dirty-media resolution without treating machine reset as application exit.
269. [P12][UI §§9.1] Implement Model UI actions bound to `machine.model.set` for 48K/128K with defined reset workflow.
270. [P12][UI §§9.2] Implement Reset UI action bound to `machine.reset`, preserving host application/service state.
271. [P12][UI §§9.3] Implement state-sensitive Pause/Resume menu/toolbar action backed by explicit `machine.pause` and `machine.resume` handlers.
272. [P12][UI §§9.4] Implement all seven emulation-speed menu values bound to `machine.speed.set`.
273. [P12][UI §§24.1] Implement the toolbar speed selector and immediate status/audio-muted reflection.
274. [P12][UI §24] Implement the canonical toolbar controls in the specified semantic set/order appropriate to the chosen toolkit.
275. [P12][UI §§18.1] Implement Fullscreen as host presentation only with zero canonical-state mutation.
276. [P12][UI §§18.2,25] Implement the optional Machine / Media Status panel and its View toggle, showing model, speed/audio, tape, all eight Microdrives, networking mode, and remote-control state without obsolete host trivia.
277. [P12][UI §§18.3,23.1] Implement `View > Display Settings...` as a front end to the shared Display settings surface rather than a second display-settings implementation.
278. [P12][UI §§25.1] Implement status-line display of active model, speed, pause/run state, audio state, tape, primary Microdrive, networking mode, and Control Port state placeholder.
279. [P12][UI §§25.1] Make the status Control Port label always render either `Control Port: <number>` or `Control Port: unavailable`, sourcing live transport state when Phase 15 exists.
280. [P12][UI §§24.3,40] Bind toolbar/menu enabled state exclusively to registry predicates.
281. [P12][UI §§47] Add regression tests for model, reset, pause/resume, speed, fullscreen, toolbar state, and status correctness.
282. [P12][UI §§48] Complete basic-main-window acceptance subset.

## Phase 12C — File, screenshots, recent files, snapshots

283. [P12][UI §§8.1] Implement `Open / Run...` native file-dialog workflow bound to `file.open_run`.
284. [P12][UI §§8.1] Implement format routing from Open/Run to tape, snapshot, or MDR subsystem without format-specific top-level menus.
285. [P12][UI §§8.1,22.2] Route formats requiring conversion to Compatibility Tools or controlled unsupported errors; never silently lossy-convert.
286. [P12][UI §§8.2] Implement local Recent-file metadata whose entries call `file.open_run` rather than creating new commands.
287. [P12][UI §§8.2,39.1] Ensure dynamic Recent paths are not part of remotely discoverable registry metadata.
288. [P12][UI §§8.3] Implement Load Snapshot workflow with atomic-load error reporting.
289. [P12][UI §§8.3] Implement Save Snapshot and Save Snapshot As parameter-acquisition behavior.
290. [P12][UI §§13.1] Implement `Tools > Snapshot Inspector...` using shared snapshot/debugger inspection machinery.
291. [P12][UI §§13.2] Display snapshot format/version, implied model, registers, paging, AY state, pages, and warnings as applicable.
292. [P12][UI §§13.3] Route snapshot state editing to shared debugger/state-editor machinery; do not duplicate editors.
293. [P12][Core §19.6][UI §8.4] Implement the shared screenshot capture service from the completed logical/presentation raster, excluding all host chrome.
294. [P12][UI §§8.4] Implement GUI Save Screenshot with user-selected destination and PNG encoding through the shared service.
295. [P12][UI §§24.2] Bind toolbar Screenshot to the GUI screenshot workflow rather than the Telnet temp-path behavior.
296. [P12][Core §28.7][UI §44.1] Make output-file creation/atomic replacement safe against concurrent WZSN processes.
297. [P12][UI §§45] Add cancel/error tests proving file-dialog cancellation and screenshot/snapshot failures leave machine state unchanged.
298. [P12][UI §§47] Add screenshot pixel-source equivalence tests independent of GUI chrome.
299. [P12][UI §§47] Add snapshot inspector and snapshot workflow regression tests.
300. [P12][UI §§48] Complete File/Snapshot/Screenshot acceptance subset.

## Phase 12D — Tape UI and Tape Manager

301. [P12][UI §§10,11.1] Implement Tape Insert quick action with applicability/disabled-state handling.
302. [P12][UI §§11.2] Implement Tape Eject quick action.
303. [P12][UI §§11.3] Implement Normal/Instant loading-mode selector bound to `media.tape.loading_mode.set`, with Normal visibly default.
304. [P12][UI §§11.4] Implement compact toolbar Tape control using shared commands.
305. [P12][UI §§12.1] Implement Tape Manager block list/details presentation from the media subsystem.
306. [P12][UI §§12.2] Implement required block reorder/extract/delete/add/edit/change-position operations through one media-manager backend.
307. [P12][UI §§12.3] Surface Warajevo-native exclude/linearize/implode/decompress/efficiency operations only when applicable, not as standard-TAP/TZX ordinary controls.
308. [P12][UI §§12.4] Replace legacy print-to-screen with manager presentation and implement optional portable Export Tape Report only if retained in backlog.
309. [P12][UI §§12.5] Enforce atomic failure behavior for tape mutation so the original image remains unchanged unless commit succeeds.
310. [P12][UI §§20.1–20.8] Verify every legacy TapeFiles operation is represented by Tape Manager, Compatibility Tools, LATER, REPLACE, or DROP disposition with no hidden missing command.
311. [P12][UI §§47] Add Tape Manager enabled-state, mutation, cancel, malformed-media, and atomic-error regression tests.
312. [P12][UI §§48] Complete Tape UI acceptance subset.

## Phase 12E — Microdrive, Printer, debugger, compatibility tools

313. [P12][UI §§14] Implement eight Microdrive slot controls using `media.microdrive.mount/eject/set_default` semantics.
314. [P12][UI §§14] Implement toolbar `MDV 1` compact control without creating a one-drive-only backend.
315. [P12][UI §§15.1] Implement Microdrive Manager overview showing image identity, logical name, sector count, write protection, default/current state, and validation status.
316. [P12][UI §§15.2] Implement mount/eject/default/catalog/format/optimize/allocation/rename/write-protect workflows.
317. [P12][UI §§15.3] Implement logical MDR file delete/rename/hide/unhide/copy operations.
318. [P12][UI §§15.4] Implement advanced sector verify/repair/edit surfaces with dangerous raw-sector operations visually distinguished.
319. [P12][UI §§14,44.1] Enforce cross-process single-writer media ownership in UI errors and optional read-only fallback policy if frozen.
320. [P12][UI §§45.2] Require explicit local confirmation for destructive MDR operations.
321. [P12][UI §§16] Implement ZX Printer Manager presentation/export over the shared printer subsystem.
322. [P12][UI §§21] Implement Debugger/Monitor window over the shared Phase-11 debugger APIs.
323. [P12][UI §§19] Implement Diagnostics entry point for project-owned diagnostic surfaces without duplicating hardware behavior.
324. [P12][UI §§22] Implement Compatibility Tools container and availability rules.
325. [P12][UI §§22.2] Distinguish native load/run from explicit conversion workflows.
326. [P12][UI §§22.3] Require clear loss disclosure before lossy conversions.
327. [P12][UI §§20.9–20.18] Map all snapshot/MDR/Dock/database legacy conversion entries to the frozen dispositions; do not implement LATER tools merely for menu completeness.
328. [P12][UI §§17] Keep Dock Cartridge UI absent until Timex/DCK support exists.
329. [P12][UI §§47] Add Microdrive Manager, destructive-confirmation, printer, debugger, and tool-availability regression tests.
330. [P12][UI §§48] Complete media-manager/tools acceptance subset.

## Phase 12F — Settings, networking radio group, focus, accessibility, persistence

331. [P12][UI §§23.1] Implement Display settings for host presentation only.
332. [P12][UI §§23.2] Implement Audio settings for host output preferences without changing emulated audio semantics.
333. [P12][UI §§23.3] Implement Input settings for host keyboard/Kempston mappings with no host identity leaking into core.
334. [P12][UI §§23.4] Implement ROM/Firmware settings using explicit ROM identities and core licensing/hash policy.
335. [P12][UI §§23.5] Implement the single Networking radio group with exactly `None`, `Interface-1`, `Ear+Mic` bound to `machine.networking.set`.
336. [P12][UI §§23.5] Make Interface-1 and Ear+Mic mutually exclusive by data model, not checkbox validation.
337. [P12][UI §§23.5] Present Ear+Mic as unavailable with a stable disabled reason until Architecture #3 certifies the current 48K Issue-2 profile; do not implement Architecture #3.
338. [P12][UI §§23.5] When Networking changes, invoke the shared cold-reconfiguration handler rather than hot-swapping machine devices.
339. [P12][UI §§23.5,45] Block leaving Interface-1 when dirty Microdrive state requires local resolution; do not silently discard writes.
340. [P12][UI §§23.5] Disable ordinary Tape signal use whenever upstream core state is EAR_MIC, while leaving the actual Architecture-3 bootstrap/network operations unimplemented.
341. [P12][UI §§23.6] Implement ZX Printer settings over the shared peripheral configuration.
342. [P12][UI §§23.7] Implement the Telnet/Remote Control settings/status page shell, capable of showing enabled architecture state, base/search range, actual Control Port, IP-family state, client state, and plaintext/no-auth warning once Phase 15 transport is present.
343. [P12][UI §§42.1] Implement keyboard focus ownership so viewport typing reaches Spectrum input while application shortcuts remain explicit.
344. [P12][UI §§42.2] Implement dialog/text-control focus so typing is consumed locally and cannot leave stale Spectrum keys.
345. [P12][UI §§43] Ensure all required workflows are keyboard-operable and expose visible focus/non-color-only status.
346. [P12][UI §§43] Connect selected toolkit accessibility APIs where available and document any unavoidable limitations.
347. [P12][UI §§44] Persist approved host preferences only; keep deterministic machine state out of preferences.
348. [P12][UI §§44.1] Implement interprocess-safe settings writes using the frozen atomic/locking policy and merge/conflict rules.
349. [P12][UI §§44.1] Ensure dynamically selected Control Port is never persisted as a preference.
350. [P12][UI §§45.4] Add tests for every “no silent fallback” rule, including no model/port/tape-mode/lossy-conversion/output-path/overwrite surprises.
351. [P12][UI §§47] Run keyboard-only, focus-transition, accessibility, networking-radio, and settings-concurrency regression tests.
352. [P12][UI §§48] Complete Settings/Accessibility/Persistence acceptance subset.

## Phase 12G — Complete legacy disposition and Phase-12 exit

353. [P12][UI §20] Create a backlog trace row for every legacy menu/facility listed in UI §20, pointing to its implemented task or explicit LATER/REPLACE/DROP disposition.
354. [P12][UI §§20.19] Verify legacy Setup mappings resolve to modern Machine/Settings controls and no DOS-era hardware compensation control is accidentally reproduced.
355. [P12][UI §§20.20–20.21] Verify DOS shell/directory/heap/child-emulator UI is absent and Start/Names/status behavior is represented by modern surfaces.
356. [P12][UI §§45] Run cancellation, non-destructive failure, destructive confirmation, and state-preservation tests across all initial UI workflows.
357. [P12][UI §§46] Run GUI/toolbar/direct-registry equivalence tests for all commands implemented before Telnet integration.
358. [P12][UI §§47] Run the complete Phase-12-required UI regression list excluding only tests that intrinsically require Phase-15 Telnet transport.
359. [P12][UI §§48] Evaluate every UI acceptance criterion applicable before Phase 15 and record pass/fail evidence.
360. [P12][Core §49.1][UI §49] Close the Phase-12 gate — done when UI/registry/workflow/accessibility acceptance passes without canonical core divergence.

## Phase 13 — Cross-platform build and packaging

361. [P13][Core §§28.1–28.5,33,34] Produce release-mode Windows x86-64 build with no project-supplied multimedia shared-library dependency.
362. [P13][Core §§28.1–28.5,33,34] Produce release-mode Linux x86-64 build using the frozen X11/system graphics/ALSA path.
363. [P13][Core §§28.1–28.5,33,34] Produce release-mode Linux AArch64 build and run it on real AArch64 Linux hardware, on project-controlled/self-hosted hardware when hosted CI is insufficient.
364. [P13][Core §§28.1–28.5,33,34] Produce release-mode macOS AArch64 build using Metal/CoreAudio host integration.
365. [P13][Core §§34,35] Maintain/test secondary macOS x86-64 only while the project continues to advertise it.
366. [P13][Core §51] Freeze initial Linux package formats before packaging scripts depend on them.
367. [P13][Core §§28.6] Define external configuration/data locations without embedding ROMs or private test media in distributions.
368. [P13][Core §§28.7] Stress concurrent-process settings, output-file, and writable-media ownership on supported desktop hosts.
369. [P13][Core §§34] Run dependency inspection for every release artifact and document permitted OS/system dependencies.
370. [P13][Core §§34] Run practical launch/display/audio/input/media smoke tests on real host architecture, not only cross-compiled binaries.
371. [P13][Core §49.1] Close the Phase-13 gate.

## Phase 14 — Full timing torture and difficult-media validation

372. [P14][Core §§18,37,49] Run the full overscan/rainbow timing-torture set and capture first-divergence traces for failures.
373. [P14][Core §§18,37,49] Run the full multicolor/NIRVANA/NIRVANA+ timing-torture set.
374. [P14][Core §§18,37,49] Run BIFROST-class timing-sensitive cases.
375. [P14][Core §§39] Run documented historical Warajevo problem cases and classify every divergence against hardware/reference authority.
376. [P14][Core §§36.2] Run every usable file currently present directly in `WZSN_PRIVATE_TEST_MEDIA`, auto-detecting/attempting supported media as the private harness defines, with no metadata bureaucracy.
377. [P14][Core §§36.2] Record unexplained private-media failures as implementation defects or explicitly unsupported-format findings; do not claim full difficult-media compatibility while unexplained failures remain.
378. [P14][Core §§40] Execute the planned real-hardware timing validation set needed for first architecture-complete certification.
379. [P14][Core §§44] Add minimal deterministic regressions for every newly discovered timing bug before fixing it.
380. [P14][Core §49.1] Close the Phase-14 gate only when torture, private-media, and required hardware-reference checks have no unexplained release-blocking divergence.

## Phase 15A — Multi-instance Control Port and Telnet transport

381. [P15][Core §§55.15] Implement the small platform socket abstraction using Winsock on Windows and POSIX/BSD sockets on Linux/macOS, outside the deterministic core.
382. [P15][Core §§55.2] Implement automatic Control Port probing in strict ascending order from 30740 through 32787 inclusive.
383. [P15][Core §§55.2] Implement candidate ownership so one WZSN process owns one numeric port across supported IPv4/IPv6 families; reject split-family duplicate ownership.
384. [P15][Core §§55.2] Make bind acquisition race-safe across simultaneous processes by treating successful socket bind/listen as the reservation.
385. [P15][Core §§55.2] Implement degraded-family operation when one supported address family is unavailable for reasons other than numeric-port ownership, according to the frozen rules.
386. [P15][Core §§55.2] Implement full 2048-port exhaustion as nonfatal Control service unavailability with no probing outside the range.
387. [P15][Core §§55.2] Keep selected Control Port as session-only state and expose it to UI/status without persistence.
388. [P15][Core §§55.3] Implement exactly one active Telnet client per WZSN process, returning `BUSY\r\n` then closing a second client.
389. [P15][Core §§55.3] Keep the listener alive after client disconnect for subsequent clients.
390. [P15][Core §§55.11] Implement minimal Telnet IAC negotiation parsing, including IAC IAC, WILL/WONT/DO/DONT, subnegotiation, and rejection of unsupported options.
391. [P15][Core §§55.17] Implement bounded network buffers and malformed-input recovery with no unbounded allocation.
392. [P15][Core §§55.16] Route network-thread/nonblocking-poll output through a bounded host command/input queue; never mutate machine state directly from socket code.
393. [P15][Core §§55.18] Surface the plaintext/no-authentication security state to the application/UI.
394. [P15][Core §§55.19] Add automated first-free, simultaneous-start, IP-family, exhaustion, one-client, reconnect, malformed-input, and negotiation transport tests.
395. [P15][Core §§55.20] Complete the transport-only acceptance subset before adding keyboard/control grammar.

## Phase 15B — Telnet keyboard source and deterministic input integration

396. [P15][Core §§55.5–55.7] Add Telnet as an independent source in the host input arbiter without adding source identity to the Spectrum core API.
397. [P15][Core §§55.6] Implement OR-combined local/Telnet effective key ownership so one source cannot release another source’s held key.
398. [P15][Core §§55.7] Map the exact frozen 40-key Telnet vocabulary to Spectrum matrix keys.
399. [P15][Core §§55.8] Ensure Telnet keyboard commands never inject BASIC characters, tokens, ROM input buffers, or RAM shortcuts.
400. [P15][Core §§55.9] Apply Telnet effective transitions at orchestrator drain/current-master-tick using the same rule as local input.
401. [P15][Core §§55.11] Implement `KEY DOWN <key>`.
402. [P15][Core §§55.11] Implement `KEY UP <key>`.
403. [P15][Core §§55.11] Implement `KEY PRESS <key>` with exactly two active-machine emulated frame periods and rejection of already-held/pending keys.
404. [P15][Core §§55.11] Implement `RELEASE ALL` affecting only Telnet-owned keys.
405. [P15][Core §§55.12] Release all Telnet-owned keys on disconnect while preserving local key state.
406. [P15][Core §§55.13] Preserve local/Telnet key ownership and pending KEY PRESS releases across ordinary emulated Spectrum Reset as frozen.
407. [P15][Core §§55.14] Record/replay normalized effective key transitions/master ticks rather than raw TCP arrival timing.
408. [P15][Core §§55.19] Add local-vs-Telnet equivalence, same-key-two-source, modifier, multi-row, disconnect, KEY PRESS duration, reset, speed-change, and headless replay tests.
409. [P15][Core §§55.20] Complete the Telnet keyboard acceptance subset.

## Phase 15C — Telnet application grammar and command-registry projection

410. [P15][UI §§28.1] Implement ASCII line-oriented application command input accepting CRLF and bare LF.
411. [P15][UI §§28.2] Enforce the 1024-byte decoded line limit with `ERR LINE_TOO_LONG`, discard-to-terminator, and deterministic recovery.
412. [P15][UI §§28.3] Implement whitespace tokenization plus double-quoted arguments and only the frozen `\\` and `\"` escapes.
413. [P15][UI §§28.3] Prohibit shell/environment/wildcard/command-substitution interpretation of Telnet arguments.
414. [P15][UI §§27.1] Implement canonical keyboard-command success/error responses over the transport.
415. [P15][UI §§29.1] Implement `HELP` from the frozen command/help surface.
416. [P15][UI §§29.2] Implement `STATUS`, including PROTOCOL/CONTROL_PORT/IP-family/CLIENT/MODEL/STATE/SPEED/AUDIO/NETWORKING using the exact frozen field names and values without arbitrary host path disclosure.
417. [P15][UI §30] Implement `RESET` alias through `machine.reset` and exact response/connection-preservation semantics.
418. [P15][UI §31] Implement idempotent `PAUSE` and `RESUME` aliases through explicit registry commands.
419. [P15][UI §32] Implement `MODEL 48K` and `MODEL 128K` aliases through the shared model-change/reset workflow.
420. [P15][UI §33] Implement all seven `SPEED` aliases through `machine.speed.set`.
421. [P15][UI §§34.1–34.8] Implement `SCREENSHOT` through `host.screenshot.temp` using OS temp, shared raster/PNG service, exact timestamp filename, exclusive no-clobber creation, collision suffix, and absolute-path success response.
422. [P15][UI §35.1] Implement `MENU` root discovery from the shared registry.
423. [P15][UI §35.2] Implement `MENU TREE` with the exact ITEM record format and END terminator.
424. [P15][UI §35.3] Implement `MENU <id>` node/child/metadata inspection.
425. [P15][UI §35.4] Implement case-insensitive `MENU FIND <text>` over static ID/label/description metadata without leaking Recent paths.
426. [P15][UI §36] Implement `DESCRIBE <command-id>` metadata/schema output without dynamic private host data.
427. [P15][UI §§37] Implement generic `DO <command-id> [arguments]` with parse→resolve→validate→availability→permission→enqueue→result ordering.
428. [P15][UI §§37.2] Implement exact disabled-command response including stable reason.
429. [P15][UI §§37.3] Implement exact denied-command response including remote permission class.
430. [P15][UI §§37.4] Implement generic success response plus command-specific optional fields.
431. [P15][UI §§37.5] Verify special aliases are grammar/response conveniences only and contain no private implementations.
432. [P15][UI §§38] Implement deterministic response framing and controlled unknown/bad-argument errors.
433. [P15][UI §§39] Enforce initial unauthenticated remote policy: only REMOTE_SAFE executable; HOST_READ/HOST_WRITE/MEDIA_DESTRUCTIVE/APPLICATION_CONTROL/LOCAL_ONLY denied.
434. [P15][UI §§5.1,23.5,37.1] Verify `machine.networking.set` is `REMOTE_SAFE` only when the requested cold reconfiguration needs no host write/confirmation; dirty Interface-1 media must surface the registry disabled reason and prevent remote switching.
435. [P15][UI §§39.1] Verify MENU/DESCRIBE/STATUS cannot expose arbitrary absolute media paths or Recent history; retain the explicitly frozen screenshot-path exception.
436. [P15][UI §§39.2–39.3] Verify destructive media operations and application quit remain denied remotely.
437. [P15][UI §§40] Make GUI and Telnet report identical command enabled/disabled state from registry predicates.
438. [P15][UI §§23.7,25.1] Connect live Control Port/listener/client state to settings page and always-visible status line.
439. [P15][UI §§42.3] Verify local GUI focus changes never disable independently owned Telnet keyboard input.
440. [P15][UI §§46] Run GUI/toolbar/Telnet/direct-registry equivalence tests for Reset, Speed, Pause/Resume, Model, and screenshot source.
441. [P15][UI §§47] Run the complete Telnet HELP/STATUS/parser/MENU/DESCRIBE/DO/alias/permission/screenshot regression list.
442. [P15][UI §§48,50][Core §49.1] Evaluate all remaining UI acceptance criteria and close the Phase-15 gate only when both Core §55 and UI §50 contracts pass.

## Phase 16 — Optimization after correctness lock

443. [P16][Core §43] Establish performance baselines for CPU execution, raster generation, audio mixing, tape, snapshots, and UI presentation before optimization.
444. [P16][Core §43] Profile with deterministic correctness checks enabled; identify bottlenecks without changing architectural boundaries.
445. [P16][Core §43] Optimize one subsystem at a time with before/after canonical state/raster/audio regression evidence.
446. [P16][Core §§7] Reject optimizations that introduce undefined behavior, host-width assumptions, or compiler-specific deterministic results.
447. [P16][Core §§6,31] Reject optimizations that make machine results depend on thread scheduling or host wall clock.
448. [P16][Core §43] Add performance regressions/benchmarks for accepted optimizations so later changes do not silently erase gains.
449. [P16][Core §49.1] Close the Phase-16 gate when every optimization in the release has correctness evidence.

## Final Architecture-#1/#2 acceptance and release-readiness sweep

450. [FINAL][Core §50] Walk every Core/System acceptance criterion and attach concrete test/build/evidence references; no criterion may be marked satisfied by prose alone.
451. [FINAL][UI §48] Walk every UI acceptance criterion and attach concrete regression/manual-accessibility evidence as applicable.
452. [FINAL][Core §2.5] Reconcile the final backlog against the legacy-feature disposition and verify every REQUIRED item is implemented/tested and every LATER/REPLACE/NOT-INITIAL item remains correctly scoped.
453. [FINAL][UI §20] Reconcile every historical UI/menu item against its implemented modern destination or explicit disposition.
454. [FINAL][Core §§34,35] Run the required compiler/platform matrix and investigate every deterministic disagreement as a defect.
455. [FINAL][Core §§36–40] Run deterministic, video, audio, differential, private difficult-media, and required real-hardware validation suites appropriate to the release claim.
456. [FINAL][Core §§28,47,48] Audit release artifacts for dependency, license/notice, ROM exclusion, private-test-media exclusion, and single-program-binary requirements.
457. [FINAL][Core §§28.7][UI §44.1] Run multi-instance stress tests for Control Port allocation, settings writes, screenshot/output creation, and writable-media ownership.
458. [FINAL][Core §§30,55][UI §§4,40,46] Verify no GUI/Telnet/test front end bypasses the shared application/orchestrator command boundary or mutates deterministic machine state from the wrong thread.
459. [FINAL][Core §§5,6,26,45] Run a dependency-boundary audit proving the deterministic core remains usable headlessly and contains no Sokol/socket/OS UI dependency.
460. [FINAL][Core §49][UI §49–50] Confirm every mandatory phase gate is closed with evidence and no deferred decision was silently chosen inside implementation code.
461. [FINAL] Tag the first Architecture-#1/#2 implementation-complete candidate only after Tasks 450–460 are clean; Architecture #3 remains a separate subsequent program.
