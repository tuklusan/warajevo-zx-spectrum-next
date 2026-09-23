<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# UI acceptance evidence audit

Task 451 audit is complete; UI acceptance is not. PASS requires direct evidence for the full criterion; PARTIAL records narrower evidence; OPEN means required evidence is missing. The four-runner DIZZY4K proof exercised the GUI application through its Telnet surface and captured a Spectrum screenshot (run 35890238325). The six-case application-command boundary proof adds hosted evidence for shared GUI menu, toolbar, test, and Telnet dispatch plus host-only state preservation (run `35932389804`, commit `1b4a9bc`). These runs do not close the full UI regression/accessibility contract.

| # | UI §48 criterion | Status | Evidence or outstanding proof |
|---:|---|---|---|
| 1 | the semantic top-level menu tree is `File`, `Machine`, `Media`, `View`, `Tools`, `Settings`, `Help`; | OPEN | No criterion-specific proof is recorded. |
| 2 | the running Spectrum display is the primary application surface; | PARTIAL | Partial: four-runner GUI/Telnet smoke produced the Spectrum screenshot; the complete primary-surface contract lacks UI regression coverage (run `35890238325`). |
| 3 | one shared command registry owns every cross-front-end semantic operation; | PARTIAL | Hosted application-command boundary case exercises one registry across GUI menu, toolbar, application-test, and Telnet reset projections (run `35932389804`, commit `1b4a9bc`); complete semantic command coverage remains open. |
| 4 | stable command IDs are unique and independent of cosmetic GUI wording; | OPEN | No criterion-specific proof is recorded. |
| 5 | GUI menus, toolbar, Telnet control, and application tests use shared semantic handlers rather than private duplicate implementations; | PARTIAL | Hosted boundary proof shows GUI menu, toolbar, application-test, and Telnet reset projections invoke the shared registered handler; complete command coverage and production UI interaction remain open (run `35932389804`, commit `1b4a9bc`). |
| 6 | command availability predicates and disabled reasons come from the registry; | OPEN | No criterion-specific proof is recorded. |
| 7 | Telnet reports the same availability state as the GUI; | OPEN | No criterion-specific proof is recorded. |
| 8 | remote permission class is independent from application availability; | OPEN | No criterion-specific proof is recorded. |
| 9 | the initial Telnet service allows only `REMOTE_SAFE` command execution; | PARTIAL | Partial: safe Telnet keyboard/speed/screenshot commands succeeded; denied command classes are not comprehensively tested (run `35890238325`). |
| 10 | the entire command/menu tree remains discoverable over Telnet even when a command is denied remotely; | OPEN | No criterion-specific proof is recorded. |
| 11 | `Open / Run...` is the universal initial user entry for supported media; | OPEN | No criterion-specific proof is recorded. |
| 12 | Tape Insert/Eject/Loading Mode are direct quick operations; | OPEN | No criterion-specific proof is recorded. |
| 13 | Normal tape loading is visibly the default; | OPEN | No criterion-specific proof is recorded. |
| 14 | detailed tape manipulation lives in Tape Manager; | OPEN | No criterion-specific proof is recorded. |
| 15 | detailed Microdrive manipulation lives in Microdrive Manager; | OPEN | No criterion-specific proof is recorded. |
| 16 | snapshots use load/save semantics rather than persistent select/unselect; | OPEN | No criterion-specific proof is recorded. |
| 17 | register/hardware/memory editing reuses debugger/state-inspection machinery; | OPEN | No criterion-specific proof is recorded. |
| 18 | legacy conversion features are grouped under Compatibility Tools; | OPEN | No criterion-specific proof is recorded. |
| 19 | historical DOS shell/directory/heap/child-EXE UI is absent; | OPEN | No criterion-specific proof is recorded. |
| 20 | the historical database application is not reproduced as an initial top-level subsystem; | OPEN | No criterion-specific proof is recorded. |
| 21 | Dock UI is absent until Timex/DCK support is actually implemented; | OPEN | No criterion-specific proof is recorded. |
| 22 | the toolbar contains the canonical quick controls in Section 24; | OPEN | No criterion-specific proof is recorded. |
| 23 | the emulation-speed selector exposes exactly 25, 50, 100, 200, 400, 800, and Unlimited; | OPEN | No criterion-specific proof is recorded. |
| 24 | speed state is visible and audio-muted-by-speed state is visible; | OPEN | No criterion-specific proof is recorded. |
| 25 | Machine Reset invokes the authentic machine reset and does not restart WZSN; | OPEN | No criterion-specific proof is recorded. |
| 26 | GUI Reset, toolbar Reset, Telnet `RESET`, and `DO machine.reset` converge on the same handler; | OPEN | No criterion-specific proof is recorded. |
| 27 | Telnet remains connected across `RESET`; | OPEN | No criterion-specific proof is recorded. |
| 28 | `PAUSE` and `RESUME` are idempotent shared-command aliases; | OPEN | No criterion-specific proof is recorded. |
| 29 | `MODEL 48K` and `MODEL 128K` use the same model-change/reset workflow as the GUI; | OPEN | No criterion-specific proof is recorded. |
| 30 | Telnet `SPEED` uses the same runtime-speed handler as GUI/toolbar control; | OPEN | No criterion-specific proof is recorded. |
| 31 | Telnet `SCREENSHOT` writes PNG to the OS temporary directory; | PARTIAL | Partial: Telnet `SCREENSHOT` returned a readable PNG path; the complete temporary-directory contract is not verified (run `35890238325`). |
| 32 | Telnet screenshots use `ZX-Screen-YYYYMMDDHHMMSSmmm.png` and collision suffixes rather than overwriting; | OPEN | No criterion-specific proof is recorded. |
| 33 | Telnet `SCREENSHOT` reports the absolute saved path only after a successful completed write; | PARTIAL | Partial: the successful screenshot response supplied a path used to retrieve the PNG; absolute-path/reporting semantics are not asserted (run `35890238325`). |
| 34 | screenshot capture excludes host application chrome and does not mutate or advance Spectrum state; | PARTIAL | Partial: the runner captured a Spectrum display PNG; no before/after canonical-state proof establishes zero mutation or advancement (run `35890238325`). |
| 35 | GUI screenshot save and Telnet screenshot use the same capture/PNG path; | OPEN | No criterion-specific proof is recorded. |
| 36 | `MENU`, `MENU TREE`, `MENU <id>`, `MENU FIND`, `DESCRIBE`, and `DO` operate against the shared registry; | OPEN | No criterion-specific proof is recorded. |
| 37 | state-disabled registry commands are represented consistently in GUI and Telnet discovery; | OPEN | No criterion-specific proof is recorded. |
| 38 | host-read, host-write, destructive-media, application-control, and local-only commands are denied by initial unauthenticated Telnet policy; | OPEN | No criterion-specific proof is recorded. |
| 39 | the Telnet application parser is bounded to 1024-byte decoded command lines and deterministically recovers after overflow; | OPEN | No criterion-specific proof is recorded. |
| 40 | Telnet generic arguments are never interpreted by an OS shell; | OPEN | No criterion-specific proof is recorded. |
| 41 | required local workflows are keyboard-operable without a mouse; | OPEN | No criterion-specific proof is recorded. |
| 42 | focus changes do not leave stuck local Spectrum matrix keys; | OPEN | No criterion-specific proof is recorded. |
| 43 | local GUI focus does not disable independently owned Telnet keys; | OPEN | No criterion-specific proof is recorded. |
| 44 | fullscreen, manager windows, dialogs, and status presentation do not alter canonical machine state; | OPEN | No criterion-specific proof is recorded. |
| 45 | cancellation and non-destructive error paths leave machine/media state unchanged where the operation has not committed; | OPEN | No criterion-specific proof is recorded. |
| 46 | destructive local media actions require explicit confirmation; | OPEN | No criterion-specific proof is recorded. |
| 47 | platform-specific menu relocation does not change semantic command IDs; | OPEN | No criterion-specific proof is recorded. |
| 48 | the complete legacy-item disposition in Section 20 is represented in the backlog with no unclassified legacy menu command; | OPEN | No criterion-specific proof is recorded. |
| 49 | Phase-12 UI toolkit selection documents static-link/single-binary fit, keyboard operation, and accessibility support; | OPEN | No criterion-specific proof is recorded. |
| 50 | all Section 47 required regression tests applicable to the implemented milestone pass; | OPEN | No criterion-specific proof is recorded. |
| 51 | successful Telnet keyboard commands return the Section 27.1 response and invalid/held-key cases return the frozen error responses; | OPEN | No criterion-specific proof is recorded. |
| 52 | reset/model changes invoked while paused leave the application paused; | OPEN | No criterion-specific proof is recorded. |
| 53 | `MENU TREE` uses the Section 35 record format and does not disclose dynamic Recent-file absolute paths; | OPEN | No criterion-specific proof is recorded. |
| 54 | `STATUS`/`DESCRIBE` do not expose arbitrary absolute host media paths under the initial unauthenticated policy; | OPEN | No criterion-specific proof is recorded. |
| 55 | `Tools > Snapshot Inspector...` exists and reuses shared state-inspection machinery rather than a separate snapshot-state implementation; | OPEN | No criterion-specific proof is recorded. |
| 56 | every required Section 7.1 menu/action and non-menu semantic command ID is registered with the frozen metadata contract; | OPEN | No criterion-specific proof is recorded. |
| 57 | the status line always exposes `Control Port: <number>` for an available listener or `Control Port: unavailable` after full range exhaustion; | OPEN | No criterion-specific proof is recorded. |
| 58 | each WZSN process uses the first bindable candidate in 30740-32787 and simultaneous processes cannot acquire the same numeric Control Port; | OPEN | No criterion-specific proof is recorded. |
| 59 | the selected Control Port is session state and is never persisted; | OPEN | No criterion-specific proof is recorded. |
| 60 | the Networking settings surface is one radio group with exactly `None`, `Interface-1`, and `Ear+Mic`, backed by `machine.networking.set`; | OPEN | No criterion-specific proof is recorded. |
| 61 | `Interface-1` and `Ear+Mic` cannot be active simultaneously in UI state, registry state, or core state; | OPEN | No criterion-specific proof is recorded. |
| 62 | `Ear+Mic` performs no networking-ROM paging and does not automatically install the Architecture-#3 resident RAM stack; | OPEN | No criterion-specific proof is recorded. |
| 63 | networking-mode changes use the shared cold machine-reconfiguration path, do not preserve old Spectrum RAM/hooks/device state, and preserve only the application paused/running state; | OPEN | No criterion-specific proof is recorded. |
| 64 | multi-instance settings writes, writable-media claims, and Telnet screenshot creation satisfy the interprocess safety contract without corrupting shared host data; | OPEN | No criterion-specific proof is recorded. |
| 65 | `machine.networking.set` has an explicit remote-permission class and, when invoked through an allowed front end, reaches the same cold-reconfiguration handler as the Networking radio group; dirty Interface-1 media makes the remote command unavailable until resolved locally; | OPEN | No criterion-specific proof is recorded. |
| 66 | in `Ear+Mic` mode the ordinary Tape transport cannot drive/consume the same cassette signals concurrently with the routed virtual network, and the Architecture-3 bootstrap action uses the explicit `BOOTSTRAP_TAPE` waveform path rather than RAM injection; | OPEN | No criterion-specific proof is recorded. |
| 67 | `Ear+Mic` is unavailable on 128K or any 48K profile/variant not certified for Architecture #3's Issue-2 target, with a stable disabled reason and no machine-state mutation on a rejected request. | OPEN | No criterion-specific proof is recorded. |
