<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->



# IMPORTANT — FINAL CERTIFICATION ONLY

> **RUN THESE CORRECTNESS/CERTIFICATION TESTS ONLY AFTER PROJECT DEVELOPMENT IS COMPLETE AND BEFORE FINAL DELIVERY.**
>
> **This specification is the final pre-delivery correctness/certification gate.**

# Warajevo ZX Spectrum Next - Forensic Emulator Correctness Proof and Certification Specification

## Document purpose and authority

This document is the release-grade correctness proof specification for WZSN. It was constructed and audited against the **saved disk copies** of the three project architectures in the mandatory order:

1. Core/System Architecture.
2. UI Architecture.
3. ZX48 MIC/EAR Router Network Architecture.

It does not redefine those architectures. Where this document and an architecture disagree, the architecture wins and this document is defective until repaired.

### Source fingerprints used for this revision

| Order | Exact audited disk-copy filename | Canonical repository filename | SHA-256 | Lines |
|---:|---|---|---|---:|
| 1 | `warajevo-zx-spectrum-next-architecture(1).md` | `design/warajevo-zx-spectrum-next-architecture.md` | `129ba88114b9f81c78adacc1a1b38be766c3ad4ab4c03554e738d7fc8f37de80` | 4612 |
| 2 | `warajevo-zx-spectrum-next-ui-architecture(1).md` | `design/warajevo-zx-spectrum-next-ui-architecture.md` | `9dc6a14eedd65a905f4f8b25859b5b553a12227f4d7c4aa6103d4098214b2191` | 3141 |
| 3 | `zx48-mic-ear-router-network-architecture(1).md` | `design/zx48-mic-ear-router-network-architecture.md` | `c3b44689bb842da4e79aaedaf2c99ec0ea6e806bccede2cdabeba018e765f034` | 4138 |

Certification against different architecture bytes is invalid until a fresh forensic reconciliation is performed.

## Certification vocabulary

- **PASS** - exact stated behavior proven with required artifacts and retained evidence.
- **FAIL** - expected behavior not obtained, unexplained divergence exists, required artifact is corrupt/wrong, or a stronger oracle contradicts WZSN.
- **BLOCKED_GATE** - architecture deliberately leaves a value/choice open until a named gate. This is never PASS and never permission to invent a value.
- **N/A** - only when the architecture explicitly allows the feature to be outside the release claim.
- **CANARY** - integration stress software; useful detector, never a higher authority than exact machine/hardware evidence.

## Evidence precedence

1. Identified real-hardware measurements / silicon-derived evidence.
2. Architecture-frozen hardware/format specifications and primary technical references.
3. Hardware-derived specialist tests.
4. Independent exercisers and regression suites.
5. Differential emulators / historical Warajevo behavior.
6. Commercial software, demos, and visual/audio canaries.

A lower tier may not silently overrule a higher tier. Any oracle conflict freezes the timing trace, reduces the failure, records the exact external revision/artifact, resolves it against stronger evidence, adds a project regression, and reruns every dependent gate.

## Artifact rules

Every test below names its required artifacts. The `Required artifacts` list attached to a test block is normative for every test in that block, and each individual result MUST enumerate the exact subset it actually consumed in its `required_artifacts` field. Basenames are never sufficient when two paths could contain the same name: the exact repository-relative path shown by this specification or by `certification/manifests/per-test-artifacts.json` is authoritative. Project-owned fixtures use the exact path shown and are checked into the development/test tree or generated deterministically into that path. External artifacts are acquired from the source catalog below and their acquired SHA-256 is frozen in the release evidence manifest. ROMs/firmware that WZSN cannot redistribute are supplied legally by the developer/user and identified by frozen hash; this document does not provide unauthorized ROM distribution.

`certification/manifests/per-test-artifacts.json` is mandatory release evidence. It contains one record for every executable test, ledger subtest, acceptance item, gate item, and appendix proof (`C*`, `U*`, `M*`). Each record contains the exact test ID plus every required artifact's exact local path, SHA-256 or explicit pre-acquisition/gate status, provenance class (`project-generated`, `legally-supplied`, `external`, or `private-development-only`), and source authority. For an external artifact it also records the catalog ID (`E01`-`E12` or `R01`-`R05`), exact acquisition filename, best acquisition/reference source from the catalog, acquired SHA-256, and—when an archive is involved—the exact runnable member path and member SHA-256 after any required identity gate. A test is `BLOCKED_GATE`, never PASS, if a required external archive member has not yet been forensically identified. This manifest is an index of requirements, not permission to substitute a different file.

The deliberately unrestricted private difficult-media directory remains an exception to fixed predeclared filenames: `WZSN-PRIVATE-TEST-MEDIA/` may contain arbitrary developer media. The architecture and this certification document impose no naming, subdirectory, manifest, hash, provenance, source-URL, media-type, or one-test-per-file requirement on those private files. A private regression result may log the filenames it actually exercised for developer convenience, but no complete directory inventory or per-file hash is required. Unrecognized files may be ignored exactly as Architecture #1 permits.

### `WZSN-PRIVATE-TEST-MEDIA/` gathering checklist

This is a **developer collection checklist, not a manifest or a new architecture requirement**. Keep the private media files directly in the flat `WZSN-PRIVATE-TEST-MEDIA/` directory. The private media contents must remain Git-ignored/untracked and must never be committed or pushed to the public GitHub repository, copied into public source/binary distributions, or required by public CI. The directory remains deliberately unmanaged: this checklist does not impose filenames, hashes, provenance records, source URLs, media-type grouping, subdirectories, or one-test-per-file coverage. Except for the two Architecture-#3 developer-selected cases explicitly identified below, the project may choose whichever private files are useful for the difficult-media regression.

Gather, from legally held/developer-available media, a useful representative set spanning both initially certified machine profiles—ZX Spectrum 48K PAL and ZX Spectrum 128K PAL—where the media/software is applicable:

- **One ZX Spectrum 48K Issue-2-compatible game/program for M28.52**: a real program that can be left running while the router queues a message. The purpose is to prove that queued delivery does not depend on the Spectrum already being in a receive operation.
- **One compatible ZX Spectrum 48K Issue-2 game/demo for M28.53**: software that leaves the Architecture-#3 resident high-RAM region and required interrupt hook intact so the slow EAR attention pattern can still be detected while the software runs. The same file may satisfy M28.52 as well if it genuinely exercises both cases.
- **Historically troublesome Warajevo media/programs** from the developer's collection, especially cases that previously needed compatibility offsets, timing workarounds, loader workarounds, or other emulator-specific handling.
- **Timing- and raster-sensitive programs/demos** that stress within-scanline and between-scanline border changes, overscan, rainbow/multicolor effects, bitmap/attribute writes close to ULA fetches, HALT/interrupt synchronization, contention, floating-bus-dependent behavior, and other hardware-timing tricks. Private examples supplement the controlled E09-E12 NIRVANA/NIRVANA+/BIFROST-family artifacts; they do not replace those cataloged files.
- **Difficult real-world tape media**: standard TAP, Warajevo-native TAP where available, complex TZX, and WAV/audio captures; include custom/turbo/non-ROM loaders, unusual but valid block layouts, protection/loader tricks, and borderline real-world cases that are useful for compatibility testing.
- **Difficult real-world snapshots**: 48K and 128K SNA files and Z80 snapshots across the supported v1/v2/v3 families, including examples with paging, AY state, Interface-1-related state where applicable, unusual but valid extensions, and snapshots known to expose restore-order or machine-identification bugs.
- **Microdrive / Interface 1 / original ZX Net media and programs**: MDR images and software that exercise cartridge catalog/read/write behavior, write protection, sector/allocation oddities, Interface-1 paging/state, and original ZX Net behavior where suitable private examples are available.
- **Audio-sensitive programs/demos** that expose beeper level/timing behavior or 128K AY timing/mixer/envelope behavior strongly enough to reveal machine-time errors.
- **ZX Printer and input/peripheral exercisers** from real software where useful, including programs known to depend on ZX Printer behavior, keyboard-matrix handling, Kempston reads, or peripheral timing rather than merely displaying a menu option.
- **Real-world specimens for retained Compatibility Tools**, but only for a conversion/tool that is actually implemented for the certification baseline being run. This can include legacy tape, snapshot, Spectrum-data, Microdrive, and legacy-database source formats represented by the UI architecture's retained conversion inventory.
- **Any additional developer-owned difficult TAP, TZX, SNA, Z80, MDR, program, or other artifact that has previously exposed an unexplained emulator mismatch or hardware-hack compatibility problem.** This catch-all is intentional and follows Architecture #1 Section 36.2.

Do **not** move controlled certification artifacts into this private folder merely for convenience. Cataloged external artifacts E01-E12 remain at their exact `external/...` paths; project-owned fixtures remain at their exact `tests/fixtures/...` paths; legally supplied certification ROM/firmware remains at the exact paths required by the relevant tests. The private folder supplements those artifacts and never substitutes for them.

## External artifact and reference catalog

| ID | Artifact | Exact acquisition filename / local path | Best acquisition/reference source | Certification role |
|---|---|---|---|---|
| E01 | Fuse source release | `fuse-1.9.2.tar.gz` -> `external/fuse-1.9.2.tar.gz`; extract those pinned archive bytes under `external/` while preserving the archive's top-level path, yielding required `external/fuse-1.9.2/z80/tests/tests.in` and `external/fuse-1.9.2/z80/tests/tests.expected` | https://sourceforge.net/projects/fuse-emulator/files/fuse/1.9.2/ | Pin archive bytes/hash in the certification manifest and record the extracted-tree hash/listing. Run the complete applicable upstream Z80 suite using those exact test vectors/expected results plus the upstream harness semantics. |
| E02 | Patrik Rak Zilog Z80 CPU Test Suite v1.2a | runnable archive `z80test-1.2a.zip` -> `external/z80test-1.2a.zip`; extract the pinned archive under `external/` while preserving its top-level `z80test-1.2a/` path, yielding exact runnable paths `external/z80test-1.2a/z80full.tap`, `external/z80test-1.2a/z80doc.tap`, `external/z80test-1.2a/z80flags.tap`, `external/z80test-1.2a/z80docflags.tap`, `external/z80test-1.2a/z80ccf.tap`, and `external/z80test-1.2a/z80memptr.tap` | runnable archive: `http://zxds.raxoft.cz/taps/misc/z80test-1.2a.zip`; upstream revision: https://github.com/raxoft/z80test/releases/tag/v1.2a (`c490c0c`); independent archive/member cross-check: https://github.com/redcode/Z80 | Pin archive SHA-256, extracted member SHA-256 values, and upstream source revision in the certification manifest. Upstream states expected values are derived from a real 48K Spectrum with a Zilog Z80. The long depot filename sometimes used by test depots is not treated as a GitHub-release asset. |
| E03 | Cringle/Harston ZEX Spectrum ports | `Z80 Documented Instruction Set Exerciser for Spectrum (2018)(Harston, Jonathan Graham)[!].tap` -> `external/zex/zexdoc.tap`; `Z80 Full Instruction Set Exerciser for Spectrum (2018)(Harston, Jonathan Graham)[!].tap` -> `external/zex/zexall.tap` | https://github.com/redcode/Z80 | Use the redcode/Z80 test depot/fetch tooling or its referenced Harston Spectrum conversions. Preserve each original acquisition filename in the certification manifest and record exact acquired bytes/hash; the shorter local aliases are harness paths only. |
| E04 | Mark Woodmass Z80 Test Suite | `Z80 Test Suite (2008)(Woodmass, Mark)[!].tap` -> `external/woodmass/z80tests.tap` | https://github.com/redcode/Z80 | Use redcode/Z80's test depot/fetch tooling. Local canonical name may be `z80tests.tap`; preserve original acquisition filename in manifest. |
| E05 | Mark Woodmass HALT2INT v3 - supplemental until archive member is explicitly pinned | `HALT2INT v3 (2022-01-04)(Woodmass, Mark) [!].zip` -> `external/woodmass/HALT2INT-v3.zip` | https://zxe.io/depot/software/ZX%20Spectrum/HALT2INT%20v3%20%282022-01-04%29%28Woodmass%2C%20Mark%29%20%5B%21%5D.zip | The archive filename is exact. It is NOT a required certification artifact until the project records the exact runnable archive-member filename and both archive/member hashes in the certification manifest; no test may invent a member name. |
| E06 | Ramsoft floating-bus diagnostic | authoritative archive `floatspy.zip` -> `external/ramsoft/floatspy.zip`; runnable TAP member -> `external/ramsoft/floatspy.tap` only after member identity/hash is frozen in `certification/gates/external/E06-floatspy-identity.json` | https://k1.spdns.de/Develop/Projects/zxsp/Info/Floating%20bus%20technical%20guide.html (download target: `https://www.ramsoft.bbk.org/tech/floatspy.zip`) | Hardware-measured floating-bus self-test. The authoritative guide proves the distribution archive target but does not expose the internal ZIP member name. Until `E06-floatspy-identity.json` records the exact archive SHA-256, runnable member path/name, and member SHA-256, tests requiring E06 return BLOCKED_GATE rather than guessing a member name. |
| E07 | SoftSpectrum 48K timing tests | `timing_tests-48k_v1.0.tzx` -> `external/softspectrum/timing_tests-48k_v1.0.tzx` | https://softspectrum48.weebly.com/uploads/6/6/7/5/66753101/timing_tests-48k_v1.0.tzx | Supplemental machine-level timing diagnostic; do not let it override stronger hardware evidence. |
| E08 | SoftSpectrum 128K timing tests | `timing_tests-128k_v1.0.z80` -> `external/softspectrum/timing_tests-128k_v1.0.z80` | https://softspectrum48.weebly.com/uploads/6/6/7/5/66753101/timing_tests-128k_v1.0.z80 | Supplemental 128K timing diagnostic; exact bytes/hash recorded. |
| E09 | NIRVANA ENGINE | authoritative archive `NIRVANAENGINE.tap.zip` -> `external/canary/NIRVANAENGINE.tap.zip`; runnable member -> `external/canary/NIRVANAENGINE.tap` only after member identity/hash is frozen in `certification/gates/external/E09-nirvana-member.json` | https://worldofspectrum.net/item/0030001/ | Tier-6 multicolour/raster canary. The authoritative page proves the archive filename, not the internal ZIP member name. Until the gate proves the exact runnable member/hash, tests requiring E09 return BLOCKED_GATE. |
| E10 | NIRVANA+ ENGINE | authoritative archive `NIRVANA+ENGINE.tap.zip` -> `external/canary/NIRVANA+ENGINE.tap.zip`; runnable member -> `external/canary/NIRVANA+ENGINE.tap` only after member identity/hash is frozen in `certification/gates/external/E10-nirvanaplus-member.json` | https://worldofspectrum.net/item/0030002/ | Tier-6 multicolour/raster canary. The authoritative page proves the archive filename, not the internal ZIP member name. Until the gate proves the exact runnable member/hash, tests requiring E10 return BLOCKED_GATE. |
| E11 | BIFROST* ENGINE v1.2 low/high | authoritative archives `BIFROSTENGINEV1.2L.tzx.zip` and `BIFROSTENGINEV1.2H.tzx.zip` -> `external/canary/`; runnable members -> `external/canary/BIFROSTENGINEV1.2L.tzx` and `external/canary/BIFROSTENGINEV1.2H.tzx` only after both member identities/hashes are frozen in `certification/gates/external/E11-bifrost-members.json` | https://worldofspectrum.net/item/0027405/ | Tier-6 multicolour/raster canaries. The authoritative page proves the two archive filenames, not their internal ZIP member names. Until the gate proves both runnable members/hashes, tests requiring E11 return BLOCKED_GATE. |
| E12 | BIFROST*2 ENGINE | authoritative archive `BIFROST2ENGINE.tap.zip` -> `external/canary/BIFROST2ENGINE.tap.zip`; runnable member -> `external/canary/BIFROST2ENGINE.tap` only after member identity/hash is frozen in `certification/gates/external/E12-bifrost2-member.json` | https://worldofspectrum.net/item/0030003/ | Tier-6 multicolour/raster canary. The authoritative page proves the archive filename, not the internal ZIP member name. Until the gate proves the exact runnable member/hash, tests requiring E12 return BLOCKED_GATE. |
| R01 | TZX Technical Specification v1.20 | `TZXformat.html` -> `reference/TZXformat-v1.20.html` | https://worldofspectrum.net/TZXformat.html | Format authority for TZX block semantics/timing. |
| R02 | World of Spectrum snapshot-format documentation | `formats.htm` -> `reference/formats.html`; `snaformat.html` -> `reference/snaformat.html`; `z80format.html` -> `reference/z80format.html` | index: https://worldofspectrum.net/faq/reference/formats.htm ; SNA: https://worldofspectrum.net/zx-modules/fileformats/snaformat.html ; Z80: https://worldofspectrum.net/zx-modules/fileformats/z80format.html | Pin the exact downloaded bytes and SHA-256 of all three pages in the certification reference manifest. `snaformat.html` and `z80format.html` are the exact Phase-8 format authorities; the general index is navigational/supporting evidence only. |
| R03 | 48K technical reference | `48kreference.htm` -> `reference/48kreference.html` | https://worldofspectrum.org/faq/reference/48kreference.htm | Hardware/timing reference; real-hardware evidence remains higher authority. |
| R04 | 128K technical reference | `128kreference.htm` -> `reference/128kreference.html` | https://worldofspectrum.org/faq/reference/128kreference.htm | Hardware/timing reference for 128K PAL. |
| R05 | Official Sokol source repository | pinned commit checkout -> `third_party/sokol/` | https://github.com/floooh/sokol | Vendor the exact commit recorded in `certification/manifests/build.json`; release builds must not substitute an ambient/system Sokol copy. |

External artifacts are not automatically redistributed with WZSN. Acquisition rights/licensing and hashes are recorded separately in the development certification manifest.

# Part I - Architecture #1: Core/System correctness tests

## C01. Architecture section 1: Architectural mission

**Architecture authority:** `C` source lines 19-44, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Accuracy-first architectural mission.

**Required artifacts:**

- `certification/config/core-release-profile.json`

**Artifact/source authority:** Project-generated certification configuration; no external media.

**Tests:**

### C01.01

Execute the architecture-complete certification program and prove every mission property explicitly: preserve Warajevo 2.50 knowledge/provenance, replace host-era mechanisms, prefer real hardware over emulator hacks, portable deterministic core, master-tick timing fine enough for ULA/bus order, authentic 48K+128K profiles, modern host integration without contaminating the core, one application binary per supported platform, and regression evidence for every correctness-sensitive subsystem.

**Evidence output:** `certification/results/core/C01.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C01.02

Run optimized/unoptimized and headless/GUI builds from identical initial state/input traces; any behavioral difference or application-specific compatibility shortcut is a mission failure.

**Evidence output:** `certification/results/core/C01.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## C02. Architecture section 2: Sources of truth

**Architecture authority:** `C` source lines 45-212, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Source authority, legacy disposition, and evidence precedence.

**Required artifacts:**

- `certification/manifests/core-legacy-feature-disposition-v1.json`
- `wzsn-architectures-1-2-developer-tasks.md`
- `design/migration-ledger.md`
- `certification/manifests/warajevo-source-authority.json`
- `reference/warajevo/Download.html`
- `reference/warajevo/Revision.html`
- preserved legacy checkout recorded by commit/tree hash in `certification/manifests/warajevo-source-authority.json`
- `external/fuse-1.9.2.tar.gz`
- `external/z80test-1.2a/z80full.tap`
- `external/zex/zexdoc.tap`

**Artifact/source authority:** Preserved Warajevo 2.50 repository is the migration-source authority. `reference/warajevo/Download.html` and `reference/warajevo/Revision.html` are frozen copies of the architecture-cited official historical pages `https://worldofspectrum.net/warajevo/Download.html` and `https://worldofspectrum.net/warajevo/Revision.html`. External CPU authorities are catalog E01-E04; higher-tier hardware evidence governs hardware disagreements.

**Tests:**

### C02.01

Audit every REQUIRED/LATER/REPLACE/DROP legacy capability against the backlog and test manifest; no REQUIRED item may be unclassified.

**Evidence output:** `certification/results/core/C02.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C02.02

Deliberately inject one disagreement between a Warajevo-2.50 behavior and a hardware-grounded expected result and prove the hardware-grounded oracle wins with the difference recorded.

**Evidence output:** `certification/results/core/C02.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C02.03

Verify the complete pinned Fuse Z80 suite is present in the certification manifest.

**Evidence output:** `certification/results/core/C02.03.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C02.04

Forensically verify the surviving-source authority boundary: the recorded legacy checkout is the preserved Warajevo 2.50 source; the frozen official `Download.html`/`Revision.html` evidence supports treating observable 2.51/2.52 binaries, documentation, revision notes and sample files only as secondary behavioral evidence; no ticket may cite an unverified imaginary 2.51/2.52 source routine. Inject a mock "newly discovered 2.52 source" and prove it is rejected until repository/blob provenance and cryptographic hash are independently verified and admitted to the source-authority manifest.

**Evidence output:** `certification/results/core/C02.04.json`

**Pass rule:** PASS only when migration-source versus secondary-behavioral authority is enforced exactly and an unverified later-source claim cannot become implementation authority.

### C02 literal legacy-feature disposition ledger

Every Section-2.5 disposition row is a separate scope test. The project-owned `core-legacy-feature-disposition-v1.json` mirrors the literal row/status; REQUIRED/REPLACE rows additionally point to the concrete implementation proof, while LATER/NOT-initial rows point to the backlog/scope artifact proving they are not silently promoted into initial blockers or falsely advertised as complete.

| Subtest | Literal architecture row | Primary proof test(s) | Required proof artifact(s) | Evidence artifact |
|---|---|---|---|---|
| C02.D01 | ZX Spectrum 48K PAL                        REQUIRED - initial certified machine | C09.01 | `tests/fixtures/machines/wzsn-profile-scope-v1.json`; `tests/fixtures/timing/zx48-pal-timing-v1.json` | `certification/results/core/C02.D01.json` |
| C02.D02 | ZX Spectrum 128K PAL                       REQUIRED - initial certified machine | C09.01 | `tests/fixtures/machines/wzsn-profile-scope-v1.json`; `tests/fixtures/timing/zx128-pal-timing-v1.json` | `certification/results/core/C02.D02.json` |
| C02.D03 | ZX Spectrum +2                             LATER compatibility target; not an initial blocker | C09.03 | `tests/fixtures/machines/wzsn-profile-scope-v1.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/core/C02.D03.json` |
| C02.D04 | Timex Sinclair 2068                        LATER compatibility target; preserved source retained | C09.03 | `tests/fixtures/machines/wzsn-profile-scope-v1.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/core/C02.D04.json` |
| C02.D05 | DCK/Timex memory expansions                LATER with Timex support | C09.03; U17.01 | `tests/fixtures/machines/wzsn-profile-scope-v1.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/core/C02.D05.json` |
| C02.D06 | Z80 CPU incl. documented/undocumented      REQUIRED | C11.01; C11.03 | `tests/fixtures/cpu/wzsn-z80-semantic-v1.json`; `external/fuse-1.9.2/z80/tests/tests.in`; `external/fuse-1.9.2/z80/tests/tests.expected` | `certification/results/core/C02.D06.json` |
| C02.D07 | ULA/border/contention/floating bus          REQUIRED | C13.01; C15.01; C15.03 | `tests/fixtures/timing/wzsn-contention-48k.tap`; `tests/fixtures/ula/wzsn-ula-fetch-48k.tap`; `external/ramsoft/floatspy.tap`; `certification/gates/external/E06-floatspy-identity.json` | `certification/results/core/C02.D07.json` |
| C02.D08 | 48K keyboard matrix                        REQUIRED | C22.01 | `tests/fixtures/input/wzsn-keyboard-matrix-v1.json`; `design/input-hardware-evidence.md` | `certification/results/core/C02.D08.json` |
| C02.D09 | Kempston joystick                          REQUIRED initial joystick interface | C22.03 | `tests/fixtures/input/wzsn-kempston-port-v1.json`; `design/input-hardware-evidence.md` | `certification/results/core/C02.D09.json` |
| C02.D10 | Beeper                                     REQUIRED | C20.01 | `tests/fixtures/audio/wzsn-beeper-events-v1.json` | `certification/results/core/C02.D10.json` |
| C02.D11 | AY-3-8912 on 128K                          REQUIRED | C20.02 | `tests/fixtures/audio/wzsn-ay-register-events-v1.json`; `tests/fixtures/timing/zx128-pal-timing-v1.json` | `certification/results/core/C02.D11.json` |
| C02.D12 | Tape: standard TAP                         REQUIRED | C23.01 | `tests/fixtures/tape/wzsn-tap-standard-oneblock.tap`; `design/media-format-support.md` | `certification/results/core/C02.D12.json` |
| C02.D13 | Tape: Warajevo native TAP                  REQUIRED compatibility path | C23.01 | `tests/fixtures/tape/wzsn-warajevo-native-minimal.tap`; `tests/fixtures/tape/wzsn-warajevo-native-edge.tap`; `design/media-format-support.md` | `certification/results/core/C02.D13.json` |
| C02.D14 | Tape: TZX                                  REQUIRED | C23.02 | `tests/fixtures/tzx/wzsn-tzx-standard-speed.tzx`; `tests/fixtures/tzx/wzsn-tzx-turbo.tzx`; `design/media-format-support.md` | `certification/results/core/C02.D14.json` |
| C02.D15 | Tape: WAV/audio input                      REQUIRED deterministic decode path | C23.02 | `tests/fixtures/tape/wzsn-tape-44100-mono.wav`; `tests/fixtures/tape/wzsn-tape-48000-noisy.wav`; `design/media-format-support.md` | `certification/results/core/C02.D15.json` |
| C02.D16 | Live physical cassette capture              LATER; not an initial blocker | C23.08 | `certification/manifests/core-legacy-feature-disposition-v1.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/core/C02.D16.json` |
| C02.D17 | Snapshots: SNA 48K/128K                    REQUIRED | C25.01; C25.03 | `tests/fixtures/snapshot/wzsn-sna-48k-canonical.sna`; `tests/fixtures/snapshot/wzsn-sna-128k-canonical.sna` | `certification/results/core/C02.D17.json` |
| C02.D18 | Snapshots: Z80                             REQUIRED | C25.01; C25.03 | `tests/fixtures/snapshot/wzsn-z80-v1-uncompressed.z80`; `tests/fixtures/snapshot/wzsn-z80-v2-48k.z80`; `tests/fixtures/snapshot/wzsn-z80-v3-128k.z80` | `certification/results/core/C02.D18.json` |
| C02.D19 | Interface 1                                REQUIRED | C24.03 | `design/interface1-microdrive-zxnet.md`; `tests/fixtures/interface1/wzsn-if1-rom-page.tap` | `certification/results/core/C02.D19.json` |
| C02.D20 | Microdrive / MDR                           REQUIRED | C24.03 | `tests/fixtures/mdr/wzsn-mdr-minimal.mdr`; `tests/fixtures/mdr/wzsn-mdr-write-protect.mdr` | `certification/results/core/C02.D20.json` |
| C02.D21 | Original Sinclair/ZX Net behavior          REQUIRED | C24.03 | `tests/fixtures/interface1/wzsn-if1-zxnet-v1.tap`; `design/interface1-microdrive-zxnet.md` | `certification/results/core/C02.D21.json` |
| C02.D22 | ZX Printer                                 REQUIRED legacy peripheral | U16.01 | `tests/fixtures/printer/wzsn-zxprinter-smoke.tap`; `tests/fixtures/ui/U16-printer-manager.json` | `certification/results/core/C02.D22.json` |
| C02.D23 | Built-in monitor/debugger                  REQUIRED legacy workflow | U21.01; U21.03 | `tests/fixtures/debugger/wzsn-debugger-state-v1.json`; `tests/fixtures/ui/U21-debugger.json` | `certification/results/core/C02.D23.json` |
| C02.D24 | Runtime speed control                      REQUIRED, redesigned as host pacing | C10.05; C10.06 | `tests/fixtures/traces/wzsn-speed-transition-v1.json`; `tests/fixtures/pacing/wzsn-pacing-v1.json` | `certification/results/core/C02.D24.json` |
| C02.D25 | 128K MIDI interface                        LATER; not an initial blocker | C02 disposition-only | `certification/manifests/core-legacy-feature-disposition-v1.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/core/C02.D25.json` |
| C02.D26 | 128K extended keypad                       LATER; not an initial blocker | C02 disposition-only | `certification/manifests/core-legacy-feature-disposition-v1.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/core/C02.D26.json` |
| C02.D27 | Historical RS-232 host redirection         LATER host-integration work; authentic IF1 state first | C24.05; U20.02 | `tests/fixtures/interface1/wzsn-if1-serial-state-v1.json`; `tests/fixtures/ui/U20-legacy-prose-rules.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/core/C02.D27.json` |
| C02.D28 | Historical external plug-in ABI            NOT an initial compatibility requirement | C02 disposition-only | `certification/manifests/core-legacy-feature-disposition-v1.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/core/C02.D28.json` |
| C02.D29 | ZXCOMP executable-snapshot compiler        LATER/separate tool; not part of initial emulator binary | C02 disposition-only | `certification/manifests/core-legacy-feature-disposition-v1.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/core/C02.D29.json` |
| C02.D30 | Historical database/shell/help system      REPLACE with modern UI where functionality is retained | U20.01; U20.02 | `tests/fixtures/ui/U20-legacy-disposition.json`; `tests/fixtures/ui/U20-legacy-prose-rules.json` | `certification/results/core/C02.D30.json` |
| C02.D31 | Historical file-conversion utilities       LATER utility work unless required by a media test | U20.01; U22.01 | `tests/fixtures/ui/U20-legacy-disposition.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/core/C02.D31.json` |
| C02.D32 | DOS/BIOS/video/sound host mechanisms       REPLACE; never compatibility targets themselves | C42.01; C42.02 | `certification/manifests/legacy-host-replacement.json`; `tests/fixtures/performance/wzsn-optimization-policy-v1.json` | `certification/results/core/C02.D32.json` |

## C03. Architecture section 3: Language and implementation contract

**Architecture authority:** `C` source lines 213-252, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** ISO C11 and implementation-language contract.

**Required artifacts:**

- `CMakeLists.txt`
- `cmake/toolchains/`
- `certification/config/compiler-matrix.json`

**Artifact/source authority:** Project source tree and compiler matrix.

**Tests:**

### C03.01

Compile core/application C sources as ISO C11 with mandatory warning levels on every required compiler; later language-standard selection is rejected unless architecture/build revision is explicit and canonical results remain unchanged.

**Evidence output:** `certification/results/core/C03.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C03.02

Static-scan deterministic core translation units for Objective-C/C++/Win32/X11/Metal/D3D/OpenGL/ALSA/CoreAudio/WASAPI types or headers: none may appear. Platform host translation units may use required platform language mode (for example macOS Objective-C) only behind a C interface.

**Evidence output:** `certification/results/core/C03.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C03.03

Trace preserved Pascal/x86-assembly behavior into C provenance records; no test assumes source-language or host-trick preservation.

**Evidence output:** `certification/results/core/C03.03.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## C04. Architecture section 4: Proposed source-tree architecture

**Architecture authority:** `C` source lines 253-388, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Source-tree/module-boundary contract.

**Required artifacts:**

- `certification/manifests/source-tree-expected.json`

**Artifact/source authority:** Project-generated manifest derived from Architecture #1 section 4.

**Tests:**

### C04.01

Compare the repository tree to the architectural subsystem boundaries and required logical roles/targets; report missing, merged-across-forbidden-boundary, or circular layers. Treat the filenames shown in Architecture Section 4 as provisional: a harmless rename that preserves the required subsystem role and dependency direction must not fail certification merely for differing from an example filename.

**Evidence output:** `certification/results/core/C04.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C04.02

Static-scan include/link dependencies for the forbidden dependency edges named by Architecture #1.

**Evidence output:** `certification/results/core/C04.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## C05. Architecture section 5: Dependency direction

**Architecture authority:** `C` source lines 389-466, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Dependency direction.

**Required artifacts:**

- `certification/manifests/allowed-dependencies.json`
- `tests/fixtures/boundaries/wzsn-core-orchestrator-contract-v1.json`

**Artifact/source authority:** Project-generated dependency manifest and normalized-input/canonical-output boundary fixture.

**Tests:**

### C05.01

Compile and link `wz_core` with no Sokol, GUI toolkit, socket, host clock, filesystem UI, or host key-code dependency.

**Evidence output:** `certification/results/core/C05.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C05.02

Inject a forbidden include/link edge in a mutation build and prove the architecture-dependency gate detects it.

**Evidence output:** `certification/results/core/C05.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C05.03

Using `wzsn-core-orchestrator-contract-v1.json`, prove the positive boundary as well as forbidden edges: the core accepts only normalized deterministic machine inputs carrying explicit emulated boundaries/master ticks (keyboard matrix, joystick, media-state changes); it emits only machine-domain canonical raster/audio/state/timing-trace results; and the application/orchestrator, not the core, owns local/Telnet event conversion, source ownership/merging, master-tick scheduling, Sokol submission, host pacing, file dialogs/UI, and listener lifecycle. Mutation cases that pass host key codes, socket objects, wall-clock timestamps, or UI/file-dialog objects directly into the core must fail the boundary gate.

**Evidence output:** `certification/results/core/C05.03.json`

**Pass rule:** PASS only when both the positive interface shape and every named orchestrator responsibility/forbidden core dependency in Section 5 are enforced.

## C06. Architecture section 6: Host-independence and determinism contract

**Architecture authority:** `C` source lines 467-508, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Host independence and determinism.

**Required artifacts:**

- `tests/fixtures/traces/wzsn-determinism-input-v1.json`
- `certification/config/compiler-matrix.json`
- `tests/fixtures/determinism/wzsn-host-abi-variance-v1.json`

**Artifact/source authority:** Project-generated deterministic input trace and ABI-variance build/test matrix.

**Tests:**

### C06.01

Run identical machine profile, ROM, initial state, normalized input trace, and checkpoint master ticks across every mandatory host/compiler group; compare canonical state hashes.

**Evidence output:** `certification/results/core/C06.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C06.02

Repeat with different host window sizes, GPU/audio devices, and wall-clock pacing; canonical results must not change.

**Evidence output:** `certification/results/core/C06.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C06.03

Use `wzsn-host-abi-variance-v1.json` to exercise every Section-6 host-property class that can be varied or simulated independently of the supported release targets: signed/unsigned `char`, enum-size/representation options where compiler-supported, structure packing/alignment stress, pointer-width-sensitive static-analysis/build checks, endian codec simulation, alternate floating-point optimization modes for host code, randomized host thread scheduling, distinct audio callback sizes/sample rates, monitor refresh/DPI/Sokol frame cadence, and compiler optimization/register-allocation modes. Canonical machine checkpoints must be identical. Properties not physically available on a required host (for example a big-endian release CPU) are proved by byte-order-independent codec/mutation tests rather than silently marked untested.

**Evidence output:** `certification/results/core/C06.03.json`

**Pass rule:** PASS only when every host-dependence class named in Section 6 has an explicit exercised or simulated proof path and none changes canonical state.

## C07. Architecture section 7: Defined C behavior contract

**Architecture authority:** `C` source lines 509-606, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Defined C behavior.

**Required artifacts:**

- `certification/config/sanitizers.json`
- `certification/manifests/core-state-fuzz-seeds.json`
- `certification/manifests/machine-storage-types.json`

**Artifact/source authority:** Project-generated sanitizer/fuzz inputs pinned by `core-state-fuzz-seeds.json`, which enumerates every admitted seed by exact relative filename, SHA-256, generator/provenance, and exercised boundary class; plus a source/AST-derived manifest of machine-state storage types. A directory listing alone is not certification evidence.

**Tests:**

### C07.01

Run ASan/UBSan or compiler-equivalent checks, high warnings, alignment/aliasing/shift/overflow boundary tests, and endian serialization tests.

**Evidence output:** `certification/results/core/C07.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C07.02

Round-trip all on-disk/network byte encodings without raw-struct serialization assumptions.

**Evidence output:** `certification/results/core/C07.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C07.03

Generate `machine-storage-types.json` from the compiled/parsed core declarations and verify that the core includes the ISO C11 `<stdint.h>` fixed-width type source and that representation-sensitive machine fields use the appropriate `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`, `int8_t`, `int16_t`, `int32_t`, or `int64_t` families (including the project aliases built directly from them, such as byte/address/master-tick types). Verify Z80 registers, Spectrum addresses, master ticks, serialized persistent fields, and other representation-sensitive machine state use explicit-width/explicit encoding contracts. Reject plain `int`, `long`, host pointers, raw enums, raw booleans, or host-structure layout as persistent/machine storage wherever their representation can affect machine results. This test is structural and complements runtime sanitizer/boundary tests; it must also prove little-endian Spectrum reads/writes are assembled explicitly rather than by host pointer casts or unaligned wider loads.

**Evidence output:** `certification/results/core/C07.03.json`

**Pass rule:** PASS only when every representation-sensitive field/path named by Section 7 has a host-independent representation or an explicit proof that its host representation is irrelevant.

## C08. Architecture section 8: Machine state and serialization

**Architecture authority:** `C` source lines 607-655, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Machine state and canonical serialization/hash.

**Required artifacts:**

- `tests/fixtures/state/wzsn-state-48k-v1.bin`
- `tests/fixtures/state/wzsn-state-128k-v1.bin`
- `tests/fixtures/state/wzsn-state-bad-version-v1.bin`
- `tests/fixtures/state/wzsn-state-bad-checksum-v1.bin`
- `certification/manifests/canonical-state-format-v1.json`

**Artifact/source authority:** Project-generated canonical state fixtures and an explicit field-width/byte-order/version/checksum format manifest.

**Tests:**

### C08.01

Serialize, deserialize, and canonical-hash complete 48K and 128K deterministic machine state; hashes must survive round trip and cross-host transfer.

**Evidence output:** `certification/results/core/C08.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C08.02

Prove host-only state (window, Control Port, paths, thread handles, trace filename) is absent from canonical serialization/hash.

**Evidence output:** `certification/results/core/C08.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C08.03

Verify the canonical persistent-state encoding against `canonical-state-format-v1.json`: every serialized field has an explicit width and byte order, the format carries an explicit version and checksum, and no raw `sizeof(struct)`/padding/enum/boolean representation enters the byte stream. Load the valid 48K/128K fixtures on every required host, then prove `wzsn-state-bad-version-v1.bin` and `wzsn-state-bad-checksum-v1.bin` are rejected before machine-state commit.

**Evidence output:** `certification/results/core/C08.03.json`

**Pass rule:** PASS only when portable persistent state is explicitly encoded/versioned/checksummed and malformed version/checksum input cannot partially mutate the machine.

## C09. Architecture section 9: Machine profiles

**Architecture authority:** `C` source lines 656-747, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Machine profiles and frozen timing evidence.

**Required artifacts:**

- `design/machine-timing-evidence.md`
- `tests/fixtures/timing/zx48-pal-timing-v1.json`
- `tests/fixtures/timing/zx128-pal-timing-v1.json`
- `tests/fixtures/rom/zx48-certification.rom`
- `tests/fixtures/rom/zx128-certification-0.rom`
- `tests/fixtures/rom/zx128-certification-1.rom`
- `tests/fixtures/machines/wzsn-profile-scope-v1.json`
- `certification/manifests/C09-timing-evidence-audit.json`

**Artifact/source authority:** Project-generated timing/scope evidence; ROM bytes must be legally supplied and hashes frozen before certification. `C09-timing-evidence-audit.json` is generated by a line/item audit of the frozen timing-evidence document and records the source citation or identified real-hardware measurement behind each certified timing/profile datum.

**Tests:**

### C09.01

Validate profile constants, integer/rational master-clock relations, scanline/frame totals, interrupt definitions, contention tables, and ROM hashes against the frozen Phase-0 evidence.

**Evidence output:** `certification/results/core/C09.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C09.02

Reject a profile whose ROM hash or timing-evidence revision differs from the certification manifest.

**Evidence output:** `certification/results/core/C09.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C09.03

Using `wzsn-profile-scope-v1.json`, prove that the first architecture-complete certification contains exactly required ZX Spectrum 48K PAL and ZX Spectrum 128K PAL profiles; +2/Timex and 16K/+2A/+3/clones are not silently certified. For every hardware-observable Issue/ULA/memory/model difference claimed inside a certified profile, require explicit profile data or a named documented variant—never host randomness. Run a deliberately distinct variant and prove certification of the parent profile does not automatically certify that variant or any later machine.

**Evidence output:** `certification/results/core/C09.03.json`

**Pass rule:** PASS only when profile scope, observable variant identity, and non-inheritance of certification match Section 9.1 exactly.

### C09.04

Audit `design/machine-timing-evidence.md` item by item and require `C09-timing-evidence-audit.json` to map every certified machine-profile timing/hardware datum to its evidence. At minimum this includes canonical master frequency and exact clock ratios, T-states/line, lines/frame, T-states/frame, raster clocks/line, raster origins, active/blanking regions, contention tables, interrupt timing/windows, same-edge ordering, and model/ULA-specific behavior. Each frozen value must record a source citation or identified real-hardware measurement, the measured/referenced value, uncertainty or observed variation where measurement makes that meaningful, the final frozen constant/table/revision, and the rationale for any reconciliation. A value that is merely copied from another emulator or lacks the architecture-required evidence remains BLOCKED_GATE.

**Evidence output:** `certification/results/core/C09.04.json`

**Pass rule:** PASS only when the Phase-0 evidence record itself satisfies every evidence-content obligation in Architecture #1 Section 9; agreement between code and an undocumented constant is not sufficient.

## C10. Architecture section 10: Global emulated time

**Architecture authority:** `C` source lines 748-981, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Prove the single canonical integer master timeline, exact derived coordinate systems and same-edge ordering, host-clock isolation, no emulated-frame skipping, and every runtime-speed/audio/Unlimited semantic in Section 10.

**Required artifacts:**

- `tests/fixtures/timing/wzsn-master-time-derivation-v1.json`
- `tests/fixtures/timing/wzsn-same-edge-order-v1.json`
- `tests/fixtures/pacing/wzsn-host-clock-poison-v1.json`
- `tests/fixtures/pacing/wzsn-presentation-drop-v1.json`
- `tests/fixtures/traces/wzsn-speed-transition-v1.json`
- `tests/fixtures/pacing/wzsn-unlimited-completeness-v1.json`
- `tests/fixtures/audio/wzsn-runtime-speed-audio-v1.json`
- `tests/fixtures/pacing/wzsn-overclock-separation-v1.json`
- frozen Phase-0 machine-profile timing artifact named by `C51` for the certified 48K/128K profile under test

**Artifact/source authority:** All listed fixtures are project-generated and checked into the public test tree. Their expected timing values are derived from the frozen machine-profile/Phase-0 timing evidence required by Architecture #1; hardware-derived timing evidence outranks convenience values. No external commercial media is required.

**Tests:**

### C10.01

Using `wzsn-master-time-derivation-v1.json`, advance the scheduler through boundary and long-run cases and prove that `wz_master_tick_t` is the one monotonically increasing canonical timestamp; CPU T-state/phase and ULA frame/scanline/raster coordinates are exact derived views of the same master tick using the selected machine profile. Include integral-ratio boundary cases and reject floating-point machine-time arithmetic.

**Evidence output:** `certification/results/core/C10.01.json`

**Pass rule:** PASS only when every derived coordinate maps exactly to/from the frozen profile definition with no alternate machine clock or floating-point timestamp path.

### C10.02

Using `wzsn-same-edge-order-v1.json`, schedule CPU/ULA/bus/interrupt/device effects that share one master-clock edge and prove that their visibility/order follows the frozen machine-profile hardware-evidence rule, independent of C function-call order. Deliberately permute host/internal invocation order and require identical canonical results.

**Evidence output:** `certification/results/core/C10.02.json`

**Pass rule:** PASS only when same-edge results are determined solely by the machine-profile ordering contract.

### C10.03

Using `wzsn-host-clock-poison-v1.json`, run identical machine work while the host monotonic-time provider returns normal, jittered, frozen, discontinuous-forward, and deliberately misleading pacing values. Prove host time can affect only pacing/sleep/performance/UI statistics and cannot change the master tick assigned to Z80 memory/I/O, ULA fetch, INT sampling, contention, border, beeper, AY, tape, or other machine events.

**Evidence output:** `certification/results/core/C10.03.json`

**Pass rule:** PASS only when all canonical state/event hashes are identical across host-clock variants.

### C10.04

Using `wzsn-presentation-drop-v1.json`, force the host to present every frame, every second frame, sparse frames, and no frames while executing the same deterministic interval. Prove the complete ULA frame sequence and all machine/device events are still emulated and canonical hashes are identical; only host presentation count may differ.

**Evidence output:** `certification/results/core/C10.04.json`

**Pass rule:** PASS only when presentation dropping never skips an emulated ULA frame or changes machine timing.

### C10.05

Run exactly `25%`, `50%`, `100%`, `200%`, `400%`, `800%`, and `Unlimited` from one deterministic initial state using `wzsn-speed-transition-v1.json`. At common master-tick checkpoints prove identical machine state and unchanged profile relationships (including 48K `224` T-states/line, `312` lines/frame, `69888` T-states/frame where that profile is under test). Verify that each finite speed changes only the intended host wall-clock pacing multiplier.

**Evidence output:** `certification/results/core/C10.05.json`

**Pass rule:** PASS only when the exact exposed speed set exists and internal machine timing is invariant across it.

### C10.06

Change runtime speed repeatedly while running and while application-paused. At every transition prove no reset/discontinuity of master tick, CPU T-state/phase, frame/raster position, CPU state, tape position, AY phase, beeper state, or peripheral timers; when paused, no machine progress occurs solely because the speed value changed.

**Evidence output:** `certification/results/core/C10.06.json`

**Pass rule:** PASS only when speed changes affect future host pacing and nothing in canonical machine state is reset or perturbed.

### C10.07

Using `wzsn-unlimited-completeness-v1.json`, execute a long deterministic workload in `Unlimited`. Compare against an unthrottled headless reference run and prove every ULA frame and timed device event occurs in identical master-tick order even when host presentation intentionally omits intermediate completed frames.

**Evidence output:** `certification/results/core/C10.07.json`

**Pass rule:** PASS only when Unlimited removes throttling, not emulation work.

### C10.08

Using `wzsn-runtime-speed-audio-v1.json`, test the complete Section-10.10 policy at `25%`, `50%`, `100%`, `200%`, `400%`, `800%`, and `Unlimited`, plus runtime crossings at `0.5x` and `2.0x`. Prove host sound is muted below `0.5x`, enabled inclusively from `0.5x..2.0x`, muted above `2.0x` and in Unlimited; canonical beeper/AY/audio state continues identically while muted; boundary crossings do not reset audio hardware; enabled host delivery follows selected runtime speed rather than buffer pressure forcing 1.0x; duration and pitch change naturally with speed; and no pitch-preserving time stretching is required or silently applied by the canonical path.

**Evidence output:** `certification/results/core/C10.08.json`

**Pass rule:** PASS only when host-audio enable/mute and natural-speed delivery match the frozen policy while canonical audio/machine state remains deterministic.

### C10.09

Using `wzsn-overclock-separation-v1.json`, prove runtime-speed commands never modify machine-profile CPU/master-clock relationships, and that no host-pacing value is reused as an emulated-overclock setting. A synthetic future-profile stub with a distinct clock must be selectable only through explicit machine-configuration/profile plumbing, never through the runtime-speed control.

**Evidence output:** `certification/results/core/C10.09.json`

**Pass rule:** PASS only when host pacing and emulated hardware clock configuration are structurally separate.

## C11. Architecture section 11: Z80 execution architecture

**Architecture authority:** `C` source lines 982-1050, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Z80 instruction, flag, interrupt, and bus-cycle correctness.

**Required artifacts:**

- `tests/fixtures/cpu/wzsn-z80-semantic-v1.json`
- `tests/fixtures/cpu/wzsn-z80-bus-v1.json`
- `external/fuse-1.9.2/z80/tests/tests.in`
- `external/fuse-1.9.2/z80/tests/tests.expected`
- `external/z80test-1.2a/z80full.tap`
- `external/z80test-1.2a/z80doc.tap`
- `external/z80test-1.2a/z80flags.tap`
- `external/z80test-1.2a/z80docflags.tap`
- `external/z80test-1.2a/z80ccf.tap`
- `external/z80test-1.2a/z80memptr.tap`
- `external/zex/zexdoc.tap`
- `external/zex/zexall.tap`
- `external/woodmass/z80tests.tap`
- `certification/manifests/z80-opcode-audit-map.json`

**Artifact/source authority:** E01 Fuse; E02 Patrik Rak; E03 Cringle/Harston ZEX; E04 Mark Woodmass. E02 separates the upstream v1.2a source revision from the exact runnable `z80test-1.2a.zip` distribution/member hashes. `z80-opcode-audit-map.json` is project-generated from the implemented decoder/handler tables and maps every reachable opcode/prefix form to its traceable implementation/test authority. E05 HALT2INT is supplemental only until its runnable archive member is explicitly pinned and is therefore not required by C11.

**Tests:**

### C11.01

Exhaustively test documented/undocumented opcode maps, prefixes, registers, documented/undocumented flags, R, IFF1/IFF2, IM0/1/2, EI delay, HALT, NMI/INT, WZ/MEMPTR/Q-sensitive behavior required by the architecture.

**Evidence output:** `certification/results/core/C11.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C11.02

For every relevant instruction compare exact M1/memory/I/O/refresh/interrupt-ack bus operations and master-tick placement, not only final registers.

**Evidence output:** `certification/results/core/C11.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C11.03

Run the complete pinned Fuse suite. Any mismatch is a certification failure until resolved; WZSN may not be changed away from stronger real-silicon evidence merely to satisfy a disputed external vector.

**Evidence output:** `certification/results/core/C11.03.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C11.04

Run the listed Rak, Cringle, and Woodmass artifacts as independent whole-machine CPU exercisers and preserve their result transcripts.

**Evidence output:** `certification/results/core/C11.04.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C11.05

Generate `z80-opcode-audit-map.json` from the actual decoder/dispatch implementation and prove every primary/CB/ED/DD/FD/DDCB/FDCB reachable form resolves to a traceable handler/table/micro-operation definition with linked project regression coverage. Generated tables are acceptable only when their generator/source and generated bytes are reproducible and reviewable. No opaque dispatch optimization may prevent the harness from identifying the implementation and bus-cycle sequence responsible for a failing opcode.

**Evidence output:** `certification/results/core/C11.05.json`

**Pass rule:** PASS only when the chosen implementation style remains fully auditable/traceable as required by Section 11.4.

## C12. Architecture section 12: Bus architecture

**Architecture authority:** `C` source lines 1051-1094, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Machine bus.

**Required artifacts:**

- `tests/fixtures/bus/wzsn-bus-readwrite-v1.json`

**Artifact/source authority:** Project-generated bus vectors.

**Tests:**

### C12.01

Exercise ROM/RAM reads, RAM writes, I/O reads/writes, interrupt acknowledge, unmapped/floating reads, contention/memory-map/ULA-visible-RAM/paging routing, keyboard, tape/EAR, ULA/port-FE, AY, Interface-1 and other peripheral dispatch with exact address/data/direction/master-tick/cycle-class traces.

**Evidence output:** `certification/results/core/C12.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C12.02

Prove CPU accesses are expressed as timed bus operations and no subsystem bypasses bus timing for machine-visible state.

**Evidence output:** `certification/results/core/C12.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## C13. Architecture section 13: Contention and arbitration

**Architecture authority:** `C` source lines 1095-1121, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Contention and arbitration.

**Required artifacts:**

- `tests/fixtures/timing/wzsn-contention-48k.tap`
- `tests/fixtures/timing/wzsn-contention-128k.tap`
- `tests/fixtures/timing/zx48-pal-timing-v1.json`
- `tests/fixtures/timing/zx128-pal-timing-v1.json`
- `tests/fixtures/timing/wzsn-contention-table-swap-v1.json`

**Artifact/source authority:** Project-generated TAP diagnostics plus frozen profile timing/contention tables.

**Tests:**

### C13.01

Verify every boundary and repeated contention delay for contended/uncontended RAM and I/O across full frames on 48K and 128K profiles.

**Evidence output:** `certification/results/core/C13.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C13.02

Test opcode fetch, data read/write, stack, indexed, block, and I/O cycles so wait insertion occurs at the correct bus phase.

**Evidence output:** `certification/results/core/C13.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C13.03

Using `wzsn-contention-table-swap-v1.json`, prove the contention decision consumes the machine model/profile, current master tick, bus-cycle class, memory address or I/O port, ULA phase, and paging state required by Section 13. Replace a test profile's contention table/parameters without changing/recompiling opcode handlers and require bus delays to follow only the replacement profile; mutation scans must reject arbitrary per-opcode compatibility delays.

**Evidence output:** `certification/results/core/C13.03.json`

**Pass rule:** PASS only when contention/arbitration belongs to the bus/machine timing model and can be corrected by profile/table evidence without rewriting CPU opcode behavior.

## C14. Architecture section 14: Same-edge event ordering

**Architecture authority:** `C` source lines 1122-1148, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Same-master-edge ordering.

**Required artifacts:**

- `tests/fixtures/timing/wzsn-same-edge-v1.json`

**Artifact/source authority:** Project-generated Phase-0 event-order table.

**Tests:**

### C14.01

Enumerate every frozen same-edge ordering pair (CPU/ULA fetch, CPU write/ULA visibility, interrupt edge/instruction boundary, FE write/raster, paging/access, audio/tape edges) and assert exact event order.

**Evidence output:** `certification/results/core/C14.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C14.02

Mutation-build each ordering rule reversed in isolation and prove at least one targeted test fails.

**Evidence output:** `certification/results/core/C14.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## C15. Architecture section 15: ULA authenticity contract

**Architecture authority:** `C` source lines 1149-1245, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** ULA timing, fetches, floating bus, and snow/profile quirks.

**Required artifacts:**

- `tests/fixtures/ula/wzsn-ula-fetch-48k.tap`
- `tests/fixtures/ula/wzsn-ula-fetch-128k.tap`
- `tests/fixtures/ula/wzsn-floating-bus-48k.tap`
- `external/ramsoft/floatspy.tap`
- `certification/gates/external/E06-floatspy-identity.json`
- `tests/fixtures/ula/wzsn-ula-snow-48k.tap`
- `external/softspectrum/timing_tests-48k_v1.0.tzx`
- `external/softspectrum/timing_tests-128k_v1.0.z80`
- `tests/fixtures/video/wzsn-raster-race.tap`
- `tests/fixtures/video/wzsn-raster-race-renamed.tap` (byte-identical generated copy of `wzsn-raster-race.tap`)
- `certification/manifests/C15-no-program-specific-raster-hacks-source-audit.json`

**Artifact/source authority:** Project-generated ULA/video fixtures; E06 Ramsoft floating-bus diagnostic; E07 SoftSpectrum 48K timing diagnostic; E08 SoftSpectrum 128K timing diagnostic. The E06 runnable TAP is admissible only after `E06-floatspy-identity.json` proves the exact member/hash from the authoritative `floatspy.zip` distribution; otherwise the E06 external subtest is BLOCKED_GATE while project-owned floating-bus tests remain executable.

**Tests:**

### C15.01

Verify the full timed ULA state contract over complete relevant raster windows: canonical master tick, raster position, memory-fetch state, bitmap/attribute pipeline state, border state, FLASH phase, contention/bus interaction, interrupt-generation state, model-specific ULA state, exact fetch addresses/values, CPU-write visibility, and floating-bus source.

**Evidence output:** `certification/results/core/C15.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C15.02

Run `floatspy.tap`; required self-test result for applicable 48K/128K profiles is the program's success result with zero unexplained mismatches.

**Evidence output:** `certification/results/core/C15.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C15.03

Test the profile's explicitly claimed snow/refresh interaction; if the certified profile excludes a variant, that exclusion must be explicit in the profile evidence rather than guessed.

**Evidence output:** `certification/results/core/C15.03.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C15.04

Run the exact external SoftSpectrum timing artifacts `timing_tests-48k_v1.0.tzx` and `timing_tests-128k_v1.0.z80` as supplemental full-machine diagnostics; any disagreement is investigated against project microtests/hardware evidence and cannot be approved visually.

**Evidence output:** `certification/results/core/C15.04.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C15.05

Prove Section 15.7's prohibition on program-specific raster hacks. Audit all deterministic core/bus/ULA/raster/timing source paths and generated timing tables for executable behavior selected by program title, host filename/path, media hash, known-demo identifier, or other software identity. Record every reviewed source path and finding in `certification/manifests/C15-no-program-specific-raster-hacks-source-audit.json`. Then run the same byte-identical project-owned raster-racing workload as both `wzsn-raster-race.tap` and `wzsn-raster-race-renamed.tap`; canonical machine/raster/event hashes at the same master-tick checkpoints must be identical. Historical compatibility tables may exist only as non-authoritative regression evidence and must not select timing behavior in the final core.

**Evidence output:** `certification/results/core/C15.05.json`

**Pass rule:** PASS only when the source audit finds no executable program-identity timing/raster special case and the byte-identical renamed-media run produces identical canonical results. Any title/path/hash/program-identity branch that changes machine timing or raster behavior is FAIL.

## C16. Architecture section 16: Raster output representation

**Architecture authority:** `C` source lines 1246-1293, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Canonical raster output.

**Required artifacts:**

- `tests/fixtures/video/wzsn-raster-pattern-48k.tap`
- `tests/fixtures/video/wzsn-raster-pattern-128k.tap`
- `tests/golden/video/wzsn-raster-48k-v1.bin`
- `tests/golden/video/wzsn-raster-128k-v1.bin`
- `certification/manifests/native-raster-sample-v1.json`

**Artifact/source authority:** Project-generated raster programs/golden canonical rasters plus the frozen logical raster-sample encoding selected by the Section-51 gate.

**Tests:**

### C16.01

Generate complete logical rasters including border/blanking representation required by the profile and compare exact canonical samples before Sokol.

**Evidence output:** `certification/results/core/C16.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C16.02

Verify model-derived raster dimensions and mapping from master tick to line/position at every boundary.

**Evidence output:** `certification/results/core/C16.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C16.03

Verify the required initial implementation materializes one logical sample per native raster-clock position using dimensions supplied by the active machine profile (including the 48K 448-by-312 timing grid and corresponding 128K profile), not hard-coded generic 48K dimensions. Validate every sample against `native-raster-sample-v1.json`: only Spectrum-logical palette/border/blanking/explicit debug diagnostic states are allowed in canonical raster storage; Metal/D3D/OpenGL/RGB/BGRA/texture-format values must be absent.

**Evidence output:** `certification/results/core/C16.03.json`

**Pass rule:** PASS only when the native logical raster buffer is the required initial derived artifact and remains host-format independent.

## C17. Architecture section 17: Spectrum color semantics

**Architecture authority:** `C` source lines 1294-1341, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Spectrum color/FLASH/BRIGHT semantics.

**Required artifacts:**

- `tests/fixtures/video/wzsn-color-flash-bright.tap`
- `tests/golden/video/wzsn-color-flash-bright-v1.bin`

**Artifact/source authority:** Project-generated color fixture.

**Tests:**

### C17.01

Exercise all INK/PAPER/BRIGHT combinations and FLASH phase transitions from emulated machine time; prove logical palette indices 0..7/8..15 behave as defined, normal black and bright black are visually identical (15 distinct visible active-display colors), the classic border uses only its 3-bit color with no BRIGHT border palette, and FLASH swaps/interprets INK/PAPER behavior rather than acting as an extra color.

**Evidence output:** `certification/results/core/C17.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C17.02

Prove host palette/presentation changes do not alter canonical Spectrum color indices or machine hashes.

**Evidence output:** `certification/results/core/C17.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## C18. Architecture section 18: Overscan, rainbow, multicolor, NIRVANA and BIFROST requirement

**Architecture authority:** `C` source lines 1342-1383, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Overscan, rainbow, multicolor, NIRVANA and BIFROST-class timing.

**Required artifacts:**

- `tests/fixtures/video/wzsn-raster-race.tap`
- `tests/fixtures/video/wzsn-raster-technique-matrix-v1.json`
- `external/canary/NIRVANAENGINE.tap`
- `certification/gates/external/E09-nirvana-member.json`
- `external/canary/NIRVANA+ENGINE.tap`
- `certification/gates/external/E10-nirvanaplus-member.json`
- `external/canary/BIFROSTENGINEV1.2L.tzx`
- `external/canary/BIFROSTENGINEV1.2H.tzx`
- `certification/gates/external/E11-bifrost-members.json`
- `external/canary/BIFROST2ENGINE.tap`
- `certification/gates/external/E12-bifrost2-member.json`
- `tests/fixtures/video/wzsn-warajevo-multicolor-overscan-regression.tap`
- `certification/manifests/C18-warajevo-raster-provenance.json`

**Artifact/source authority:** Project-generated stress fixtures; E09-E12 pinned NIRVANA/NIRVANA+/BIFROST-family canary artifacts from the catalog sources. `wzsn-warajevo-multicolor-overscan-regression.tap` is a project-owned reconstruction of historically troublesome Warajevo multicolor/overscan behavior, derived from the preserved Warajevo 2.50 source/reference material and recorded line/routine provenance in `C18-warajevo-raster-provenance.json`; it must not silently copy unlicensed third-party media bytes.

**Tests:**

### C18.01

Run `wzsn-raster-race.tap` under `wzsn-raster-technique-matrix-v1.json` and separately exercise every Section-18 mechanism: within-scanline border changes, between-scanline border changes, attribute writes immediately before ULA fetch, bitmap writes immediately before ULA fetch, precise interrupt entry, HALT synchronization, calibrated T-state delay loops, contended RAM, contended I/O, applicable floating-bus behavior, complete border/overscan output, and deliberate raster racing. Compare exact master-timed bus/raster-event hashes rather than visual plausibility.

**Evidence output:** `certification/results/core/C18.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C18.02

Run the E09-E12 NIRVANA, NIRVANA+, BIFROST*, and BIFROST*2 reference artifacts only after their corresponding archive-member identity/hash gates are frozen. Prove no application-specific timing hack is required and record deterministic checkpoint/raster evidence rather than visual-only approval. If any required archive-member identity is not yet frozen, this external-canary subtest is BLOCKED_GATE rather than guessing a ZIP member name; the project-owned raster-mechanism tests remain independently executable.

**Evidence output:** `certification/results/core/C18.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C18.03

Run `tests/fixtures/video/wzsn-warajevo-multicolor-overscan-regression.tap`, whose reconstruction provenance is frozen in `certification/manifests/C18-warajevo-raster-provenance.json`, to cover the architecture's historical Warajevo multicolor/overscan regression class. Compare exact bus/contention/ULA-fetch/border/raster-event checkpoints against the reconstructed expected behavior and, where historical Warajevo behavior conflicts with stronger hardware evidence, record the hardware-correct expected result rather than preserving the old emulator approximation. The test may not pass on visual plausibility alone.

**Evidence output:** `certification/results/core/C18.03.json`

**Pass rule:** PASS only when the provenance manifest identifies the preserved Warajevo source/reference basis, the project-owned fixture produces the expected deterministic event-level result, and no program-specific compatibility exception is required. Any unexplained divergence is FAIL.

## C19. Architecture section 19: Video presentation through Sokol

**Architecture authority:** `C` source lines 1384-1488, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Sokol video boundary and screenshot raster provenance.

**Required artifacts:**

- `tests/fixtures/video/wzsn-screenshot-source.tap`
- `tests/golden/video/wzsn-screenshot-source.png`
- `tests/fixtures/video/wzsn-presentation-matrix-v1.json`
- `tests/fixtures/host/C19-sokol-role-matrix.json`
- `certification/manifests/C19-sokol-boundary-source-audit.json`
- `tests/fixtures/video/C19-default-presentation.json`
- `tests/fixtures/host/C19-shader-isolation.json`
- `tests/fixtures/replay/C19-screenshot-input-trace.json`

**Artifact/source authority:** Project-generated known raster/PNG golden, presentation-state matrix, host-boundary audit inputs, and deterministic replay fixture; the selected default crop/border is the frozen Section-51 host-presentation gate value.

**Tests:**

### C19.01

Compare canonical raster headlessly and through Sokol presentation; scaling/filter/window changes must not alter core hashes.

**Evidence output:** `certification/results/core/C19.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C19.02

Capture GUI and Telnet screenshots from the shared completed-raster service and compare decoded PNG pixels to the selected host-visible crop; chrome/cursor/debugger content must be absent.

**Evidence output:** `certification/results/core/C19.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C19.03

Prove screenshot capture does not advance master time or mutate state.

**Evidence output:** `certification/results/core/C19.03.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C19.04

Verify the complete Section-19 Sokol responsibility boundary using `C19-sokol-role-matrix.json` plus `C19-sokol-boundary-source-audit.json`. Sokol may create/manage the application window, receive host input, create graphics resources, upload the completed native raster/converted host texture, draw, scale/letterbox, present fullscreen, and host optional presentation shaders. It must not determine ULA raster position/fetch value, contention, border timing, FLASH phase, CPU-write visibility, or emulated frame progression. The source audit must identify every Sokol-facing translation unit and prove that all machine-timing/state decisions remain on the deterministic-core side of the boundary.

**Evidence output:** `certification/results/core/C19.04.json`

**Pass rule:** PASS only when every allowed responsibility is host/presentation-only and every forbidden responsibility is absent from Sokol/host decision logic. Any host presentation call that determines canonical machine behavior is FAIL.

### C19.05

Verify the default presentation contract using `C19-default-presentation.json`: preserve the selected native aspect, native pixel structure, and border-timing visibility; scaling/letterboxing is host-only. Compare identical canonical raster/state hashes across at least the supported graphics backends exercised by the platform matrix, including backend changes that alter only presentation.

**Evidence output:** `certification/results/core/C19.05.json`

**Pass rule:** PASS only when default presentation does not hide required border timing or distort the architecture-frozen aspect/pixel contract and backend/scaling choices leave canonical core results unchanged.

### C19.06

Exercise `C19-shader-isolation.json`. If optional CRT/analog shaders are implemented, vary each supported presentation effect and inject shader creation/compile/runtime failure where the host layer permits; canonical raster and machine hashes must remain unchanged and the emulator must fall back or report a host-presentation failure without changing emulation. If no optional shader exists in the certified build, record that fact and prove the canonical raster reaches presentation without a shader dependency.

**Evidence output:** `certification/results/core/C19.06.json`

**Pass rule:** PASS only when shader choice/failure can affect appearance only, never canonical emulation.

### C19.07

Capture screenshots while both RUNNING and PAUSED. Using `C19-screenshot-input-trace.json`, prove that screenshot creation adds no deterministic machine-input event, does not advance master time, and does not alter canonical state; host wall-clock-derived filename/log metadata may differ and is excluded from deterministic hashes.

**Evidence output:** `certification/results/core/C19.07.json`

**Pass rule:** PASS only when screenshots are valid in both application run states and remain pure host-output operations with no deterministic-input or machine-state side effect.

## C20. Architecture section 20: Audio authenticity contract

**Architecture authority:** `C` source lines 1489-1596, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Beeper, AY and deterministic mixer contract.

**Required artifacts:**

- `tests/fixtures/audio/wzsn-beeper-events-v1.json`
- `tests/fixtures/audio/wzsn-ay-register-events-v1.json`
- `tests/fixtures/audio/wzsn-mixer-vectors-q16_16-v1.json`
- `tests/fixtures/audio/wzsn-ay-channel-mixer-v1.json`
- `tests/golden/audio/wzsn-audio-v1.pcm`
- `src/audio/wz_audio_mixer.h`
- `src/audio/wz_ay_mixer_policy.h`
- `certification/manifests/C20-audio-symbol-contract.json`

**Artifact/source authority:** Project-generated exact beeper/AY event vectors, AY channel/mixer policy vectors, golden canonical PCM, the architecture-proposed canonical audio header, the project-owned frozen AY attenuation-table source, and a generated symbol-contract audit mapping the exact frozen identifiers to their declarations/values.

**Tests:**

### C20.01

Verify separate FE MIC/speaker-level semantics where profile exposes them, master-timestamped beeper transitions, AY register/state evolution, Q16.16 mixer arithmetic, int64 accumulation, specified saturation/rounding, and frozen AY attenuation table.

**Evidence output:** `certification/results/core/C20.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C20.02

Audit `src/audio/wz_audio_mixer.h` and the compiled interface represented by `C20-audio-symbol-contract.json`: `wz_audio_sample_t` must exist and be a signed 32-bit Q16.16 sample type; `wz_audio_accumulator_t` must exist and be a signed 64-bit accumulator type; and `WZ_CANONICAL_AUDIO_SAMPLE_RATE` must exist with the exact value `44100`. Then generate canonical 44.1-kHz PCM from integer/rational master-time mapping and compare byte-for-byte across required hosts/compilers. An equivalent anonymous/local typedef or numeric literal does not satisfy the frozen identifier contract.

**Evidence output:** `certification/results/core/C20.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C20.03

At speed values outside 0.5x..2.0x, host output must mute while canonical beeper/AY/mixer state continues unchanged.

**Evidence output:** `certification/results/core/C20.03.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C20.04

Using `wzsn-ay-register-events-v1.json`, `wzsn-ay-channel-mixer-v1.json`, and the exact project table in `src/audio/wz_ay_mixer_policy.h`, prove the complete initial 128K AY contract: three mono channels; independent tone generators; noise generator; envelope generator; mixer/register state; master-timestamped register writes; 16-level normalized attenuation represented in Q16.16; equal channel summation into the canonical mixer; and host-stereo independence. Re-run the same AY event stream while varying host audio callback/request cadence, including no host requests; AY counters, envelope/noise/tone state, mixer phase, and canonical PCM must be identical at the same master ticks.

**Evidence output:** `certification/results/core/C20.04.json`

**Pass rule:** PASS only when every listed AY generator/mixer responsibility advances solely from emulated time and the frozen machine-model clock relationship, with no host-audio-demand influence and no host-stereo-dependent canonical result.

## C21. Architecture section 21: Sokol audio contract

**Architecture authority:** `C` source lines 1597-1644, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Sokol audio push delivery.

**Required artifacts:**

- `tests/fixtures/audio/wzsn-audio-v1.pcm`
- `tests/fixtures/audio/wzsn-sokol-push-degradation-v1.json`

**Artifact/source authority:** Project-generated canonical PCM and host push/degradation scenarios.

**Tests:**

### C21.01

Feed canonical PCM to the Sokol push path under normal, underrun, overrun, slow-device, and absent-device conditions; host queue state must never drive CPU execution or alter canonical audio.

**Evidence output:** `certification/results/core/C21.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C21.02

Repeat core workload with Sokol audio enabled/disabled and compare canonical hashes.

**Evidence output:** `certification/results/core/C21.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C21.03

Using `wzsn-sokol-push-degradation-v1.json`, prove the baseline integration is push-only (`saudio_push` or pinned equivalent) and no callback/pull request can execute CPU/machine work. Inside 0.5x..2.0x, force an unsustainable slow/blocked audio device and require explicit host-audio degradation/drop rather than silent emulation-speed change; outside that range prove host samples are discarded/not accumulated into an unbounded queue. Repeat muted operation with PCM materialization suppressed and require beeper, AY, mixer phase/accumulator and every Spectrum-observable audio state to match a fully materialized reference run.

**Evidence output:** `certification/results/core/C21.03.json`

**Pass rule:** PASS only when queue fullness is purely a bounded host-delivery concern and can never clock or retime the deterministic emulator.

## C22. Architecture section 22: Input architecture

**Architecture authority:** `C` source lines 1645-1744, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Keyboard matrix, input arbiter, and Kempston joystick.

**Required artifacts:**

- `tests/fixtures/input/wzsn-keyboard-matrix-v1.json`
- `tests/fixtures/input/wzsn-input-ownership-v1.json`
- `tests/fixtures/input/wzsn-kempston-port-v1.json`
- `tests/fixtures/input/wzsn-focus-release-v1.json`
- `design/input-hardware-evidence.md`

**Artifact/source authority:** Project-generated input vectors. `design/input-hardware-evidence.md` freezes the Phase-5 keyboard-ghosting conclusion and exact Kempston I/O decode/bit values from cited hardware/documented evidence before certification; no host mapping is an oracle for Kempston semantics.

**Tests:**

### C22.01

Test all 40 Spectrum keys, every keyboard row, multi-row selection, selected multi-key combinations, and the Phase-5 frozen ghosting/electrical conclusion.

**Evidence output:** `certification/results/core/C22.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C22.02

Drive equivalent local/Telnet/replay physical matrix transitions through the host arbiter and compare effective matrix state while preserving independent source ownership; explicitly prove OR-merging of simultaneously held sources, emission of only effective matrix transitions, and exact canonical master-tick assignment.

**Evidence output:** `certification/results/core/C22.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C22.03

Test Kempston RIGHT/LEFT/DOWN/UP/FIRE individually and in combinations with exact frozen decode/bit semantics; the core must receive normalized joystick state, not host keyboard shortcuts.

**Evidence output:** `certification/results/core/C22.03.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C22.04

Using `wzsn-focus-release-v1.json`, lose/regain GUI focus while local and Telnet keys are independently held: any local-release policy must occur only in the host input arbiter and must not release the other source's ownership or mutate unrelated Spectrum state. Record the normalized effective transitions, replay the same trace headlessly with Sokol/Telnet absent, and require identical keyboard/core hashes.

**Evidence output:** `certification/results/core/C22.04.json`

**Pass rule:** PASS only when focus/UI events are host policy, source ownership is preserved, and deterministic traces are independently headless-replayable.

## C23. Architecture section 23: Tape architecture

**Architecture authority:** `C` source lines 1745-2069, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Tape subsystem, Normal timing, Warajevo TAP, TZX, WAV, SAVE, and trap equivalence.

**Required artifacts:**

- `design/media-format-support.md`
- `reference/TZXformat-v1.20.html`
- `certification/manifests/R01-tzx-reference-hash.json`
- `tests/fixtures/tape/wzsn-tap-standard-oneblock.tap`
- `tests/fixtures/tape/wzsn-tap-standard-multiblock.tap`
- `tests/fixtures/tape/wzsn-warajevo-native-minimal.tap`
- `tests/fixtures/tape/wzsn-warajevo-native-edge.tap`
- `tests/fixtures/tzx/wzsn-tzx-standard-speed.tzx`
- `tests/fixtures/tzx/wzsn-tzx-turbo.tzx`
- `tests/fixtures/tzx/wzsn-tzx-pure-tone.tzx`
- `tests/fixtures/tzx/wzsn-tzx-pulse-sequence.tzx`
- `tests/fixtures/tzx/wzsn-tzx-pure-data.tzx`
- `tests/fixtures/tzx/wzsn-tzx-direct-recording.tzx`
- `tests/fixtures/tzx/wzsn-tzx-csw-recording.tzx`
- `tests/fixtures/tzx/wzsn-tzx-generalized-data.tzx`
- `tests/fixtures/tzx/wzsn-tzx-control-flow.tzx`
- `tests/fixtures/tzx/wzsn-tzx-malformed.tzx`
- `tests/fixtures/tape/wzsn-tape-44100-mono.wav`
- `tests/fixtures/tape/wzsn-tape-48000-noisy.wav`
- `tests/fixtures/tape/wzsn-trap-rom-standard.tap`
- `tests/fixtures/tape/wzsn-trap-checksum-error.tap`
- `tests/fixtures/tzx/wzsn-trap-custom-loader.tzx`
- `tests/fixtures/tzx/wzsn-trap-copy-protection.tzx`
- `tests/fixtures/tape/wzsn-trap-unsupported-rom.tap`
- `tests/fixtures/tape/wzsn-format-disposition-v1.json`

**Artifact/source authority:** Project-generated TAP/TZX/WAV fixtures governed by the frozen Phase-7 media-format matrix. TZX block/timing semantics use the exact pinned local R01 copy `reference/TZXformat-v1.20.html`; `R01-tzx-reference-hash.json` records source URL, acquisition date, exact local filename, and SHA-256 so Phase-7 cannot silently switch TZX specifications. No SNA/Z80 snapshot specification is an authority for this tape section.

**Tests:**

### C23.01

Validate standard TAP and every REQUIRED Warajevo-native TAP variant for read/load **and write/save** against the frozen Phase-7 `design/media-format-support.md`; signature/file-identification rules, block semantics, checksums, truncation and malformed cases are exact and failures are atomic.

**Evidence output:** `certification/results/core/C23.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C23.02

For every TZX v1.20 block marked SUPPORTED, run minimal, boundary, malformed, control-flow, and adjacent-block/no-gap vectors; metadata-only/unsupported blocks behave exactly as the frozen matrix states. WAV accepted PCM/container forms and edge-decoding rules are likewise frozen and deterministic.

**Evidence output:** `certification/results/core/C23.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C23.03

Normal mode is the default and executes the real emulated loader against master-timed EAR edges. SAVE emits canonical MIC edges. Runtime speed changes during a pulse change only host pacing; tape position/pulse duration and CPU/ULA/interrupt/audio relative timing stay on the same master timeline.

**Evidence output:** `certification/results/core/C23.03.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C23.04

Exercise EAR_MIC cassette ownership: mounted ordinary media may remain application-known but ordinary tape may neither drive EAR nor consume/record MIC while routed network owns the socket; stack bootstrap uses Architecture-#3 BOOTSTRAP_TAPE pulse path only; leaving EAR_MIC uses cold reconfiguration.

**Evidence output:** `certification/results/core/C23.04.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C23.05

Using `wzsn-trap-rom-standard.tap` and `wzsn-trap-checksum-error.tap`, for every eligible Instant/Trap path compare the canonical **post-load handoff** state against Normal, including loaded bytes, checksum/result, CPU registers/flags/stack/PC, ROM/system variables, tape position, paging/device state and any elapsed-time-dependent master/raster/FLASH/interrupt/peripheral state. The omitted transient loading history is not required to match, but the endpoint is.

**Evidence output:** `certification/results/core/C23.05.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C23.06

Using `wzsn-trap-custom-loader.tzx`, `wzsn-trap-copy-protection.tzx`, and `wzsn-trap-unsupported-rom.tap`, prove conservative trap eligibility: custom loaders/copy protection/unusual pulses/timing-dependent software/unsupported ROMs fall back to Normal; any attempted trap that cannot preserve endpoint semantics fails atomically or falls back without partial injection.

**Evidence output:** `certification/results/core/C23.06.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C23.07

Demonstrate that accelerated Normal mode (including Unlimited) preserves the complete real loader/EAR/checksum/border/interrupt/ULA execution path and is the preferred acceleration when transient equivalence is required.

**Evidence output:** `certification/results/core/C23.07.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C23.08

Verify live physical cassette capture remains LATER and unavailable in the initial architecture; no implementation ticket chooses real-time synchronization versus capture/buffer policy without a later explicit design freeze.

**Evidence output:** `certification/results/core/C23.08.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C23.09

Run Section-23.13 Normal and Instant regression lists at all applicable speed multipliers and record edge, border, loader/error, endpoint, tape-position and fallback evidence.

**Evidence output:** `certification/results/core/C23.09.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C23.10

Using `wzsn-format-disposition-v1.json`, verify the complete Section-23.12 matrix itself: Standard TAP and Warajevo-native TAP are REQUIRED read+write; TZX is REQUIRED read/timing but NOT required to write; WAV/sampled audio is REQUIRED deterministic read/decode with no write requirement; VOC/other legacy audio remains LATER and cannot become an initial release blocker. Reject any implementation that silently promotes an unsupported TZX block or later VOC variant into ad-hoc behavior outside the frozen `design/media-format-support.md`.

**Evidence output:** `certification/results/core/C23.10.json`

**Pass rule:** PASS only when every format/disposition row and write/read obligation is enforced exactly.

## C24. Architecture section 24: Networking-mode arbitration, Interface 1, Microdrive, and original ZX Net

**Architecture authority:** `C` source lines 2070-2236, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Networking-mode arbitration, Interface 1, Microdrive, original ZX Net, and EAR_MIC reservation.

**Required artifacts:**

- `design/interface1-microdrive-zxnet.md`
- `tests/fixtures/networking/wzsn-networking-modes-v1.json`
- `tests/fixtures/interface1/wzsn-if1-rom-page.tap`
- `tests/fixtures/interface1/wzsn-if1-zxnet-v1.tap`
- `tests/fixtures/mdr/wzsn-mdr-minimal.mdr`
- `tests/fixtures/mdr/wzsn-mdr-multifile.mdr`
- `tests/fixtures/mdr/wzsn-mdr-write-protect.mdr`
- `tests/fixtures/rom/zx-interface1-certification.rom`
- `tests/fixtures/interface1/wzsn-if1-serial-state-v1.json`
- `tests/fixtures/interface1/wzsn-if1-rom-compat-v1.json`
- `tests/fixtures/networking/wzsn-networking-transition-edge-v1.json`
- `tests/fixtures/networking/wzsn-control-port-independence-v1.json`
- `tests/fixtures/mdr/wzsn-mdr-malformed-truncated.mdr`
- `tests/fixtures/mdr/wzsn-mdr-dirty-transition-v1.json`
- `tests/fixtures/zxnet/wzsn-zxnet-loopback-v1.json`
- `certification/manifests/C24-core-host-boundary-source-audit.json`
- `certification/manifests/interface1-phase10-freeze-ledger.json`

**Artifact/source authority:** Project-generated IF1/MDR/networking/ZX Net fixtures, deterministic host-boundary source audit, and legally supplied Interface-1 ROM hash frozen in the companion spec. The preserved `reference/original-warajevo/` `MDRIVE.ASM` material is migration/differential evidence, not authority over stronger real-hardware behavior.

**Tests:**

### C24.01

Exhaustively test the single enum NONE/INTERFACE1/EAR_MIC using `wzsn-networking-modes-v1.json` and `wzsn-networking-transition-edge-v1.json`: no illegal combination can be represented; fresh launch defaults NONE; a genuine mode change is serialized through the application/orchestrator and cold-recreates the same machine model while destroying old RAM, resident hooks, paged-ROM state, Interface-1 state, and Ear+Mic resident-stack state and preserving only application run/pause state. Selecting the already-active mode is idempotent and must not cold-reconfigure, and no transition may hot-add/hot-remove Interface 1 or the routed MIC/EAR environment from the existing machine context.

**Evidence output:** `certification/results/core/C24.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C24.02

Using `wzsn-mdr-dirty-transition-v1.json`, prove leaving INTERFACE1 with dirty writable MDR requires successful local application-controlled commit/flush or cancellation; no silent loss is permitted. While local write/confirmation is required, remote `machine.networking.set` is unavailable. After successful departure, Microdrive slots are logically detached and a later return to INTERFACE1 requires normal remounting.

**Evidence output:** `certification/results/core/C24.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C24.03

Against the frozen Phase-10 companion spec, test IF1 ROM paging, I/O decode, device reset/latches, and the complete minimum Microdrive contract using `wzsn-mdr-minimal.mdr`, `wzsn-mdr-multifile.mdr`, `wzsn-mdr-write-protect.mdr`, and `wzsn-mdr-malformed-truncated.mdr`: insertion/ejection, deterministic cartridge position/motor state, deterministic sector/header/data visibility, write-protect/write semantics, explicit malformed/truncated errors, application-controlled save/flush rather than hidden core writes, and canonical state hashing sufficient to reproduce the same run at the same master tick.

**Evidence output:** `certification/results/core/C24.03.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C24.04

Before Architecture #3 is certified, EAR_MIC reports unavailable; afterward only the certified 48K Issue-2 profile may select it, with IF1/Microdrive/ZX Net disabled and no ROM paging or RAM auto-install.

**Evidence output:** `certification/results/core/C24.04.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C24.05

Reconcile `interface1-phase10-freeze-ledger.json` one-for-one with every Section-24.6 freeze item and refuse certification/ticket eligibility if any is absent or unresolved: I/O decode/paging, ROM identities, registers/latches/reset, Microdrive bit/byte/sector timing and MDR interpretation, old/new Interface-1 ROM compatibility, original ZX Net transitions/timing, NONE/INTERFACE1 cold-arbitration with reserved EAR_MIC exclusion, canonical serialization/hash fields, Warajevo differential tests, and hardware/reference tests for disagreements. Use `wzsn-if1-serial-state-v1.json` to exercise the authentic Interface-1 serial-facing machine state without exposing host handles/sockets/printer objects, and `wzsn-if1-rom-compat-v1.json` to prove any evidence-required old/new ROM behavioral distinction is represented explicitly rather than guessed.

**Evidence output:** `certification/results/core/C24.05.json`

**Pass rule:** PASS only when the complete Phase-10 specification gate is frozen before dependent implementation and every claimed Interface-1 machine-visible function remains deterministic and Sokol/host independent.

### C24.06

Using `wzsn-zxnet-loopback-v1.json` and `C24-core-host-boundary-source-audit.json`, prove Section 24.2/24.4/24.5 ownership boundaries: Interface 1, Microdrive, original ZX Net, and networking-mode state advance only from canonical emulated time; host file/network timing never directly mutates them; the core contains no host file handle/socket/printer object; and original ZX Net passes its Spectrum-visible state/timing tests in a deterministic single-process loopback with no host packet-timing dependency. If a later multi-instance host transport exists, inject the same received bytes under varied host-arrival schedules and prove the application/orchestrator normalizes/schedules them before the core sees any ZX Net state change.

**Evidence output:** `certification/results/core/C24.06.json`

**Pass rule:** PASS only when all device timing/state is canonical and the loopback result is independent of host transport timing and host object identity.

### C24.07

Using `wzsn-control-port-independence-v1.json`, vary the per-process Telnet Control Port allocation, including unavailable/exhausted host-listener state, while holding the same machine/networking configuration. Prove the selected Control Port never changes NONE/INTERFACE1/EAR_MIC state, snapshots, deterministic hashes, or machine behavior.

**Evidence output:** `certification/results/core/C24.07.json`

**Pass rule:** PASS only when Control Port session state is demonstrably unrelated to the emulated networking-mode state.

## C25. Architecture section 25: Snapshot architecture

**Architecture authority:** `C` source lines 2237-2298, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** SNA/Z80 snapshot compatibility and atomicity.

**Required artifacts:**

- `design/media-format-support.md`
- `reference/formats.html`
- `reference/snaformat.html`
- `reference/z80format.html`
- `certification/manifests/R02-snapshot-reference-hashes.json`
- `tests/fixtures/snapshot/wzsn-snapshot-format-disposition-v1.json`
- `tests/fixtures/snapshot/wzsn-native-state-v1.json`
- `certification/manifests/C25-snapshot-parser-source-audit.json`
- `tests/fixtures/snapshot/wzsn-sna-48k-canonical.sna`
- `tests/fixtures/snapshot/wzsn-sna-128k-canonical.sna`
- `tests/fixtures/snapshot/wzsn-sna-48k-stack-boundary.sna`
- `tests/fixtures/snapshot/wzsn-sna-malformed-truncated.sna`
- `tests/fixtures/snapshot/wzsn-z80-v1-uncompressed.z80`
- `tests/fixtures/snapshot/wzsn-z80-v1-compressed.z80`
- `tests/fixtures/snapshot/wzsn-z80-v2-48k.z80`
- `tests/fixtures/snapshot/wzsn-z80-v2-128k.z80`
- `tests/fixtures/snapshot/wzsn-z80-v3-128k.z80`
- `tests/fixtures/snapshot/wzsn-z80-malformed-rle.z80`
- `tests/fixtures/snapshot/wzsn-sna-valid-wrong-extension.z80`
- `tests/fixtures/snapshot/wzsn-z80-valid-wrong-extension.sna`
- `reference/original-warajevo/snapshots/warajevo-sna-48k-reference.sna`
- `reference/original-warajevo/snapshots/warajevo-z80-v2-reference.z80`

**Artifact/source authority:** Project-generated fixtures governed by the frozen Phase-8 SNA/Z80 byte-level matrix; R02 supplies exact pinned local copies of the general index plus the SNA and Z80 format pages. `R02-snapshot-reference-hashes.json` records source URL, acquisition date, exact local filename, and SHA-256 for each page so Phase-8 cannot silently switch references.

**Tests:**

### C25.01

Validate every Phase-8 frozen SNA and Z80 variant including compression, PC encoding, paging, hardware-mode fields, R/IFF/IM/border and represented AY/timing fields.

**Evidence output:** `certification/results/core/C25.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C25.02

Load malformed/unsupported snapshots into validated temporary state and prove live state hash remains unchanged on every failure path.

**Evidence output:** `certification/results/core/C25.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C25.03

Round-trip canonical SNA 48K/128K and Z80 v2-class save forms; where a state is not representable, require controlled error rather than state loss.

**Evidence output:** `certification/results/core/C25.03.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C25.04

Using `wzsn-native-state-v1.json`, verify the internally generated WZSN state format is explicitly versioned and portable x86-64↔AArch64, records `networking_mode` plus every active deterministic peripheral state required to restore that configuration, and can never encode or restore simultaneous Interface-1/EAR_MIC activation. Reconcile historical SNA/Z80 treatment of external Interface-1 state and an active EAR_MIC environment against the frozen `design/media-format-support.md`; import/export must not invent a networking ROM or silently create illegal combined state.

**Evidence output:** `certification/results/core/C25.04.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C25.05

Load `wzsn-sna-valid-wrong-extension.z80` and `wzsn-z80-valid-wrong-extension.sna` and prove filename extension is only a hint: the parser validates actual structure/semantics before selecting format/model and before committing live state. Corrupt structural/model fields in mutation copies and require atomic rejection.

**Evidence output:** `certification/results/core/C25.05.json`

**Pass rule:** PASS only when model/format selection follows defined snapshot semantics rather than extension alone.

### C25.06

Load the preserved `warajevo-sna-48k-reference.sna` and `warajevo-z80-v2-reference.z80` through WZSN and compare all format-representable state against the corresponding preserved Warajevo reference behavior. Any intentional divergence must be resolved by the architecture evidence hierarchy and recorded; no silent compatibility difference is accepted.

**Evidence output:** `certification/results/core/C25.06.json`

**Pass rule:** PASS only when applicable original-Warajevo snapshot behavior has explicit differential evidence and hardware-correct deviations are documented.

### C25.07

Using `wzsn-snapshot-format-disposition-v1.json`, verify the complete Section-25.1 matrix exactly: SNA 48K REQUIRED load+save with exact 48K restore; SNA 128K REQUIRED load+save with exact paging/state restore; Z80 v1 REQUIRED load and no required save; Z80 v2 REQUIRED load+save and is the canonical initial `.Z80` writer when representable; Z80 v3 REQUIRED load and no required save; the Warajevo Timex Z80 extension remains LATER with TS2068; other historical snapshots remain explicitly LATER. For a selected state not losslessly representable by the canonical v2 writer, require an explicitly supported alternative format or controlled error—never silent state loss.

**Evidence output:** `certification/results/core/C25.07.json`

**Pass rule:** PASS only when every load/save/LATER disposition is exact and no unsupported writer behavior is silently promoted.

### C25.08

Audit the snapshot parser implementation with `C25-snapshot-parser-source-audit.json`: file fields are parsed explicitly with defined endianness; every structural length/range is bounds-checked before use; no on-disk snapshot is reinterpreted through a raw C-structure cast; unsupported/malformed variants return controlled deterministic errors; and model selection is made from validated format semantics. Cross-check the audit with the malformed/truncated/RLE mutation fixtures.

**Evidence output:** `certification/results/core/C25.08.json`

**Pass rule:** PASS only when parser safety/portability is structural, not dependent on host alignment, padding, or endianness.

## C26. Architecture section 26: Application/orchestrator and host services

**Architecture authority:** `C` source lines 2299-2334, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Application/orchestrator and headless services.

**Required artifacts:**

- `tests/fixtures/app/wzsn-command-sequence-v1.json`
- `tests/fixtures/app/wzsn-orchestrator-io-contract-v1.json`
- `certification/manifests/C26-orchestrator-boundary-source-audit.json`

**Artifact/source authority:** Project-generated command/I-O contract fixtures and a source-boundary audit covering deterministic-core, application/orchestrator, screenshot, Sokol, pacing, and host-interface call edges.

**Tests:**

### C26.01

Run identical semantic command sequence through headless orchestrator and GUI application; normalized core inputs and canonical outputs must match.

**Evidence output:** `certification/results/core/C26.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C26.02

Using `C26-orchestrator-boundary-source-audit.json`, verify only the application/orchestrator owns shared command-registry serialization, GUI/Telnet/test convergence, host-input normalization/scheduling, screenshot routing, runtime-speed audio policy/Sokol delivery, and host-clock pacing. Prove the deterministic core contains no call/dependency on Sokol or a generic host API and that host-facing project interfaces, if present for organization, are not dependencies of Spectrum machine logic.

**Evidence output:** `certification/results/core/C26.02.json`

**Pass rule:** PASS only when the source/call graph preserves the architecture boundary with no machine-state/timing decision delegated to a host API.

### C26.03

Using `wzsn-orchestrator-io-contract-v1.json`, exercise the exact Section-26 data contract in a headless harness: the core consumes only normalized timed input and explicit media/control requests and produces canonical raster, canonical audio, and deterministic state/trace results. Run the same fixture without window, GPU, live keyboard, Telnet, or audio device and require the same canonical outputs/checkpoints as the application run.

**Evidence output:** `certification/results/core/C26.03.json`

**Pass rule:** PASS only when the headless orchestrator/test harness is sufficient for automated testing and deterministic replay and all core inputs/outputs remain inside the frozen machine-domain contract.

## C27. Architecture section 27: Sokol integration contract

**Architecture authority:** `C` source lines 2335-2424, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Pinned Sokol integration.

**Required artifacts:**

- `third_party/sokol/`
- `certification/manifests/build.json`
- `certification/manifests/sokol-surface-v1.json`
- `certification/manifests/sokol-backend-matrix-v1.json`

**Artifact/source authority:** R05 official Sokol repository; exact commit frozen before Phase 5. `sokol-surface-v1.json` records the approved header surface and controlled implementation translation unit(s); `sokol-backend-matrix-v1.json` records the compile-time platform backend selection, including the frozen Linux graphics backend within the X11-compatible baseline.

**Tests:**

### C27.01

Verify exact Sokol revision in source and build manifest; release build must not use an unpinned installed copy.

**Evidence output:** `certification/results/core/C27.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C27.02

Run presentation/audio/input/package tests on every supported backend; Sokol upgrade cannot change canonical core hashes.

**Evidence output:** `certification/results/core/C27.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C27.03

On macOS, Objective-C host integration must remain outside the C core.

**Evidence output:** `certification/results/core/C27.03.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C27.04

Static-audit the vendored Sokol integration against `sokol-surface-v1.json`: required baseline use is limited to `sokol_app.h`, `sokol_gfx.h`, `sokol_audio.h`, `sokol_time.h`, `sokol_glue.h` where appropriate and optional `sokol_log.h`; `sokol_framebuffer.h` is allowed only as a presentation convenience; any other Sokol header requires a recorded concrete requirement. Verify implementation defines/headers are compiled in controlled application/host translation units and no host/Sokol type leaks into the C core.

**Evidence output:** `certification/results/core/C27.04.json`

**Pass rule:** PASS only when the pinned Sokol surface, compile-time backend selection and source integration match Section 27 rather than an ambient host install.

### C27.05

Reconcile every certified application build against `sokol-backend-matrix-v1.json` and the pinned Sokol/build revision: Windows uses Sokol app with D3D11 graphics and WASAPI audio; macOS uses Sokol app with Metal graphics and CoreAudio; Linux uses the X11-compatible desktop path, the explicitly frozen/tested Linux graphics backend, and ALSA audio. Backend choice is compile-time per release target. Native Wayland is not required by the initial architecture and must not silently replace the X11-compatible baseline in a certified build; any later Wayland path is recorded as a later host-platform addition.

**Evidence output:** `certification/results/core/C27.05.json`

**Pass rule:** PASS only when each target's compiled backend tuple matches the frozen matrix/build manifest and canonical core hashes remain independent of that backend choice.

## C28. Architecture section 28: Single-binary distribution contract

**Architecture authority:** `C` source lines 2425-2535, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Single-binary packaging and multi-instance host-data safety.

**Required artifacts:**

- `certification/scripts/inspect-dependencies`
- `certification/manifests/release-artifacts.json`
- `tests/fixtures/concurrency/wzsn-multi-instance-plan-v1.json`

**Artifact/source authority:** `release-artifacts.json` identifies the exact built executable or platform bundle under certification for every target, including path, file hash, architecture, and bundle-contained executable where applicable; the dependency inspector analyzes those exact bytes. The concurrency plan is project-generated.

**Tests:**

### C28.01

For every entry in `certification/manifests/release-artifacts.json`, run `certification/scripts/inspect-dependencies` against the exact hashed executable bytes (and the bundle-contained executable for macOS). Verify WZSN/Sokol/approved application code is in the program binary and no project-shipped multimedia DLL/.so/.dylib is required; normal OS libraries/frameworks/drivers remain permitted exactly as Section 28 states. Verify the Windows target shape is `Warajevo-ZX-Spectrum-Next.exe`, macOS may use the signed/notarized `Warajevo ZX Spectrum Next.app` distribution container without project-shipped third-party dylibs, and Linux x86-64/AArch64 each contain one WZSN ELF executable with Sokol compiled in.

**Evidence output:** `certification/results/core/C28.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C28.02

Verify normal configuration/cache/screenshots/snapshots/media files may be created after launch and default configuration is compiled in where practical; these generated host files are not confused with the one-program-binary requirement or deterministic machine state.

**Evidence output:** `certification/results/core/C28.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C28.03

Race two or more processes against preferences, writable media aliases, snapshots/exports/conversions/temp outputs, and Telnet screenshot naming; prove atomic valid writes, resolved-identity one-writer media ownership (or deliberate read-only fallback), unique/exclusive temp names, atomic commit/replacement, and no clobber/interleaving. The dynamically selected Control Port is never persisted.

**Evidence output:** `certification/results/core/C28.03.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## C29. Architecture section 29: ROM and firmware distribution constraint

**Architecture authority:** `C` source lines 2536-2554, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** ROM/firmware redistribution and identity.

**Required artifacts:**

- `certification/manifests/roms.json`
- `certification/manifests/rom-license-separation.json`
- `LICENSE.txt`
- `NOTICE.md`

**Artifact/source authority:** Project ROM manifest; legally supplied ROM images.

**Tests:**

### C29.01

Verify every certification ROM has an exact hash and declared redistribution status; embedded ROM bytes are present only when redistribution rights are established.

**Evidence output:** `certification/results/core/C29.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C29.02

Build with external user-supplied ROM configuration and prove the legal/distribution exception does not alter emulator architectural correctness.

**Evidence output:** `certification/results/core/C29.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C29.03

Reconcile `rom-license-separation.json`, `roms.json`, `LICENSE.txt`, `NOTICE.md`, and the release/package manifest. Prove the build/release system tracks ROM/firmware redistribution permission and exact identity separately from the emulator source-code license: emulator GPL status must not be used as evidence that Sinclair/Interface-1/other firmware may be redistributed, and lack of ROM redistribution rights must not be reported as an emulator-source licensing failure.

**Evidence output:** `certification/results/core/C29.03.json`

**Pass rule:** PASS only when source licensing and every ROM/firmware licensing decision are separate, explicit records and packaging follows the ROM-specific record.

## C30. Architecture section 30: UI and application-command authority boundary

**Architecture authority:** `C` source lines 2555-2607, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Core/UI authority boundary.

**Required artifacts:**

- `warajevo-zx-spectrum-next-ui-architecture.md`
- `certification/manifests/command-authority.json`
- `certification/manifests/C30-core-ui-authority-ledger.json`
- `certification/manifests/ui-toolkit-phase12.json`

**Artifact/source authority:** Architecture #2 and project authority manifest; `ui-toolkit-phase12.json` is produced only after the Architecture-2 Phase-12 toolkit gate freezes the selected toolkit/revision/integration.

**Tests:**

### C30.01

Static/dynamic audit proves GUI/Telnet registry requests machine operations through orchestrator and does not redefine reset/speed/input/screenshot/Telnet transport semantics.

**Evidence output:** `certification/results/core/C30.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C30.02

Run UI acceptance suite and compare canonical hashes with equivalent headless commands.

**Evidence output:** `certification/results/core/C30.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C30.03

Against `ui-toolkit-phase12.json`, prove the selected UI toolkit/revision is statically incorporable into the application binary, lives wholly outside the emulation core, and is absent from a headless-core/test link. Exercise menus/file choosers/command registry/Telnet control/screenshot operations while poisoning their host timing and require no canonical Spectrum-time dependency. Refuse Phase-12 exit unless the complete Architecture-2 initial-workflow/acceptance suite passes on disk-built binaries with no core-state divergence.

**Evidence output:** `certification/results/core/C30.03.json`

**Pass rule:** PASS only when the exact Section-30 authority/toolkit/headless boundaries are all satisfied.

### C30.04

Reconcile `C30-core-ui-authority-ledger.json` one-for-one with Section 30. The UI architecture must own: top-level menu tree/toolbar; Machine/Media status; media-manager workflows; debugger/monitor presentation; settings and Compatibility Tools presentation; shared command registry/stable IDs; GUI/toolbar/Telnet/tests/future-CLI projections; Telnet application-control commands; remote permission/UI-visible control policy; screenshot naming/destination/reporting; and UI workflow/cancel/accessibility/error/acceptance contracts. The Core architecture must retain: machine-reset semantics; runtime-speed semantics; local/Telnet keyboard normalization; screenshot raster provenance; and Telnet socket/framing/connection/threading/keyboard injection. For every shared-registry operation, prove the UI/Telnet layer requests but does not redefine the Core effect.

Then instrument GUI toolkit events, application menus, file choosers, registry discovery/dispatch, Telnet application-control parsing, and host screenshot operations with varied/poisoned host timestamps; none may alter canonical Spectrum event timestamps or become a source of machine time.

**Evidence output:** `certification/results/core/C30.04.json`

**Pass rule:** PASS only when every ownership row has exactly one authority, no UI surface redefines machine semantics, and no UI/host-control operation supplies canonical Spectrum time.

## C31. Architecture section 31: Core threading model

**Architecture authority:** `C` source lines 2608-2627, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Single-owner core threading.

**Required artifacts:**

- `tests/fixtures/concurrency/wzsn-thread-stress-v1.json`
- `tests/fixtures/concurrency/wzsn-cross-thread-queue-contract-v1.json`

**Artifact/source authority:** Project-generated concurrency stress plan and queue-content contract.

**Tests:**

### C31.01

Stress Sokol/audio/network/background worker activity while one emulation thread exclusively owns mutable machine state; use race detector where available.

**Evidence output:** `certification/results/core/C31.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C31.02

Perturb host thread scheduling and compare canonical results; only bounded queues may cross thread boundaries.

**Evidence output:** `certification/results/core/C31.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C31.03

Using `wzsn-cross-thread-queue-contract-v1.json`, inspect every cross-thread producer/consumer path and prove bounded queues contain only the Section-31 permitted classes: completed presentation data, completed audio data, host input events, or host control requests. Mutation cases that pass a mutable `wz_machine_t`, direct core pointer, host callback that mutates machine state, or an unbounded queue across threads must fail.

**Evidence output:** `certification/results/core/C31.03.json`

**Pass rule:** PASS only when one emulation thread remains the exclusive mutable-machine owner and every cross-thread channel is bounded and class-constrained.

## C32. Architecture section 32: Pacing and audio buffering

**Architecture authority:** `C` source lines 2628-2657, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Pacing and audio buffering.

**Required artifacts:**

- `tests/fixtures/pacing/wzsn-pacing-v1.json`
- `tests/fixtures/pacing/wzsn-runloop-order-v1.json`

**Artifact/source authority:** Project-generated pacing scenarios and run-loop instrumentation contract.

**Tests:**

### C32.01

Vary host sleep/yield granularity, audio queue fullness, render cadence, and temporary host stalls while executing fixed normalized input; canonical master-tick/event order remains identical.

**Evidence output:** `certification/results/core/C32.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C32.02

Prove selected runtime speed is not silently adjusted by audio buffer feedback.

**Evidence output:** `certification/results/core/C32.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C32.03

Instrument one normal application run using `wzsn-runloop-order-v1.json` and verify the architectural order at every scheduling slice: collect host input -> normalize/schedule emulated input -> run deterministic core to the next boundary -> produce canonical raster/audio blocks -> submit completed presentation/audio to host -> compare emulated progress to host monotonic time -> sleep/yield only when ahead. Deliberately reorder pacing before core execution in a mutation build and require the gate to fail.

**Evidence output:** `certification/results/core/C32.03.json`

**Pass rule:** PASS only when the scheduler decides machine work before the host decides presentation timing, and audio fullness never changes event order, hardware timing, or selected speed.

## C33. Architecture section 33: Build architecture

**Architecture authority:** `C` source lines 2658-2706, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** CMake build architecture and manifests.

**Required artifacts:**

- `CMakeLists.txt`
- `cmake/toolchains/`
- `certification/manifests/build.json`
- `certification/config/developer-build-configs.json`

**Artifact/source authority:** Project build tree and compiler-specific high-warning/sanitizer configuration manifest.

**Tests:**

### C33.01

Build required logical targets `wz_core`, `wz_headless`, `wz_tests`, and GUI app from root CMake entry point on each mandatory platform.

**Evidence output:** `certification/results/core/C33.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C33.02

Verify build manifest records commit/compiler/OS/CPU/C standard/flags/Sokol/backend/SDK fields and no runtime language environment is required.

**Evidence output:** `certification/results/core/C33.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C33.03

Configure and build every compiler-supported developer profile declared in `developer-build-configs.json`, including the project's high-warning profile and memory/undefined-behavior sanitizers where the selected compiler provides them. Prove these are reachable from the root CMake entry point rather than a separate platform-specific project architecture.

**Evidence output:** `certification/results/core/C33.03.json`

**Pass rule:** PASS only when the Section-33 developer configurations are first-class CMake build modes on applicable compilers.

## C34. Architecture section 34: Initial supported platform matrix

**Architecture authority:** `C` source lines 2707-2738, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Supported platform matrix.

**Required artifacts:**

- `certification/config/platform-matrix.json`
- `certification/manifests/release-artifacts.json`
- `certification/manifests/platform-run-artifacts.json`

**Artifact/source authority:** `platform-matrix.json` freezes claimed target scope; `release-artifacts.json` identifies the exact hashed executable/bundle bytes for each target; `platform-run-artifacts.json` identifies the real-machine/runner evidence bundle for release build, deterministic core, raster, audio, input, media, dependency inspection, and practical launch/run tests.

**Tests:**

### C34.01

For Windows x86-64, Linux x86-64, Linux AArch64, and macOS AArch64, resolve the exact hashed release executable/bundle from `release-artifacts.json` and the corresponding real-host evidence bundle from `platform-run-artifacts.json`. Require successful release build, deterministic core suite, raster tests, audio tests, input tests, media tests, package dependency inspection, and practical launch/run testing on the actual target CPU/OS for those exact bytes. Mere compilation is never sufficient.

**Evidence output:** `certification/results/core/C34.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C34.02

If macOS x86-64 is advertised as maintained, require its exact release artifact and complete real-host evidence bundle in the same manifests and apply the full C34.01 support test. If it is no longer advertised, the platform matrix must explicitly mark it unmaintained/secondary-not-required rather than silently skipping a claimed target.

**Evidence output:** `certification/results/core/C34.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C34.03

Reconcile `platform-matrix.json` against Section 34 and prove Windows ARM64, Android, iOS and WebAssembly are not required initial release gates merely because experimental builds exist. A future platform can be added only by an explicit supported-platform decision with the full release/determinism/raster/audio/input/media/dependency/real-launch obligations; experimental success does not inherit supported status.

**Evidence output:** `certification/results/core/C34.03.json`

**Pass rule:** PASS only when initial supported versus later/experimental platform scope is exact.

## C35. Architecture section 35: Cross-compiler matrix

**Architecture authority:** `C` source lines 2739-2777, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Cross-compiler matrix.

**Required artifacts:**

- `certification/config/compiler-matrix.json`
- `certification/manifests/runner-provenance.json`

**Artifact/source authority:** Project compiler matrix plus runner provenance identifying hosted or project-controlled hardware for every mandatory CPU/compiler group.

**Tests:**

### C35.01

Run required deterministic suite under MSVC+clang-cl, GCC+Clang on Linux x86-64/AArch64, and Apple Clang on macOS AArch64; compare canonical hashes.

**Evidence output:** `certification/results/core/C35.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C35.02

Compiler disagreement blocks release and triggers UB/implementation-defined behavior investigation.

**Evidence output:** `certification/results/core/C35.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C35.03

For every mandatory target/compiler group, verify `runner-provenance.json` identifies an actual executed runner. Simulate hosted-CI unavailability for a required architecture and require the release gate to route to project-controlled/self-hosted hardware rather than mark the group skipped. macOS x86-64 may be omitted only when the project no longer advertises it as maintained.

**Evidence output:** `certification/results/core/C35.03.json`

**Pass rule:** PASS only when runner convenience can never waive a mandatory architecture/compiler execution gate.

## C36. Architecture section 36: Deterministic regression protocol

**Architecture authority:** `C` source lines 2778-2884, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Deterministic regression protocol and mandatory Fuse/private media gates.

**Required artifacts:**

- `external/fuse-1.9.2/z80/tests/tests.in`
- `external/fuse-1.9.2/z80/tests/tests.expected`
- `WZSN-PRIVATE-TEST-MEDIA/`

**Artifact/source authority:** E01 Fuse; private developer-only flat directory is intentionally unstructured and not distributed.

**Tests:**

### C36.01

For each deterministic test record machine model, ROM hash, initial state, media hash where applicable outside the explicit Section-36.2 private-media exception, configuration, normalized input trace, and checkpoint master ticks; compare all defined canonical state domains. Private difficult-media files remain exempt from architecture-mandated hash/manifest metadata.

**Evidence output:** `certification/results/core/C36.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C36.02

Run complete pinned Fuse suite with no unexplained skipped/failing applicable case.

**Evidence output:** `certification/results/core/C36.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C36.03

When the private media directory is present, execute the project-defined private difficult-media regression using whichever directly stored private files its developer tests select; the architecture does not require recursion, a complete inventory, per-file hashes, or one test for every recognized file, and unrecognized files may be ignored. A full difficult-media/hardware-hack compatibility claim requires that this private regression run complete with zero unexplained failures. Public CI may report the private media unavailable/skipped without failing the product.

**Evidence output:** `certification/results/core/C36.03.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## C37. Architecture section 37: Video regression protocol

**Architecture authority:** `C` source lines 2885-2906, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Canonical video regression.

**Required artifacts:**

- `certification/manifests/C37-golden-video.json`
- `external/canary/NIRVANAENGINE.tap`
- `certification/gates/external/E09-nirvana-member.json`
- `external/canary/NIRVANA+ENGINE.tap`
- `certification/gates/external/E10-nirvanaplus-member.json`
- `external/canary/BIFROSTENGINEV1.2L.tzx`
- `external/canary/BIFROSTENGINEV1.2H.tzx`
- `certification/gates/external/E11-bifrost-members.json`
- `external/canary/BIFROST2ENGINE.tap`
- `certification/gates/external/E12-bifrost2-member.json`

**Artifact/source authority:** Project-generated golden video artifacts plus catalog E09-E12 difficult raster canaries. Each canonical local TAP/TZX path is admissible only after its corresponding archive-member identity/hash gate proves the runnable member from the authoritative archive; the World of Spectrum item pages prove archive names but do not by themselves prove internal ZIP member names.

**Tests:**

### C37.01

Using only golden files enumerated by `C37-golden-video.json` (exact relative filename, SHA-256, machine profile/ROM hash, producing test ID, and generator revision), compare full logical raster, active bitmap, border timeline, blanking map, ULA fetch trace, raster hash, and exact critical transitions before Sokol. An unmanifested file in the golden directory cannot become an oracle.

**Evidence output:** `certification/results/core/C37.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C37.02

Classify host post-scaling screenshots only as presentation evidence, never core video oracle.

**Evidence output:** `certification/results/core/C37.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C37.03

Run the E09-E12 NIRVANA/NIRVANA+/BIFROST-family artifacts listed above as deliberately difficult raster software only after the corresponding archive-member identity/hash gate is frozen, and capture canonical logical-raster/border/fetch/event hashes before Sokol. If any required member identity is not yet frozen, this external-canary subtest is BLOCKED_GATE rather than guessing a ZIP member. These canaries supplement, and may not replace, project-owned exact raster/timing tests; visual plausibility alone is not a PASS.

**Evidence output:** `certification/results/core/C37.03.json`

**Pass rule:** PASS only when timing-sensitive video regression includes difficult real software and its result is evaluated at the canonical pre-presentation boundary.

## C38. Architecture section 38: Audio regression protocol

**Architecture authority:** `C` source lines 2907-2924, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Canonical audio regression.

**Required artifacts:**

- `tests/golden/audio/wzsn-audio-v1.pcm`
- `tests/fixtures/audio/wzsn-audio-events-v1.json`

**Artifact/source authority:** Project-generated audio vectors/golden PCM.

**Tests:**

### C38.01

Compare timestamped beeper/AY writes, AY internal state, fixed-point mixer output, canonical PCM hash, and sample accumulator before host conversion.

**Evidence output:** `certification/results/core/C38.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C38.02

Host WASAPI/CoreAudio/ALSA capture may test delivery but cannot replace canonical audio proof.

**Evidence output:** `certification/results/core/C38.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## C39. Architecture section 39: Differential testing against Warajevo 2.50

**Architecture authority:** `C` source lines 2925-2954, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Differential testing against Warajevo 2.50.

**Required artifacts:**

- `certification/manifests/warajevo-source-authority.json`
- `certification/manifests/warajevo-251-252-behavior-evidence.json`
- `certification/manifests/warajevo250-differential-cases.json`

**Artifact/source authority:** Preserved Warajevo 2.50 source/build/reference artifacts.

**Tests:**

### C39.01

For compatible scenarios compare registers, memory, snapshots, tape, MDR, IF1, ordinary screen, debugger semantics, errors/workflows.

**Evidence output:** `certification/results/core/C39.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C39.02

Every mismatch gets a disposition; if preserved Warajevo approximation conflicts with authenticated hardware, WZSN follows hardware and records the intentional difference.

**Evidence output:** `certification/results/core/C39.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C39.03

Reconcile `warajevo-251-252-behavior-evidence.json` with every differential claim that relies on observable Warajevo 2.51/2.52 behavior absent from the preserved 2.50 source. For each used item, record the exact executable/document/sample-media filename, acquisition/source identity, hash where bytes are available, observed behavior, and classification `BEHAVIOR_ONLY`. Prove no migration record, source mapping, or implementation comment treats a 2.51/2.52 behavior observation as if a matching source routine had survived. If no later-version evidence is used for a release, the manifest explicitly records NONE.

**Evidence output:** `certification/results/core/C39.03.json`

**Pass rule:** PASS only when preserved 2.50 source remains the source-level authority and every later-version artifact is traceable secondary behavioral evidence rather than imaginary source code.

## C40. Architecture section 40: Real-hardware validation

**Architecture authority:** `C` source lines 2955-2980, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Real-hardware/reference certification.

**Required artifacts:**

- `certification/hardware/zx48-certified-profile-inventory.json`
- `certification/hardware/zx128-pal-inventory.json`
- `certification/hardware/captures-manifest.json`
- `tests/fixtures/timing/wzsn-contention-48k.tap`
- `tests/fixtures/ula/wzsn-ula-fetch-48k.tap`
- `tests/fixtures/video/wzsn-border-timing-48k.tap`
- `tests/fixtures/input/wzsn-port-fe-timing-48k.tap`
- `tests/fixtures/tape/wzsn-tape-ear-mic-48k.tap`
- `tests/fixtures/audio/wzsn-ay-timing-128k.tap`
- `tests/fixtures/interface1/wzsn-if1-rom-page.tap`

**Artifact/source authority:** Real identified Spectrum hardware and project hardware-evidence records. The listed project-owned TAP diagnostics are the exact preferred on-machine stimuli for the corresponding measurements; independently established hardware-reference evidence may substitute only when its identity/capture is recorded.

**Hardware-capture identity rule:** `certification/hardware/captures-manifest.json` must enumerate every capture/test ROM admitted to certification by exact relative filename, SHA-256, machine/board/ULA/CPU identity, ROM hash, instrument configuration, measured behavior, repetition count, and source/provenance. The capture directory alone is never certification evidence.

**Tests:**

### C40.01

Measure/validate frame and line lengths, interrupt timing, contention boundaries, floating bus, FE I/O, border timing, ULA fetch ordering, cassette waveform behavior, 48K/128K differences, AY timing, and IF1/Microdrive behavior on identified hardware or independently established hardware evidence.

**Evidence output:** `certification/results/core/C40.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C40.02

Every capture records machine/ULA/CPU/ROM identity, instrument setup, test program hash, repetitions, observed variation, and interpretation.

**Evidence output:** `certification/results/core/C40.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C40.03

For every measurement/reference result used to freeze certification behavior, verify the raw capture plus the exact test-program artifact are preserved as project evidence where licensing permits. If licensing prevents preservation of an external test binary, preserve the project's measurement/capture record and an exact acquisition identity/reference without copying the restricted binary into public artifacts. Missing source/capture identity makes the dependent hardware value BLOCKED_GATE.

**Evidence output:** `certification/results/core/C40.03.json`

**Pass rule:** PASS only when hardware/reference evidence used to establish machine behavior remains reproducible and its preservation treatment respects the architecture's licensing qualifier.

## C41. Architecture section 41: Migration method for original Pascal and assembly

**Architecture authority:** `C` source lines 2981-3039, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Legacy migration method.

**Required artifacts:**

- `design/migration-ledger.md`
- `certification/manifests/migration-record-schema-v1.json`
- `certification/manifests/legacy-routine-inventory.json`

**Artifact/source authority:** Preserved Warajevo source plus project migration ledger/schema and a source-derived routine/label inventory.

**Tests:**

### C41.01

Audit every migrated Pascal/ASM routine through classify→behavior extraction→boundary assignment→C implementation→regression evidence.

**Evidence output:** `certification/results/core/C41.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C41.02

No routine may reach implementation without provenance/license/derivation clearance.

**Evidence output:** `certification/results/core/C41.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C41.03

Validate every migration-ledger record against `migration-record-schema-v1.json`; require all Section-41 fields: upstream repository+commit/blob identity, source file, routine/label, purpose, copyright/notice, license classification, derivation clearance (`TRANSLATE`/`REIMPLEMENT`/`RESEARCH`/`EXCLUDE`), inputs/outputs, emulated state read/written, host state read/written, timing consequences, external calls, error behavior, C destination, regression test, provenance comment/reference, and migration status. Reconcile `legacy-routine-inventory.json` so no source file is marked fully migrated until every relevant routine/logically inseparable block is accounted for. Third-party/unclear legacy material must default to REIMPLEMENT or RESEARCH, never TRANSLATE, until clearance changes explicitly.

**Evidence output:** `certification/results/core/C41.03.json`

**Pass rule:** PASS only when the complete routine-by-routine migration workflow and clearance/accounting rules are mechanically auditable with no omitted record field or unaccounted routine.

## C42. Architecture section 42: Historical host mechanism replacement map

**Architecture authority:** `C` source lines 3040-3065, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Historical host-mechanism replacement.

**Required artifacts:**

- `certification/manifests/legacy-host-replacement.json`

**Artifact/source authority:** Project-generated ledger derived from Architecture #1 replacement table.

**Tests:**

### C42.01

For each of the fourteen Section-42 historical mechanism mappings, prove the specified modern destination exists and that the historical host mechanism itself is not retained as emulator architecture unless it represents actual Spectrum behavior. The aggregate test is limited to the literal C42.R01-R14 mappings below and does not add unrelated historical mechanisms not named by Section 42.

**Evidence output:** `certification/results/core/C42.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C42.02

Mutation scan fails if historical host compensation re-enters deterministic machine timing.

**Evidence output:** `certification/results/core/C42.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C42 literal historical-host replacement ledger

| Subtest | Literal architecture mapping | Primary proof test(s) | Required proof artifact(s) | Evidence artifact |
|---|---|---|---|---|
| C42.R01 | Turbo Pascal application      portable C application logic | C03.01 | `CMakeLists.txt`; `certification/config/compiler-matrix.json` | `certification/results/core/C42.R01.json` |
| C42.R02 | Turbo Vision UI               new host/application UI | U01.01; U06.01 | `warajevo-zx-spectrum-next-ui-architecture.md`; `tests/fixtures/ui/U06-layout.json` | `certification/results/core/C42.R02.json` |
| C42.R03 | x86 Z80 register mapping      explicit portable Z80 state | C07.03 | `certification/manifests/machine-storage-types.json`; `tests/fixtures/cpu/wzsn-z80-semantic-v1.json` | `certification/results/core/C42.R03.json` |
| C42.R04 | x86 opcode handlers           portable C Z80 handlers/micro-operations | C11.05 | `certification/manifests/z80-opcode-audit-map.json`; `tests/fixtures/cpu/wzsn-z80-bus-v1.json` | `certification/results/core/C42.R04.json` |
| C42.R05 | DOS segmented memory          explicit flat C data structures | C08.01; C07.03 | `certification/manifests/canonical-state-format-v1.json`; `certification/manifests/machine-storage-types.json` | `certification/results/core/C42.R05.json` |
| C42.R06 | BIOS keyboard                 Sokol host input -> Spectrum matrix | C22.01; C22.02 | `tests/fixtures/input/wzsn-keyboard-matrix-v1.json`; `tests/fixtures/input/wzsn-input-ownership-v1.json` | `certification/results/core/C42.R06.json` |
| C42.R07 | CGA/EGA/VGA/Hercules output   ULA raster -> Sokol gfx | C16.01; C19.01 | `tests/golden/video/wzsn-raster-48k-v1.bin`; `tests/fixtures/video/wzsn-presentation-matrix-v1.json` | `certification/results/core/C42.R07.json` |
| C42.R08 | PC speaker                    emulated beeper -> canonical mixer | C20.01 | `tests/fixtures/audio/wzsn-beeper-events-v1.json` | `certification/results/core/C42.R08.json` |
| C42.R09 | SoundBlaster                  canonical PCM -> Sokol audio | C20.03; C21.01 | `tests/golden/audio/wzsn-audio-v1.pcm`; `tests/fixtures/audio/wzsn-sokol-push-degradation-v1.json` | `certification/results/core/C42.R09.json` |
| C42.R10 | DOS PIT/timer                 Sokol/host monotonic time for pacing only | C10.03; C32.01 | `tests/fixtures/pacing/wzsn-host-clock-poison-v1.json`; `tests/fixtures/pacing/wzsn-pacing-v1.json` | `certification/results/core/C42.R10.json` |
| C42.R11 | DOS file I/O                  portable C file/media layer | C23.01; C25.02; C46.02 | `tests/fixtures/tape/wzsn-tap-standard-oneblock.tap`; `tests/fixtures/snapshot/wzsn-sna-malformed-truncated.sna`; `tests/fixtures/faults/host-io-v1.json` | `certification/results/core/C42.R11.json` |
| C42.R12 | LPT host output               host abstraction where retained | U16.01; U16.02 | `tests/fixtures/printer/wzsn-zxprinter-smoke.tap`; `tests/fixtures/ui/U16-printer-manager.json` | `certification/results/core/C42.R12.json` |
| C42.R13 | CLI/STI host critical code    modern synchronization where actually needed | C31.01; C31.02 | `tests/fixtures/concurrency/wzsn-thread-stress-v1.json`; `tests/fixtures/concurrency/wzsn-cross-thread-queue-contract-v1.json` | `certification/results/core/C42.R13.json` |
| C42.R14 | Warajevo raster hacks         accurate bus/ULA timing model | C18.01; C43.02 | `tests/fixtures/video/wzsn-raster-race.tap`; `tests/fixtures/performance/wzsn-optimization-policy-v1.json` | `certification/results/core/C42.R14.json` |

## C43. Architecture section 43: Performance policy

**Architecture authority:** `C` source lines 3066-3092, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Performance changes only after correctness.

**Required artifacts:**

- `tests/fixtures/performance/wzsn-optimization-policy-v1.json`
- `certification/manifests/C43-optimization-cases.json`

**Artifact/source authority:** Project optimization-policy vectors plus `C43-optimization-cases.json`, which pins each optimization case by exact pre/post source revision/build identity, machine profile/ROM, deterministic input fixture, checkpoint set, and lower-level regression gates. The before/after result is generated by this test; it is not a prerequisite allowed to certify itself.

**Tests:**

### C43.01

For every optimization case enumerated in `C43-optimization-cases.json`, run the pinned pre-optimization and post-optimization builds with identical machine profile, ROM, configuration, inputs, and canonical checkpoints. Compare every applicable canonical state/raster/audio/media hash and require exact equality. Generate `certification/results/performance/before-after.json` from those paired executions, including exact build/source identities and checkpoint hashes; no pre-existing before/after result may be consumed as input.

**Evidence outputs:** `certification/results/performance/before-after.json`; `certification/results/core/C43.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C43.02

Reject timing-table collapse, event skipping, host-time shortcuts, or program-specific fast paths that change observable behavior.

**Evidence output:** `certification/results/core/C43.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C43.03

Execute every row in `wzsn-optimization-policy-v1.json`. Demonstrate behaviorally equivalent examples of each permitted class (safe event batching, exhaustively verified flag table, profile-derived contention table, generated opcode decode table, contiguous layout, presentation-frame dropping, host-only SIMD color conversion). Independently mutation-test every forbidden class: instruction-end-delayed memory writes, collapsed timed I/O writes, final-border-only rendering, ULA-fetch skipping that breaks floating bus, AY advanced from host audio callback timing, and host-specific arithmetic changing flags/overflow. Each forbidden mutation must be detected by a targeted lower-level gate.

**Evidence output:** `certification/results/core/C43.03.json`

**Pass rule:** PASS only when every optimization class named by Section 43 has explicit evidence and no performance change weakens correctness.

## C44. Architecture section 44: Debugging and observability

**Architecture authority:** `C` source lines 3093-3228, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** 16 MiB circular timing trace and observability.

**Required artifacts:**

- `tests/fixtures/trace/wzsn-trace-synthetic-worstcase.bin`
- `tests/fixtures/trace/wzsn-trace-real-48k-stress.tap`
- `tools/wz_trace_dump`
- `tests/fixtures/trace/wzsn-trace-header-contract-v1.json`
- `tests/fixtures/trace/wzsn-trace-event-schema-v1.json`
- `tests/fixtures/trace/wzsn-trace-service-policy-v1.json`

**Artifact/source authority:** Project-generated trace fixtures, exact header/event/service contracts, and standalone dump tool.

**Tests:**

### C44.01

Verify one no-clobber trace per process, total file size <=16,777,216 bytes including header, versioned header/records, wrap generation, periodic absolute sync, canonical master ticks, same-tick order, and TIMING_FULL event depth.

**Evidence output:** `certification/results/core/C44.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C44.02

Under the real Phase-2 48K timing stress fixture, freeze after wrap and prove at least eight complete preceding frames are recoverable.

**Evidence output:** `certification/results/core/C44.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C44.03

Compare trace off/on/wrapping/frozen/output-failure workloads; canonical machine hashes/order must be identical.

**Evidence output:** `certification/results/core/C44.03.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C44.04

Kill process mid-record and prove standalone reader recovers complete records and rejects incomplete trailing data.

**Evidence output:** `certification/results/core/C44.04.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C44.05

Using `wzsn-trace-header-contract-v1.json`, verify every required self-description field: format version, record-area bounds, write position/wrap generation, per-process/session identity, start machine-profile and ROM identity, feature/event mask, and first/last recoverable sequence. Verify exclusive filenames distinguish concurrent instances and the chosen path is reported in developer startup diagnostics. Allow host wall-clock/PID/session/path only in header/filename convenience fields; inject them into event timestamps in a mutation build and require rejection because event timestamps must be canonical master ticks. Validate explicit record lengths/wrap boundaries prevent partial records from being interpreted as complete and periodic absolute sync permits decoding a surviving wrapped segment without overwritten history.

**Evidence output:** `certification/results/core/C44.05.json`

**Pass rule:** PASS only when the complete Sections-44.1/44.2 trace-format/session/timestamp contract is self-describing, no-clobber, wrap-decodable, and host-time-free at event level.

### C44.06

Reconcile every `TIMING_FULL` record type against `wzsn-trace-event-schema-v1.json` and emit at least one decodable instance of each architecture-required event class: global master tick; same-tick event sequence/order; derived CPU T-state/phase; instruction boundary with PC/opcode/prefix/register snapshot; CPU M-cycle/externally visible bus phase; memory read/write address+value; I/O read/write address+value; contention request+inserted delay; INT/NMI edge/sample/acceptance/acknowledge; ULA fetch address/value/kind; raster/frame position; floating-bus source/value; border change; beeper change; AY register write; tape EAR/MIC edge; Interface-1/Microdrive/ZX-Net event; networking-mode/cold-reconfiguration marker; and explicit developer/test marker.

**Evidence output:** `certification/results/core/C44.06.json`

**Pass rule:** PASS only when every listed architecture event is represented with canonical timing/order sufficient to reconstruct causal timing failures; omission or an ambiguous event encoding is FAIL.

### C44.07

Using `wzsn-trace-service-policy-v1.json`, prove tracing is optional and may be disabled by default in release use while every running developer/debug WZSN process can own an independent trace when enabled. The selected trace path/PID/session/wall-clock convenience metadata must remain host diagnostic state and never enter canonical hashes/snapshots. Force trace-file create failure and mid-run write failure: a developer build must report the diagnostic failure visibly, continue emulating without machine mutation/termination caused solely by lost trace storage, and preserve the same canonical result as trace-disabled execution.

**Evidence output:** `certification/results/core/C44.07.json`

**Pass rule:** PASS only when trace service availability/reporting is host-only and diagnostic storage failure cannot become a Spectrum failure.

## C45. Architecture section 45: Headless core

**Architecture authority:** `C` source lines 3229-3252, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Headless core.

**Required artifacts:**

- `tests/fixtures/headless/wzsn-headless-smoke-v1.json`
- `tests/fixtures/headless/wzsn-headless-workload-matrix-v1.json`

**Artifact/source authority:** Project-generated headless smoke and required-workload matrix.

**Tests:**

### C45.01

Build/run deterministic machine tests and replay without Sokol, GUI, Telnet, windowing, or audio device.

**Evidence output:** `certification/results/core/C45.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C45.02

Compare headless canonical hashes with GUI run for same machine state/input.

**Evidence output:** `certification/results/core/C45.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C45.03

Execute each Section-45 required use case headlessly from `wzsn-headless-workload-matrix-v1.json`: unit tests, CI regression, fuzzing, differential CPU tests, snapshot tests, media parser tests, deterministic replay, state hashing, and timing-trace generation. Link/run with no window, GPU, live keyboard or audio device and no Sokol dependency in the deterministic core/headless target.

**Evidence output:** `certification/results/core/C45.03.json`

**Pass rule:** PASS only when every enumerated headless use case functions without presentation devices and remains a direct proof that Sokol has not leaked into the machine model.

## C46. Architecture section 46: Failure and error handling

**Architecture authority:** `C` source lines 3253-3274, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Failure and error handling.

**Required artifacts:**

- `certification/manifests/media-fuzz-seeds.json`
- `tests/fixtures/faults/host-io-v1.json`
- `tests/fixtures/faults/wzsn-external-input-vs-assert-v1.json`
- `tests/fixtures/faults/wzsn-host-device-failure-v1.json`

**Artifact/source authority:** Project-generated malformed/fault seeds and host-device failure scenarios.

**Tests:**

### C46.01

Fuzz/truncate/corrupt every media/parser format at structural boundaries and inject allocation/read/write/rename/full-disk failures; live machine state remains unchanged before commit.

**Evidence output:** `certification/results/core/C46.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C46.02

Host-feature failures (trace, screenshot, GUI output) are controlled and cannot corrupt deterministic machine state.

**Evidence output:** `certification/results/core/C46.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C46.03

Using `wzsn-external-input-vs-assert-v1.json`, prove malformed external media/snapshot data returns controlled deterministic errors after length/integer-overflow/allocation/format checks and never relies on a development assertion for user-controlled invalid input; reserve assertions for internal invariants. Using `wzsn-host-device-failure-v1.json`, fail GPU-device and audio-device creation and prove the result is an application/host failure with no invented Spectrum state transition or partial core mutation.

**Evidence output:** `certification/results/core/C46.03.json`

**Pass rule:** PASS only when external malformed input is memory-safe/controlled and host-service failure is never misrepresented as machine state.

## C47. Architecture section 47: Dependency policy

**Architecture authority:** `C` source lines 3275-3303, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Third-party dependency policy.

**Required artifacts:**

- `certification/manifests/dependencies.json`
- `certification/manifests/dependency-approval-schema-v1.json`

**Artifact/source authority:** Project dependency manifest/approval schema and binary inspection.

**Tests:**

### C47.01

Verify initial release third-party runtime scope matches architecture (Sokol plus explicitly approved incorporated code; Telnet uses native sockets).

**Evidence output:** `certification/results/core/C47.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C47.02

Fail release if an unapproved runtime dependency appears or if dependency convenience changes core behavior.

**Evidence output:** `certification/results/core/C47.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C47.03

Validate every dependency beyond baseline Sokol against `dependency-approval-schema-v1.json`: documented insufficiency of standard/existing code, static incorporation plan, supported-platform coverage, license compatibility, maintenance burden, and deterministic-core isolation are all mandatory before architectural approval. A mutation proposal justified only by convenience must be rejected. Confirm Special Feature 1 continues to use project-owned wrappers over native OS sockets and therefore adds no third-party runtime dependency.

**Evidence output:** `certification/results/core/C47.03.json`

**Pass rule:** PASS only when additional third-party code cannot enter the release without every Section-47 justification and explicit architecture approval.

## C48. Architecture section 48: Version-pinning and provenance

**Architecture authority:** `C` source lines 3304-3332, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Version pinning and provenance.

**Required artifacts:**

- `certification/manifests/dependencies.json`
- `design/migration-ledger.md`
- `LICENSE.txt`
- `NOTICE.md`

**Artifact/source authority:** Project provenance records.

**Tests:**

### C48.01

Every third-party source dependency has exact revision/release, origin, license, and local modifications recorded; no release tracks a moving branch.

**Evidence output:** `certification/results/core/C48.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C48.02

Every migrated Warajevo routine has upstream repository/commit/blob, notice/license classification, derivation type/clearance, C destination, and audit mapping.

**Evidence output:** `certification/results/core/C48.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C48.03

Reconcile `LICENSE.txt` and `NOTICE.md` against `dependencies.json`, all bundled third-party notices/licenses, and every completed migration-ledger clearance. Verify required attribution/license notices are present in new source/release materials and no bundled material has a provenance/license classification inconsistent with the top-level files.

**Evidence output:** `certification/results/core/C48.03.json`

**Pass rule:** PASS only when release licensing/notice material and provenance records are mutually consistent and complete.

## C49. Architecture section 49: Initial implementation sequence

**Architecture authority:** `C` source lines 3333-3538, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Implementation phase sequence, literal Phase-0..16 exit gates, and mandatory implementation-ticket derivation fields.

**Required artifacts:**

- `wzsn-architectures-1-2-developer-tasks.md`
- `certification/manifests/core-phase-gate-inputs.json`
- `certification/manifests/core-ticket-derivation-v1.json`

**Artifact/source authority:** The canonical developer backlog provides ordered tickets. `core-phase-gate-inputs.json` is generated from the literal C49 phase ledger and records the exact lower-level artifact paths required by every Phase-0..16 gate; `phase-gates.json` records their resulting status and is never a substitute for those inputs. `core-ticket-derivation-v1.json` validates every implementation/research ticket against the literal Section-49.2 field contract.

**Tests:**

### C49.01

Generate `certification/manifests/core-phase-gate-inputs.json` from the literal C49 phase ledger and verify it contains the exact lower-level artifact paths for every Phase-0..16 exit gate with no missing or extra entry. Verify tickets execute Phase 0->16 ordering and each Section-49.1 minimum exit gate has PASS evidence from those exact inputs before dependent implementation is treated as dependable. Generate `certification/results/phase-gates.json` from this evaluation, recording every phase, required input, PASS/BLOCKED/FAIL state, and evidence source. A pre-existing phase-gate result JSON is never an input and cannot certify itself.

**Evidence outputs:** `certification/results/phase-gates.json`; `certification/results/core/C49.01.json`

### C49.02

Verify every implementation ticket contains all Section-49.2 required fields. Unresolved behavioral constants/format rules/hardware facts create research/specification tickets that update the companion design document before dependent implementation tickets open; no implementation ticket may choose an unresolved value merely because it compiles.

**Evidence output:** `certification/results/core/C49.02.json`

**Pass rule:** PASS only when phase ordering, all literal exit gates, and every ticket-field requirement are exact and reproducible. Any unexplained mismatch is FAIL; an architecture-deferred choice remains BLOCKED_GATE until frozen.

### C49.1 literal phase-exit gate ledger

| Subtest | Phase | Literal minimum exit gate | Required proof artifact(s) | Evidence artifact |
|---|---|---|---|---|
| C49.G0 | 0 | timing evidence files frozen; ROM hashes recorded; source/provenance rules frozen; test strategy and early smoke corpus review-approved | `design/machine-timing-evidence.md`; `certification/manifests/warajevo-source-authority.json`; `certification/manifests/roms.json`; `tests/fixtures/timing/zx48-pal-timing-v1.json`; `external/fuse-1.9.2/z80/tests/tests.in` | `certification/results/core/C49.G0.json` |
| C49.G1 | 1 | canonical state/serialization/hash round-trips; C11 UB/sanitizer smoke tests pass on at least two materially different compilers; per-process 16 MiB circular trace file, wrap/freeze/recovery, and trace-on/off equivalence tests pass before Phase-2 implementation begins | `tests/fixtures/state/wzsn-state-48k-v1.bin`; `certification/config/sanitizers.json`; `tests/fixtures/trace/wzsn-trace-synthetic-worstcase.bin`; `tests/fixtures/trace/wzsn-trace-header-contract-v1.json`; `tools/wz_trace_dump` | `certification/results/core/C49.G1.json` |
| C49.G2 | 2 | complete Fuse Z80 suite passes; project opcode/flag/interrupt tests pass; exact bus-cycle trace capability demonstrated; TIMING_FULL retention measures at least eight complete 48K frames in the fixed 16 MiB ring | `external/fuse-1.9.2/z80/tests/tests.in`; `external/fuse-1.9.2/z80/tests/tests.expected`; `tests/fixtures/cpu/wzsn-z80-semantic-v1.json`; `tests/fixtures/cpu/wzsn-z80-bus-v1.json`; `tests/fixtures/trace/wzsn-trace-real-48k-stress.tap` | `certification/results/core/C49.G2.json` |
| C49.G3 | 3 | memory/I/O/contention/interrupt tests pass against frozen evidence tables | `tests/fixtures/bus/wzsn-bus-readwrite-v1.json`; `tests/fixtures/timing/wzsn-contention-48k.tap`; `tests/fixtures/timing/zx48-pal-timing-v1.json`; `tests/fixtures/timing/wzsn-same-edge-v1.json` | `certification/results/core/C49.G3.json` |
| C49.G4 | 4 | 48K raster/fetch/border/floating-bus unit tests pass | `tests/fixtures/ula/wzsn-ula-fetch-48k.tap`; `tests/fixtures/ula/wzsn-floating-bus-48k.tap`; `tests/fixtures/video/wzsn-raster-pattern-48k.tap`; `tests/golden/video/wzsn-raster-48k-v1.bin` | `certification/results/core/C49.G4.json` |
| C49.G4A | 4A | early timing-smoke corpus passes with no unexplained trace divergence | `tests/fixtures/video/wzsn-raster-race.tap`; `external/ramsoft/floatspy.tap`; `certification/gates/external/E06-floatspy-identity.json`; `external/softspectrum/timing_tests-48k_v1.0.tzx`; `tests/fixtures/timing/wzsn-same-edge-v1.json` | `certification/results/core/C49.G4A.json` |
| C49.G5 | 5 | headless hashes unchanged by Sokol host; keyboard/Kempston/pacing/ presentation smoke tests pass on at least Windows and one Unix-like host | `certification/manifests/build.json`; `certification/manifests/release-artifacts.json`; `certification/manifests/platform-run-artifacts.json`; `tests/fixtures/input/wzsn-keyboard-matrix-v1.json`; `tests/fixtures/input/wzsn-kempston-port-v1.json`; `tests/fixtures/pacing/wzsn-pacing-v1.json`; `tests/fixtures/video/wzsn-presentation-matrix-v1.json` | `certification/results/core/C49.G5.json` |
| C49.G6 | 6 | deterministic beeper/mixer tests pass; host audio cannot alter core hash | `tests/fixtures/audio/wzsn-beeper-events-v1.json`; `tests/fixtures/audio/wzsn-mixer-vectors-q16_16-v1.json`; `tests/fixtures/audio/wzsn-sokol-push-degradation-v1.json` | `certification/results/core/C49.G6.json` |
| C49.G7 | 7 | media-format-support tape matrix frozen; Normal and trap-equivalence tape tests pass; malformed media is memory-safe | `design/media-format-support.md`; `tests/fixtures/tape/wzsn-tap-standard-oneblock.tap`; `tests/fixtures/tzx/wzsn-tzx-turbo.tzx`; `tests/fixtures/tape/wzsn-trap-rom-standard.tap`; `tests/fixtures/tzx/wzsn-trap-custom-loader.tzx` | `certification/results/core/C49.G7.json` |
| C49.G8 | 8 | SNA/Z80 byte-level matrix frozen; atomic load/save tests pass across hosts; native state records networking_mode without illegal IF1/Ear+Mic combinations | `design/media-format-support.md`; `tests/fixtures/snapshot/wzsn-native-state-v1.json`; `tests/fixtures/snapshot/wzsn-sna-48k-canonical.sna`; `tests/fixtures/snapshot/wzsn-sna-128k-canonical.sna`; `tests/fixtures/snapshot/wzsn-z80-v2-48k.z80`; `tests/fixtures/snapshot/wzsn-z80-v2-128k.z80`; `tests/fixtures/snapshot/wzsn-sna-malformed-truncated.sna`; `tests/fixtures/snapshot/wzsn-z80-malformed-rle.z80` | `certification/results/core/C49.G8.json` |
| C49.G9 | 9 | 128K paging/ULA/AY profile passes deterministic and timing tests | `tests/fixtures/timing/zx128-pal-timing-v1.json`; `tests/fixtures/ula/wzsn-ula-fetch-128k.tap`; `tests/fixtures/audio/wzsn-ay-register-events-v1.json`; `tests/fixtures/snapshot/wzsn-sna-128k-canonical.sna` | `certification/results/core/C49.G9.json` |
| C49.G10 | 10 | Interface1/Microdrive/ZXNet companion spec frozen; NONE/INTERFACE1 arbitration and cold-reconfiguration tests pass; deterministic device/MDR tests and round-trips pass | `design/interface1-microdrive-zxnet.md`; `certification/manifests/interface1-phase10-freeze-ledger.json`; `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/networking/wzsn-networking-transition-edge-v1.json`; `tests/fixtures/mdr/wzsn-mdr-dirty-transition-v1.json`; `tests/fixtures/interface1/wzsn-if1-rom-page.tap`; `tests/fixtures/mdr/wzsn-mdr-minimal.mdr`; `tests/fixtures/interface1/wzsn-if1-zxnet-v1.tap`; `tests/fixtures/zxnet/wzsn-zxnet-loopback-v1.json` | `certification/results/core/C49.G10.json` |
| C49.G11 | 11 | monitor/debugger and ZX Printer required workflows have regression tests | `tests/fixtures/debugger/wzsn-debugger-state-v1.json`; `tests/fixtures/ui/U21-debugger.json`; `tests/fixtures/printer/wzsn-zxprinter-smoke.tap`; `tests/fixtures/ui/U16-printer-manager.json` | `certification/results/core/C49.G11.json` |
| C49.G12 | 12 | UI companion command-registry/menu/workflow/cancel/error/accessibility acceptance passes without core divergence | `certification/manifests/ui-section48-acceptance.json`; `certification/gates/ui-phase12-toolkit.md`; `certification/gates/ui-phase12-accessibility.md`; `tests/fixtures/ui/U43-keyboard-workflows.json`; `tests/fixtures/ui/U46-equivalence.json` | `certification/results/core/C49.G12.json` |
| C49.G13 | 13 | release binaries pass dependency inspection and launch tests per platform | `certification/scripts/inspect-dependencies`; `certification/config/platform-matrix.json`; `certification/manifests/build.json`; `certification/manifests/release-artifacts.json`; `certification/manifests/platform-run-artifacts.json` | `certification/results/core/C49.G13.json` |
| C49.G14 | 14 | full timing torture suite and private difficult-media regression run pass | `tests/fixtures/video/wzsn-raster-race.tap`; `tests/fixtures/video/wzsn-raster-technique-matrix-v1.json`; `tests/fixtures/video/wzsn-warajevo-multicolor-overscan-regression.tap`; `certification/manifests/C18-warajevo-raster-provenance.json`; `external/canary/NIRVANAENGINE.tap`; `certification/gates/external/E09-nirvana-member.json`; `external/canary/NIRVANA+ENGINE.tap`; `certification/gates/external/E10-nirvanaplus-member.json`; `external/canary/BIFROSTENGINEV1.2L.tzx`; `external/canary/BIFROSTENGINEV1.2H.tzx`; `certification/gates/external/E11-bifrost-members.json`; `external/canary/BIFROST2ENGINE.tap`; `certification/gates/external/E12-bifrost2-member.json`; `WZSN-PRIVATE-TEST-MEDIA/` | `certification/results/core/C49.G14.json` |
| C49.G15 | 15 | complete Section 55 transport/keyboard acceptance and UI companion Telnet control/permission acceptance contracts pass, including concurrent process first-free Control Port allocation and exhaustion behavior | `tests/fixtures/telnet/wzsn-telnet-negotiation-v1.bin`; `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/telnet/wzsn-control-port-race-v1.json`; `certification/manifests/ui-section48-acceptance.json` | `certification/results/core/C49.G15.json` |
| C49.G16 | 16 | every optimization has before/after canonical regression evidence | `tests/fixtures/performance/wzsn-optimization-policy-v1.json`; `certification/manifests/C43-optimization-cases.json`; `certification/results/performance/before-after.json`; `certification/results/core/C43.01.json` | `certification/results/core/C49.G16.json` |

### C49.2 implementation-ticket field ledger

| Subtest | Literal required ticket field | Required proof artifact(s) | Evidence artifact |
|---|---|---|---|
| C49.T01 | architecture section(s) and implementation phase | `wzsn-architectures-1-2-developer-tasks.md`; `certification/manifests/core-ticket-derivation-v1.json` | `certification/results/core/C49.T01.json` |
| C49.T02 | exact machine profile(s) affected | `wzsn-architectures-1-2-developer-tasks.md`; `certification/manifests/core-ticket-derivation-v1.json` | `certification/results/core/C49.T02.json` |
| C49.T03 | source/migration-ledger record if legacy behavior is involved | `wzsn-architectures-1-2-developer-tasks.md`; `certification/manifests/core-ticket-derivation-v1.json` | `certification/results/core/C49.T03.json` |
| C49.T04 | license/provenance clearance when legacy or third-party material is referenced | `wzsn-architectures-1-2-developer-tasks.md`; `certification/manifests/core-ticket-derivation-v1.json` | `certification/results/core/C49.T04.json` |
| C49.T05 | explicit dependencies and prior phase gates | `wzsn-architectures-1-2-developer-tasks.md`; `certification/manifests/core-ticket-derivation-v1.json` | `certification/results/core/C49.T05.json` |
| C49.T06 | inputs/state owned and outputs/state produced | `wzsn-architectures-1-2-developer-tasks.md`; `certification/manifests/core-ticket-derivation-v1.json` | `certification/results/core/C49.T06.json` |
| C49.T07 | forbidden dependency/boundary notes relevant to the ticket | `wzsn-architectures-1-2-developer-tasks.md`; `certification/manifests/core-ticket-derivation-v1.json` | `certification/results/core/C49.T07.json` |
| C49.T08 | public/synthetic fixture identities or other test inputs appropriate to the ticket | `wzsn-architectures-1-2-developer-tasks.md`; `certification/manifests/core-ticket-derivation-v1.json` | `certification/results/core/C49.T08.json` |
| C49.T09 | positive acceptance tests | `wzsn-architectures-1-2-developer-tasks.md`; `certification/manifests/core-ticket-derivation-v1.json` | `certification/results/core/C49.T09.json` |
| C49.T10 | negative/error-path acceptance tests | `wzsn-architectures-1-2-developer-tasks.md`; `certification/manifests/core-ticket-derivation-v1.json` | `certification/results/core/C49.T10.json` |
| C49.T11 | determinism/cross-host acceptance where applicable | `wzsn-architectures-1-2-developer-tasks.md`; `certification/manifests/core-ticket-derivation-v1.json` | `certification/results/core/C49.T11.json` |
| C49.T12 | trace/hash evidence required for completion | `wzsn-architectures-1-2-developer-tasks.md`; `certification/manifests/core-ticket-derivation-v1.json` | `certification/results/core/C49.T12.json` |

Before Phase 11 implementation tickets are issued, `design/migration-ledger.md` must additionally contain the complete function-level monitor/debugger and ZX Printer inventory required by Section 49.2, with each historical function classified and linked to regression strategy.

## C50. Architecture section 50: Acceptance criteria

**Architecture authority:** `C` source lines 3539-3678, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Core architecture acceptance criteria 1-75 with explicit lower-level proof and artifact traceability.

**Required artifacts:**

- `certification/manifests/core-section50-acceptance.json`

**Artifact/source authority:** Project-generated acceptance manifest plus the exact lower-level proof inputs named per criterion below. Result JSONs are evidence outputs, not substitutes for their proof artifacts.

**Tests:**

### C50.01

Execute one explicit acceptance record for each of the 75 numbered criteria in Architecture #1 Section 50. Each row below names the primary lower-level test(s), concrete fixture/gate/measurement/build artifact(s), and result evidence. No criterion may PASS solely because another aggregate gate passed, and criteria 1 through 75 must appear exactly once with no gaps.

**Evidence output:** `certification/results/core/C50.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified. Cross-architecture criteria may depend on later UI/Architecture-#3 evidence explicitly named in their rows and cannot be finalized until that evidence exists.

### C50 acceptance-item forensic ledger

| Test | Source line | Literal acceptance criterion | Primary proof test(s) | Required proof artifact(s) | Evidence artifact |
|---|---:|---|---|---|---|
| C50.AC01 | 3544 | the emulation core is portable C; | C03.01; C07.01 | `CMakeLists.txt`; `certification/manifests/machine-storage-types.json` | `certification/results/core/C50.AC01.json` |
| C50.AC02 | 3545 | the core runs headlessly without Sokol; | C45.01; C45.03 | `tests/fixtures/headless/wzsn-headless-smoke-v1.json`; `tests/fixtures/headless/wzsn-headless-workload-matrix-v1.json` | `certification/results/core/C50.AC02.json` |
| C50.AC03 | 3546 | the deterministic core has no host/Sokol/socket dependency; | C05.01; C45.03 | `certification/manifests/allowed-dependencies.json`; `tests/fixtures/boundaries/wzsn-core-orchestrator-contract-v1.json` | `certification/results/core/C50.AC03.json` |
| C50.AC04 | 3547 | application/orchestration code bridges core output/input to host services; | C05.02; C26.01 | `tests/fixtures/boundaries/wzsn-core-orchestrator-contract-v1.json`; `tests/fixtures/app/wzsn-command-sequence-v1.json` | `certification/results/core/C50.AC04.json` |
| C50.AC05 | 3548 | Sokol code is compiled into release program binaries; | C27.01; C28.01 | `third_party/sokol/`; `certification/manifests/build.json`; `certification/manifests/release-artifacts.json`; `certification/scripts/inspect-dependencies` | `certification/results/core/C50.AC05.json` |
| C50.AC06 | 3549 | there is no project-supplied multimedia shared library required at runtime; | C28.01 | `certification/manifests/release-artifacts.json`; `certification/scripts/inspect-dependencies` | `certification/results/core/C50.AC06.json` |
| C50.AC07 | 3550 | supported platform builds use the same deterministic core; | C34.01; C35.01 | `certification/config/platform-matrix.json`; `certification/config/compiler-matrix.json`; `certification/manifests/release-artifacts.json`; `certification/manifests/platform-run-artifacts.json` | `certification/results/core/C50.AC07.json` |
| C50.AC08 | 3551 | canonical emulated time is an integer model-specific master tick; | C10.01 | `tests/fixtures/timing/wzsn-master-time-derivation-v1.json`; `design/machine-timing-evidence.md` | `certification/results/core/C50.AC08.json` |
| C50.AC09 | 3552 | CPU T-state/phase is derived from master time rather than defining universal machine time; | C10.02 | `tests/fixtures/timing/wzsn-master-time-derivation-v1.json` | `certification/results/core/C50.AC09.json` |
| C50.AC10 | 3554 | deterministic state hashes match across supported CPU architectures; | C06.01; C35.01 | `tests/fixtures/traces/wzsn-determinism-input-v1.json`; `certification/config/compiler-matrix.json` | `certification/results/core/C50.AC10.json` |
| C50.AC11 | 3555 | deterministic state hashes match across supported compilers; | C06.01; C35.01 | `tests/fixtures/traces/wzsn-determinism-input-v1.json`; `certification/config/compiler-matrix.json` | `certification/results/core/C50.AC11.json` |
| C50.AC12 | 3556 | Z80 memory and I/O operations can occur at exact intra-instruction master ticks; | C11.02; C12.01 | `tests/fixtures/cpu/wzsn-z80-bus-v1.json`; `tests/fixtures/bus/wzsn-bus-readwrite-v1.json` | `certification/results/core/C50.AC12.json` |
| C50.AC13 | 3558 | contention is applied by the bus/machine timing model; | C13.01 | `tests/fixtures/timing/wzsn-contention-48k.tap`; `tests/fixtures/timing/wzsn-contention-128k.tap` | `certification/results/core/C50.AC13.json` |
| C50.AC14 | 3559 | same-master-edge hardware ordering is explicit and evidence-based; | C14.01; C14.02 | `tests/fixtures/timing/wzsn-same-edge-v1.json` | `certification/results/core/C50.AC14.json` |
| C50.AC15 | 3560 | the ULA fetches memory on an explicit raster schedule; | C15.01 | `tests/fixtures/ula/wzsn-ula-fetch-48k.tap`; `tests/fixtures/ula/wzsn-ula-fetch-128k.tap` | `certification/results/core/C50.AC15.json` |
| C50.AC16 | 3561 | CPU writes become visible according to real bus ordering; | C14.02; C15.01 | `tests/fixtures/timing/wzsn-same-edge-v1.json`; `tests/fixtures/ula/wzsn-ula-fetch-48k.tap`; `tests/fixtures/ula/wzsn-ula-fetch-128k.tap` | `certification/results/core/C50.AC16.json` |
| C50.AC17 | 3562 | full border/raster timing is represented; | C15.01; C16.01 | `tests/fixtures/video/wzsn-raster-pattern-48k.tap`; `tests/fixtures/ula/wzsn-ula-fetch-48k.tap`; `tests/golden/video/wzsn-raster-48k-v1.bin` | `certification/results/core/C50.AC17.json` |
| C50.AC18 | 3563 | the 256 x 192 bitmap is not treated as the entire display; | C16.01 | `tests/fixtures/video/wzsn-raster-pattern-48k.tap`; `tests/golden/video/wzsn-raster-48k-v1.bin` | `certification/results/core/C50.AC18.json` |
| C50.AC19 | 3564 | FLASH and BRIGHT are emulated as Spectrum semantics; | C17.01 | `tests/fixtures/video/wzsn-color-flash-bright.tap`; `tests/golden/video/wzsn-color-flash-bright-v1.bin` | `certification/results/core/C50.AC19.json` |
| C50.AC20 | 3565 | floating-bus behavior is derived from the timed ULA model where supported; | C15.01; C15.02 | `tests/fixtures/ula/wzsn-floating-bus-48k.tap`; `tests/fixtures/ula/wzsn-ula-fetch-48k.tap`; `external/ramsoft/floatspy.tap`; `certification/gates/external/E06-floatspy-identity.json` | `certification/results/core/C50.AC20.json` |
| C50.AC21 | 3566 | overscan/rainbow/multicolor effects do not require application-specific hacks; | C15.05; C18.01; C18.03 | `tests/fixtures/video/wzsn-raster-race.tap`; `tests/fixtures/video/wzsn-raster-race-renamed.tap`; `certification/manifests/C15-no-program-specific-raster-hacks-source-audit.json`; `tests/fixtures/video/wzsn-raster-technique-matrix-v1.json`; `tests/fixtures/video/wzsn-warajevo-multicolor-overscan-regression.tap`; `certification/manifests/C18-warajevo-raster-provenance.json` | `certification/results/core/C50.AC21.json` |
| C50.AC22 | 3568 | NIRVANA/NIRVANA+ class timing can be represented by the core; | C18.02 | `external/canary/NIRVANAENGINE.tap`; `certification/gates/external/E09-nirvana-member.json`; `external/canary/NIRVANA+ENGINE.tap`; `certification/gates/external/E10-nirvanaplus-member.json` | `certification/results/core/C50.AC22.json` |
| C50.AC23 | 3569 | BIFROST-class timing can be represented by the core; | C18.02 | `external/canary/BIFROSTENGINEV1.2L.tzx`; `external/canary/BIFROSTENGINEV1.2H.tzx`; `certification/gates/external/E11-bifrost-members.json`; `external/canary/BIFROST2ENGINE.tap`; `certification/gates/external/E12-bifrost2-member.json` | `certification/results/core/C50.AC23.json` |
| C50.AC24 | 3570 | beeper transitions are master-timestamped; | C20.01 | `tests/fixtures/audio/wzsn-beeper-events-v1.json` | `certification/results/core/C50.AC24.json` |
| C50.AC25 | 3571 | AY state advances from emulated master time; | C20.04 | `tests/fixtures/audio/wzsn-ay-register-events-v1.json`; `tests/fixtures/audio/wzsn-ay-channel-mixer-v1.json`; `src/audio/wz_ay_mixer_policy.h`; `tests/fixtures/timing/zx128-pal-timing-v1.json` | `certification/results/core/C50.AC25.json` |
| C50.AC26 | 3572 | canonical audio is deterministic before Sokol conversion; | C20.01; C20.02; C38.01 | `tests/fixtures/audio/wzsn-mixer-vectors-q16_16-v1.json`; `tests/fixtures/audio/wzsn-ay-channel-mixer-v1.json`; `tests/golden/audio/wzsn-audio-v1.pcm` | `certification/results/core/C50.AC26.json` |
| C50.AC27 | 3573 | Sokol audio does not drive CPU execution; | C21.01 | `tests/fixtures/audio/wzsn-audio-v1.pcm`; `tests/fixtures/audio/wzsn-sokol-push-degradation-v1.json` | `certification/results/core/C50.AC27.json` |
| C50.AC28 | 3574 | host sound is enabled only from 0.5x through 2.0x inclusive and is muted outside that range without stopping emulated beeper/AY progression; | C10.08; C20.03; C21.03 | `tests/fixtures/audio/wzsn-runtime-speed-audio-v1.json`; `tests/fixtures/audio/wzsn-sokol-push-degradation-v1.json`; `tests/fixtures/audio/wzsn-ay-channel-mixer-v1.json` | `certification/results/core/C50.AC28.json` |
| C50.AC29 | 3576 | audio buffering cannot silently force the emulator away from the selected runtime speed; | C21.03 | `tests/fixtures/audio/wzsn-sokol-push-degradation-v1.json` | `certification/results/core/C50.AC29.json` |
| C50.AC30 | 3578 | runtime speed control changes host pacing without changing internal Spectrum timing relationships; | C10.05; C32.01 | `tests/fixtures/traces/wzsn-speed-transition-v1.json`; `tests/fixtures/pacing/wzsn-pacing-v1.json` | `certification/results/core/C50.AC30.json` |
| C50.AC31 | 3580 | runtime speed can change while running without discontinuity in the master-tick timeline; | C10.06 | `tests/fixtures/traces/wzsn-speed-transition-v1.json` | `certification/results/core/C50.AC31.json` |
| C50.AC32 | 3582 | emulator-controlled cassette playback and SAVE output automatically follow the runtime speed multiplier in wall-clock time while retaining canonical emulated timing; | C23.03; C23.09 | `tests/fixtures/tape/wzsn-tap-standard-oneblock.tap`; `tests/fixtures/tape/wzsn-warajevo-native-minimal.tap` | `certification/results/core/C50.AC32.json` |
| C50.AC33 | 3585 | Normal cassette loading is the default loading mode; | C23.03 | `tests/fixtures/tape/wzsn-tap-standard-oneblock.tap` | `certification/results/core/C50.AC33.json` |
| C50.AC34 | 3586 | Normal loading executes the real emulated loader against timed EAR edges; | C23.03; C23.09 | `tests/fixtures/tape/wzsn-tap-standard-oneblock.tap`; `tests/fixtures/tzx/wzsn-tzx-standard-speed.tzx` | `certification/results/core/C50.AC34.json` |
| C50.AC35 | 3587 | Instant/Trap loading is optional and explicitly selectable; | C23.05; U11.01 | `tests/fixtures/tape/wzsn-trap-rom-standard.tap`; `tests/fixtures/ui/U11-tape-quick.json` | `certification/results/core/C50.AC35.json` |
| C50.AC36 | 3588 | a supported Instant/Trap load reaches the same Spectrum-observable post-load canonical machine state as the corresponding Normal load from the same initial state and tape data; | C23.05 | `tests/fixtures/tape/wzsn-trap-rom-standard.tap`; `tests/fixtures/tape/wzsn-trap-checksum-error.tap` | `certification/results/core/C50.AC36.json` |
| C50.AC37 | 3591 | unsupported, uncertain, or non-equivalent trap cases fall back safely to Normal loading; | C23.06 | `tests/fixtures/tzx/wzsn-trap-custom-loader.tzx`; `tests/fixtures/tzx/wzsn-trap-copy-protection.tzx`; `tests/fixtures/tape/wzsn-trap-unsupported-rom.tap` | `certification/results/core/C50.AC37.json` |
| C50.AC38 | 3593 | local and Telnet source ownership is resolved outside the Spectrum core; | C22.02; C55.03 | `tests/fixtures/input/wzsn-input-ownership-v1.json`; `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json` | `certification/results/core/C50.AC38.json` |
| C50.AC39 | 3594 | the Spectrum keyboard core receives only normalized physical matrix transitions, never source identity; | C22.02; C55.03 | `tests/fixtures/input/wzsn-input-ownership-v1.json`; `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json` | `certification/results/core/C50.AC39.json` |
| C50.AC40 | 3596 | simultaneous row selection follows authentic matrix-selection behavior; | C22.01; C55.05 | `tests/fixtures/input/wzsn-keyboard-matrix-v1.json`; `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `design/input-hardware-evidence.md` | `certification/results/core/C50.AC40.json` |
| C50.AC41 | 3597 | the complete Telnet transport/keyboard acceptance contract in Section 55.20 passes and the companion UI Telnet-control acceptance contract passes; | C55.05; U50.01; U48.01 | `tests/fixtures/telnet/wzsn-telnet-negotiation-v1.bin`; `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt`; `tests/fixtures/telnet/wzsn-control-port-race-v1.json`; `certification/manifests/ui-section48-acceptance.json` | `certification/results/core/C50.AC41.json` |
| C50.AC42 | 3599 | snapshots are portable across x86-64 and AArch64; | C25.03; C25.04 | `design/media-format-support.md`; `tests/fixtures/snapshot/wzsn-sna-48k-canonical.sna`; `tests/fixtures/snapshot/wzsn-sna-128k-canonical.sna`; `tests/fixtures/snapshot/wzsn-z80-v2-48k.z80`; `tests/fixtures/snapshot/wzsn-z80-v2-128k.z80`; `tests/fixtures/snapshot/wzsn-native-state-v1.json` | `certification/results/core/C50.AC42.json` |
| C50.AC43 | 3600 | original Warajevo media/peripheral behavior is regression-tested; | C39.01; C24.03; C25.06 | `certification/manifests/warajevo-source-authority.json`; `certification/manifests/warajevo250-differential-cases.json`; `reference/original-warajevo/snapshots/warajevo-sna-48k-reference.sna` | `certification/results/core/C50.AC43.json` |
| C50.AC44 | 3601 | known differences from Warajevo 2.50, and from observable 2.51/2.52 behavior where used as evidence, are documented when hardware correctness intentionally supersedes historical approximation; | C39.02; C39.03 | `certification/manifests/warajevo-source-authority.json`; `certification/manifests/warajevo-251-252-behavior-evidence.json`; `certification/manifests/warajevo250-differential-cases.json`; `certification/hardware/captures-manifest.json` | `certification/results/core/C50.AC44.json` |
| C50.AC45 | 3604 | Windows x86-64 passes the release test suite; | C34.01 | `certification/config/platform-matrix.json`; `certification/manifests/release-artifacts.json`; `certification/manifests/platform-run-artifacts.json` | `certification/results/core/C50.AC45.json` |
| C50.AC46 | 3605 | Linux x86-64 passes the release test suite; | C34.01 | `certification/config/platform-matrix.json`; `certification/manifests/release-artifacts.json`; `certification/manifests/platform-run-artifacts.json` | `certification/results/core/C50.AC46.json` |
| C50.AC47 | 3606 | Linux AArch64 passes the release test suite; | C34.01 | `certification/config/platform-matrix.json`; `certification/manifests/release-artifacts.json`; `certification/manifests/platform-run-artifacts.json` | `certification/results/core/C50.AC47.json` |
| C50.AC48 | 3607 | macOS Apple Silicon passes the release test suite; | C34.01 | `certification/config/platform-matrix.json`; `certification/manifests/release-artifacts.json`; `certification/manifests/platform-run-artifacts.json` | `certification/results/core/C50.AC48.json` |
| C50.AC49 | 3608 | release dependency inspection confirms the single-program-binary policy; | C28.01; C34.01 | `certification/manifests/release-artifacts.json`; `certification/manifests/platform-run-artifacts.json`; `certification/scripts/inspect-dependencies` | `certification/results/core/C50.AC49.json` |
| C50.AC50 | 3609 | ROM/firmware redistribution status is explicit and legally separated from emulator architecture. | C29.01; C29.03; C48.03 | `certification/manifests/roms.json`; `certification/manifests/rom-license-separation.json`; `certification/manifests/dependencies.json`; `LICENSE.txt`; `NOTICE.md` | `certification/results/core/C50.AC50.json` |
| C50.AC51 | 3611 | the complete pinned Fuse Z80 unit-test suite passes with no unexplained skipped or failing applicable cases; | C11.03; C36.02 | `external/fuse-1.9.2/z80/tests/tests.in`; `external/fuse-1.9.2/z80/tests/tests.expected` | `certification/results/core/C50.AC51.json` |
| C50.AC52 | 3613 | Phase 0 machine-timing evidence and ROM-hash baselines are frozen before dependent timing implementation is accepted; | C09.03; C49.G0 | `design/machine-timing-evidence.md`; `certification/manifests/roms.json`; `tests/fixtures/timing/zx48-pal-timing-v1.json`; `tests/fixtures/timing/zx128-pal-timing-v1.json` | `certification/results/core/C50.AC52.json` |
| C50.AC53 | 3615 | the mandatory Phase 4A early timing-smoke gate passes before host/UI work is allowed to mask foundational timing defects; | C49.G4A | `tests/fixtures/video/wzsn-raster-race.tap`; `external/ramsoft/floatspy.tap`; `certification/gates/external/E06-floatspy-identity.json`; `external/softspectrum/timing_tests-48k_v1.0.tzx` | `certification/results/core/C50.AC53.json` |
| C50.AC54 | 3617 | the machine support matrix in Section 9.1 and legacy-feature disposition in Section 2.5 are reflected in the backlog with no unspecified "remaining" compatibility work; | C02.04; C49.02 | `wzsn-architectures-1-2-developer-tasks.md`; `certification/manifests/legacy-routine-inventory.json`; `certification/manifests/core-ticket-derivation-v1.json` | `certification/results/core/C50.AC54.json` |
| C50.AC55 | 3620 | `design/media-format-support.md` is frozen to the required block/variant detail before Phase 7/8 media tickets are accepted; | C23.10; C25.01 | `design/media-format-support.md`; `tests/fixtures/tape/wzsn-format-disposition-v1.json`; `tests/fixtures/snapshot/wzsn-sna-48k-canonical.sna`; `tests/fixtures/snapshot/wzsn-z80-v2-48k.z80` | `certification/results/core/C50.AC55.json` |
| C50.AC56 | 3622 | the private development-only `WZSN-PRIVATE-TEST-MEDIA` regression run has completed with no unexplained failures before full difficult-media or hardware-hack compatibility is claimed; the private files themselves are not repository or distribution artifacts; | C36.03; C49.G14 | `WZSN-PRIVATE-TEST-MEDIA/`; `certification/results/core/C36.03.json` | `certification/results/core/C50.AC56.json` |
| C50.AC57 | 3626 | every migrated legacy routine has provenance, copyright/license classification, derivation clearance, C destination, and regression evidence; | C41.01; C48.02 | `design/migration-ledger.md`; `certification/manifests/migration-record-schema-v1.json`; `certification/manifests/legacy-routine-inventory.json` | `certification/results/core/C50.AC57.json` |
| C50.AC58 | 3628 | the Phase-10 Interface1/Microdrive/ZXNet companion specification is frozen before those implementation tickets are accepted; | C24.05; C49.G10 | `design/interface1-microdrive-zxnet.md`; `certification/manifests/interface1-phase10-freeze-ledger.json` | `certification/results/core/C50.AC58.json` |
| C50.AC59 | 3630 | every implementation phase satisfies its Section 49.1 exit gate; | C49.01 | `wzsn-architectures-1-2-developer-tasks.md`; `certification/manifests/core-phase-gate-inputs.json`; `certification/results/phase-gates.json`; `certification/results/core/C49.01.json` | `certification/results/core/C50.AC59.json` |
| C50.AC60 | 3631 | Telnet Control-Port probing/bind-family policy, Telnet framing/negotiation, keyboard command vocabulary, hold interval, scheduling rule, and second-client behavior conform to Section 55, while application-control grammar and command-registry projection conform to the UI architecture; | C55.01; C55.02; C55.03; C55.05; U50.01 | `tests/fixtures/telnet/wzsn-control-port-race-v1.json`; `tests/fixtures/telnet/wzsn-telnet-negotiation-v1.bin`; `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `certification/manifests/ui-section48-acceptance.json` | `certification/results/core/C50.AC60.json` |
| C50.AC61 | 3635 | implementation tickets satisfy the Section 49.2 derivation contract and do not contain unresolved architecture decisions; | C49.02 | `wzsn-architectures-1-2-developer-tasks.md`; `certification/manifests/core-ticket-derivation-v1.json` | `certification/results/core/C50.AC61.json` |
| C50.AC62 | 3637 | real-hardware/reference certification covers every item required by Section 40 before the architecture-complete milestone; | C40.01; C40.02; C40.03 | `certification/hardware/zx48-certified-profile-inventory.json`; `certification/hardware/zx128-pal-inventory.json`; `certification/hardware/captures-manifest.json` | `certification/results/core/C50.AC62.json` |
| C50.AC63 | 3639 | every mandatory initial target/compiler group in Section 35 executes the required deterministic regression suite; | C35.01; C35.02; C35.03 | `certification/config/compiler-matrix.json`; `certification/manifests/runner-provenance.json` | `certification/results/core/C50.AC63.json` |
| C50.AC64 | 3641 | Kempston joystick emulation uses normalized joystick state rather than keyboard shortcuts inside the core, and direct port-read tests pass; | C22.03 | `tests/fixtures/input/wzsn-kempston-port-v1.json`; `design/input-hardware-evidence.md` | `certification/results/core/C50.AC64.json` |
| C50.AC65 | 3643 | every required UI workflow, stable command ID, GUI/Telnet/test projection, remote-permission rule, screenshot workflow, and menu-state rule defined by `design/warajevo-zx-spectrum-next-ui-architecture.md` passes its companion acceptance contract; | U48.01 | `certification/manifests/ui-section48-acceptance.json`; `certification/manifests/ui-section48-proof-inputs.json` | `certification/results/core/C50.AC65.json` |
| C50.AC66 | 3647 | concurrent WZSN processes allocate Control Ports by the Section-55.2 first-free 30740-32787 contract without duplicate numeric ownership, and full range exhaustion is nonfatal; | C55.01; C55.05 | `tests/fixtures/telnet/wzsn-control-port-race-v1.json` | `certification/results/core/C50.AC66.json` |
| C50.AC67 | 3650 | the core networking-mode state is exactly `NONE`, `INTERFACE1`, or `EAR_MIC` and cannot represent simultaneous Interface-1/Ear+Mic activation; | C24.01 | `tests/fixtures/networking/wzsn-networking-modes-v1.json` | `certification/results/core/C50.AC67.json` |
| C50.AC68 | 3652 | `NONE`/`INTERFACE1` cold-reconfiguration destroys prior RAM/hook/device state, preserves only application run/pause state, and resolves dirty Microdrive media without silent data loss before leaving Interface-1; | C24.01; C24.02 | `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/networking/wzsn-networking-transition-edge-v1.json`; `tests/fixtures/mdr/wzsn-mdr-dirty-transition-v1.json` | `certification/results/core/C50.AC68.json` |
| C50.AC69 | 3655 | before Architecture #3 is implemented, `EAR_MIC` cannot falsely expose a working routed network; when implemented it is selectable only on an Architecture-#3-certified ZX Spectrum 48K Issue-2 profile/variant, and its actual fidelity/stack/bootstrap acceptance is owned by that downstream document; | C24.04; M31.01 | `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `certification/manifests/micear-section31-acceptance.json` | `certification/results/core/C50.AC69.json` |
| C50.AC70 | 3660 | concurrent WZSN processes satisfy the Section-28.7 host-data safety rules for settings, writable media, snapshots/exports/conversions, temporary files, and Telnet screenshots; | C28.03 | `tests/fixtures/concurrency/wzsn-multi-instance-plan-v1.json` | `certification/results/core/C50.AC70.json` |
| C50.AC71 | 3663 | every developer/debug WZSN process can own an independent binary circular timing trace file whose total size never exceeds 16 MiB; | C44.01; C44.05 | `tests/fixtures/trace/wzsn-trace-synthetic-worstcase.bin`; `tests/fixtures/trace/wzsn-trace-header-contract-v1.json` | `certification/results/core/C50.AC71.json` |
| C50.AC72 | 3665 | the trace records canonical master ticks, same-tick ordering, CPU/bus/ULA/ contention/interrupt state deeply enough to reconstruct timing failures and `TIMING_FULL` retains at least eight complete 48K frames under the Phase-2 retention measurement; | C44.02; C44.06 | `tests/fixtures/trace/wzsn-trace-real-48k-stress.tap`; `tests/fixtures/trace/wzsn-trace-event-schema-v1.json`; `tools/wz_trace_dump` | `certification/results/core/C50.AC72.json` |
| C50.AC73 | 3669 | trace wrap, freeze, file-I/O failure, and trace enable/disable do not alter canonical machine state, hashes, or deterministic event ordering; | C44.03; C44.07 | `tests/fixtures/trace/wzsn-trace-synthetic-worstcase.bin`; `tests/fixtures/trace/wzsn-trace-real-48k-stress.tap`; `tests/fixtures/trace/wzsn-trace-service-policy-v1.json` | `certification/results/core/C50.AC73.json` |
| C50.AC74 | 3671 | concurrent WZSN processes cannot share or overwrite one another's trace files, and a standalone reader can recover complete records from a wrapped or abruptly terminated trace while rejecting incomplete trailing data; | C44.01; C44.04; C44.05 | `tests/fixtures/trace/wzsn-trace-header-contract-v1.json`; `tools/wz_trace_dump`; `tests/fixtures/concurrency/wzsn-multi-instance-plan-v1.json` | `certification/results/core/C50.AC74.json` |
| C50.AC75 | 3674 | the circular trace backend is implemented and tested during Phase 1 and is available before Phase-2 CPU/timing implementation begins. | C44.01; C49.G1 | `tests/fixtures/trace/wzsn-trace-synthetic-worstcase.bin`; `tests/fixtures/trace/wzsn-trace-header-contract-v1.json`; `certification/results/core/C49.G1.json` | `certification/results/core/C50.AC75.json` |

`certification/manifests/core-section50-acceptance.json` must enumerate `C50.AC01` through `C50.AC75` exactly once and retain the lower-level proof/test/artifact mapping shown above.

## C51. Architecture section 51: Deferred decisions and mandatory freeze gates

**Architecture authority:** `C` source lines 3679-3734, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Deferred-decision freeze gates.

**Required artifacts:**

- `certification/gates/core/`
- `certification/manifests/core-section51-already-frozen.json`

**Artifact/source authority:** Project gate artifacts named in the Core Gate Artifact table plus a literal manifest of decisions Section 51 declares already frozen.

**Tests:**

### C51.01

Before each named phase, verify the required decision artifact exists, is review-approved, and is referenced by every dependent test: 48K master clock/ordering/contention/floating bus; trace layout; raster encoding; Sokol revision/backend; crop; audio rate/mixer/AY/resampler; IF1 ROM handling; UI toolkit; Linux packages; keyboard ghosting conclusion.

**Evidence output:** `certification/results/core/C51.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C51.02

Attempting to certify a dependent test while its gate artifact is missing must produce BLOCKED, never an inferred default.

**Evidence output:** `certification/results/core/C51.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C51 exact gate-artifact ledger

| Deferred decision | Required gate | Exact required gate artifact |
|---|---|---|
| exact 48K master-tick frequency/ratios | Phase 0 exit | `certification/gates/core/C51-master-tick-frequency-ratios.md` |
| exact 48K same-edge CPU/ULA ordering | Phase 0 exit | `certification/gates/core/C51-same-edge-ordering.md` |
| exact 48K contention tables | Phase 0 exit | `certification/gates/core/C51-contention-tables.md` |
| exact 48K floating-bus model | Phase 0 exit | `certification/gates/core/C51-floating-bus-model.md` |
| versioned circular trace binary record/header layout | Phase 0 exit | `certification/gates/core/C51-trace-binary-layout.md` |
| logical raster sample encoding | Phase 4 implementation | `certification/gates/core/C51-raster-sample-encoding.md` |
| exact Sokol revision | Phase 5 implementation | `certification/gates/core/C51-sokol-revision.md` |
| exact Linux graphics backend within the X11 baseline | Phase 5 implementation | `certification/gates/core/C51-linux-graphics-backend.md` |
| default host-visible crop/border | Phase 5 exit | `certification/gates/core/C51-default-crop-border.md` |
| canonical internal audio sample rate | Phase 6 implementation | `certification/gates/core/C51-audio-sample-rate.md` |
| fixed-point mixer representation | Phase 6 implementation | `certification/gates/core/C51-fixed-point-mixer.md` |
| AY analog mixing model | Phase 6 implementation | `certification/gates/core/C51-ay-analog-mixing.md` |
| host resampling algorithm for 0.5x..2.0x audio | Phase 6 exit | `certification/gates/core/C51-host-resampler.md` |
| Interface 1 ROM test/redistribution handling | Phase 10 implementation | `certification/gates/core/C51-interface1-rom-handling.md` |
| exact UI toolkit | Phase 12 implementation | `certification/gates/ui/ui-phase12-toolkit.md` |
| Linux package formats | Phase 13 implementation | `certification/gates/core/C51-linux-package-formats.md` |
| keyboard ghosting/electrical conclusion beyond multiple-row selection | Phase 5 exit | `certification/gates/core/C51-keyboard-ghosting.md` |

### C51.03

Reconcile `core-section51-already-frozen.json` literally against every Section-51 "Already frozen" item: ISO C11; CMake; initial 48K/128K profiles; native Wayland not initially required; Windows ARM64 not initial; macOS x86-64 secondary; CPU micro-op representation and scheduler container/data structure implementation freedom subject to behavior; in-memory C layout/internal C API signature freedom unless constrained by serialization/ABI/test/subsystem contract; UI semantic menu/toolbar/stable IDs/Telnet application-control surface frozen by Architecture #2 with typography/localization presentation freedom; and Section-55 Telnet transport/keyboard semantics versus Architecture-2 application grammar/permissions. Attempting to reopen any of these as an implementation-local design choice must fail the gate.

**Evidence output:** `certification/results/core/C51.03.json`

**Pass rule:** PASS only when deferred and already-frozen decisions are distinguished exactly and no developer/host can silently choose either class.

## C52. Architecture section 52: Non-goals for the initial architecture

**Architecture authority:** `C` source lines 3735-3752, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Initial non-goals remain absent.

**Required artifacts:**

- `certification/manifests/non-goals-core.json`

**Artifact/source authority:** Project-generated non-goal manifest.

**Tests:**

### C52.01

Static/UI/runtime audit confirms initial release does not claim embedding every OS library, native Wayland, mobile platforms, WebAssembly, shader CRT simulation, JIT, multithreaded Spectrum core execution, deterministic-core hardware SIMD, application-specific timing hacks, or exact reproduction of historical Warajevo bugs that conflict with authenticated hardware as required features.

**Evidence output:** `certification/results/core/C52.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C52.02

Presence of a later optional feature must not weaken any frozen initial correctness boundary.

**Evidence output:** `certification/results/core/C52.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## C53. Architecture section 53: Final architectural contract

**Architecture authority:** `C` source lines 3753-3845, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Six architectural boundaries.

**Required artifacts:**

- `certification/manifests/architecture-boundaries.json`

**Artifact/source authority:** Project-generated boundary manifest.

**Tests:**

### C53.01

Audit source/link/runtime paths for preserved-source→portable-C provenance, CPU→timed-bus, master-time→machine vs host-time→pacing, canonical output→presentation, program binary→OS, and front-end→registry→orchestrator boundaries.

**Evidence output:** `certification/results/core/C53.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C53.02

Mutation tests for representative boundary violations must be caught.

**Evidence output:** `certification/results/core/C53.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## C54. Architecture section 54: Architecture baseline

**Architecture authority:** `C` source lines 3846-3947, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Frozen architecture baseline consistency.

**Required artifacts:**

- `certification/manifests/core-baseline-reconciliation.json`
- `certification/manifests/core-baseline-proof-inputs.json`
- `certification/manifests/build.json`
- `certification/manifests/release-artifacts.json`
- `warajevo-zx-spectrum-next-ui-architecture.md`
- `zx48-mic-ear-router-network-architecture.md`

**Artifact/source authority:** `core-baseline-reconciliation.json` records the frozen baseline comparison result. `core-baseline-proof-inputs.json` is generated from the literal Section-54 baseline and records, for every frozen baseline statement, the exact lower-level C/U/M test IDs and proof-artifact paths used to substantiate it; the current Architecture #2/#3 disk copies, build manifest, and exact release-artifact manifest are direct inputs rather than inferred context.

**Tests:**

### C54.01

Reconcile every frozen baseline statement one-for-one using `core-baseline-proof-inputs.json` against implemented configuration, lower-level tests, the current UI/MIC-EAR authority documents, `build.json`, `release-artifacts.json`, and the release claim; no row may PASS from the reconciliation manifest alone and no obsolete contradiction is allowed.

**Evidence output:** `certification/results/core/C54.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C54.02

Compare the baseline directly to `warajevo-zx-spectrum-next-ui-architecture.md` and `zx48-mic-ear-router-network-architecture.md`, then require the exact lower-level networking/Control-Port/delegation proof artifacts recorded in `core-baseline-proof-inputs.json`; a textual architecture match without executable evidence is insufficient.

**Evidence output:** `certification/results/core/C54.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## C55. Architecture section 55: Special feature 1 - single-client Telnet keyboard/control transport

**Architecture authority:** `C` source lines 3948-4612, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Single-client Telnet keyboard/control transport.

**Required artifacts:**

- `tests/fixtures/telnet/wzsn-telnet-negotiation-v1.bin`
- `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt`
- `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`
- `tests/fixtures/telnet/wzsn-control-port-race-v1.json`
- `tests/fixtures/input/wzsn-input-ownership-v1.json`
- `tests/fixtures/traces/wzsn-speed-transition-v1.json`
- `design/input-hardware-evidence.md`
- `certification/manifests/allowed-dependencies.json`
- `certification/manifests/dependencies.json`
- `certification/manifests/command-authority.json`
- `certification/manifests/core-section55-acceptance.json`
- `certification/manifests/ui-section48-acceptance.json`
- `warajevo-zx-spectrum-next-ui-architecture.md`

**Artifact/source authority:** Project-generated Telnet wire/input/race fixtures; `core-section55-acceptance.json` is generated from the literal C55.AC01-C55.AC31 ledger and records the exact input-artifact/result mapping for all 31 transport/keyboard acceptance criteria. No external network artifact is required.

**Tests:**

### C55.01

Verify automatic first-free Control Port selection 30740..32787, wildcard family ownership, split-family collision rejection, degraded-family reporting, simultaneous-process atomic bind, range exhaustion nonfatal, session-only port, and one active client with exact `BUSY\r\n` second-client response.

**Evidence output:** `certification/results/core/C55.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C55.02

Verify Telnet IAC escaping/negotiation/subnegotiation handling, no unsolicited banner, bounded malformed/overlong input recovery, plaintext/no-auth boundary, and native socket isolation from core.

**Evidence output:** `certification/results/core/C55.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C55.03

Verify exact physical key vocabulary, KEY DOWN/UP/PRESS/RELEASE ALL ownership, KEY PRESS exactly two active-machine frame periods, held/pending rejection, disconnect release, reset persistence, and normalized current-master-tick FIFO scheduling identical to local input.

**Evidence output:** `certification/results/core/C55.03.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C55.04

Verify network thread/poll cannot mutate machine directly; deterministic replay uses normalized matrix transitions, not TCP timing.

**Evidence output:** `certification/results/core/C55.04.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C55.05

Execute every Section 55.19 test and every Section 55.20 acceptance item; generate `core-section55-acceptance.json` from the 31 literal acceptance rows with their exact proof inputs/results; non-keyboard controls must dispatch only through Architecture #2 registry. `C55.05.json` is the aggregate execution result and the manifest is its auditable criterion map.

**Evidence output:** `certification/results/core/C55.05.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### C55.19 literal transport/keyboard test ledger

Every nonblank item in the Section-55.19 architecture test block is retained as a dedicated subtest. Blank separator lines are presentation only. Each row names the concrete wire/input/race/hardware artifact needed to execute it.

| Subtest | Literal architecture test item | Required input artifact(s) | Evidence artifact |
|---|---|---|---|
| C55.T01 | local A down/up | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/input/wzsn-input-ownership-v1.json` | `certification/results/core/C55.T01.json` |
| C55.T02 | Telnet A down/up | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/input/wzsn-input-ownership-v1.json` | `certification/results/core/C55.T02.json` |
| C55.T03 | identical effective matrix state | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/input/wzsn-input-ownership-v1.json` | `certification/results/core/C55.T03.json` |
| C55.T04 | local + Telnet same key held concurrently | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/input/wzsn-input-ownership-v1.json` | `certification/results/core/C55.T04.json` |
| C55.T05 | one source releases while other still holds | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/input/wzsn-input-ownership-v1.json` | `certification/results/core/C55.T05.json` |
| C55.T06 | CAPS SHIFT / SYMBOL SHIFT combinations | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/input/wzsn-input-ownership-v1.json` | `certification/results/core/C55.T06.json` |
| C55.T07 | multi-key combinations | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/input/wzsn-input-ownership-v1.json` | `certification/results/core/C55.T07.json` |
| C55.T08 | direct keyboard-matrix scanning software | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/input/wzsn-input-ownership-v1.json` | `certification/results/core/C55.T08.json` |
| C55.T09 | Telnet disconnect with keys held | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/input/wzsn-input-ownership-v1.json` | `certification/results/core/C55.T09.json` |
| C55.T10 | local keys preserved after Telnet disconnect | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/input/wzsn-input-ownership-v1.json` | `certification/results/core/C55.T10.json` |
| C55.T11 | second-client rejection | `tests/fixtures/telnet/wzsn-control-port-race-v1.json` | `certification/results/core/C55.T11.json` |
| C55.T12 | new client accepted after first disconnect | `tests/fixtures/telnet/wzsn-control-port-race-v1.json` | `certification/results/core/C55.T12.json` |
| C55.T13 | IAC IAC, WILL/WONT/DO/DONT, and subnegotiation handling | `tests/fixtures/telnet/wzsn-telnet-negotiation-v1.bin`; `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt` | `certification/results/core/C55.T13.json` |
| C55.T14 | unsupported Telnet options rejected without reaching command parser | `tests/fixtures/telnet/wzsn-telnet-negotiation-v1.bin`; `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt` | `certification/results/core/C55.T14.json` |
| C55.T15 | no unsolicited application banner on successful connection | `tests/fixtures/telnet/wzsn-telnet-negotiation-v1.bin`; `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt` | `certification/results/core/C55.T15.json` |
| C55.T16 | bounded line buffering and malformed-input recovery | `tests/fixtures/telnet/wzsn-telnet-negotiation-v1.bin`; `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt` | `certification/results/core/C55.T16.json` |
| C55.T17 | KEY PRESS produces exactly two emulated frame periods of hold | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json` | `certification/results/core/C55.T17.json` |
| C55.T18 | KEY PRESS on already-held/pending key is rejected without changing the hold | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json` | `certification/results/core/C55.T18.json` |
| C55.T19 | multiple keyboard rows selected simultaneously | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json` | `certification/results/core/C55.T19.json` |
| C55.T20 | matrix electrical/ghosting tests as required by hardware evidence | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `design/input-hardware-evidence.md` | `certification/results/core/C55.T20.json` |
| C55.T21 | automatic listener startup | `tests/fixtures/telnet/wzsn-control-port-race-v1.json` | `certification/results/core/C55.T21.json` |
| C55.T22 | first-free probing from 30740 through 32787 inclusive | `tests/fixtures/telnet/wzsn-control-port-race-v1.json` | `certification/results/core/C55.T22.json` |
| C55.T23 | simultaneous multi-process startup cannot produce duplicate Control Port ownership | `tests/fixtures/telnet/wzsn-control-port-race-v1.json` | `certification/results/core/C55.T23.json` |
| C55.T24 | IPv4/IPv6 wildcard-family reporting and degraded-family behavior | `tests/fixtures/telnet/wzsn-control-port-race-v1.json` | `certification/results/core/C55.T24.json` |
| C55.T25 | candidate rejection when any supported family reports address-in-use | `tests/fixtures/telnet/wzsn-control-port-race-v1.json` | `certification/results/core/C55.T25.json` |
| C55.T26 | 2048-port-range exhaustion is nonfatal and visibly reported | `tests/fixtures/telnet/wzsn-control-port-race-v1.json` | `certification/results/core/C55.T26.json` |
| C55.T27 | selected Control Port is not persisted and no port outside the range is chosen | `tests/fixtures/telnet/wzsn-control-port-race-v1.json` | `certification/results/core/C55.T27.json` |
| C55.T28 | second client receives BUSY then closes | `tests/fixtures/telnet/wzsn-control-port-race-v1.json` | `certification/results/core/C55.T28.json` |
| C55.T29 | runtime speed changes while Telnet client is active | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/traces/wzsn-speed-transition-v1.json` | `certification/results/core/C55.T29.json` |
| C55.T30 | Normal/Unlimited emulator speed operation | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/traces/wzsn-speed-transition-v1.json` | `certification/results/core/C55.T30.json` |
| C55.T31 | host-input FIFO order and current-master-tick scheduling rule | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/input/wzsn-input-ownership-v1.json` | `certification/results/core/C55.T31.json` |
| C55.T32 | headless deterministic replay from normalized key trace | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/headless/wzsn-headless-workload-matrix-v1.json` | `certification/results/core/C55.T32.json` |
| C55.T33 | non-keyboard control line dispatch reaches shared command registry | `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt`; `certification/manifests/command-authority.json`; `warajevo-zx-spectrum-next-ui-architecture.md` | `certification/results/core/C55.T33.json` |
| C55.T34 | Telnet networking layer contains no private reset/speed/screenshot semantics | `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt`; `certification/manifests/command-authority.json`; `warajevo-zx-spectrum-next-ui-architecture.md` | `certification/results/core/C55.T34.json` |

### C55.20 acceptance-item ledger

| Test | Source line | Literal acceptance criterion | Required proof artifact(s) | Evidence artifact |
|---|---:|---|---|---|
| C55.AC01 | 4560 | the Telnet listener starts automatically with each Warajevo application process; | `tests/fixtures/telnet/wzsn-control-port-race-v1.json` | `certification/results/core/C55.AC01.json` |
| C55.AC02 | 4561 | each process probes candidate Control Ports in strict ascending order from 30740 through 32787 inclusive and selects the first candidate satisfying the Section-55.2 family-ownership rule; | `tests/fixtures/telnet/wzsn-control-port-race-v1.json` | `certification/results/core/C55.AC02.json` |
| C55.AC03 | 4564 | simultaneous WZSN process startup cannot result in two processes owning or reporting the same Control Port through split IPv4/IPv6 or shared-listener semantics; | `tests/fixtures/telnet/wzsn-control-port-race-v1.json` | `certification/results/core/C55.AC03.json` |
| C55.AC04 | 4567 | exhaustion of all 2048 candidates is nonfatal, visibly reported, and never causes automatic selection outside the frozen range; | `tests/fixtures/telnet/wzsn-control-port-race-v1.json` | `certification/results/core/C55.AC04.json` |
| C55.AC05 | 4569 | the selected Control Port is session state and is not persisted across application launches; | `tests/fixtures/telnet/wzsn-control-port-race-v1.json` | `certification/results/core/C55.AC05.json` |
| C55.AC06 | 4571 | active/degraded IPv4/IPv6 family state is visibly reported for the selected Control Port; | `tests/fixtures/telnet/wzsn-control-port-race-v1.json` | `certification/results/core/C55.AC06.json` |
| C55.AC07 | 4573 | one active Telnet client per WZSN process is supported; | `tests/fixtures/telnet/wzsn-control-port-race-v1.json` | `certification/results/core/C55.AC07.json` |
| C55.AC08 | 4574 | a second simultaneous client receives `BUSY` and is immediately closed without disturbing the first; | `tests/fixtures/telnet/wzsn-control-port-race-v1.json` | `certification/results/core/C55.AC08.json` |
| C55.AC09 | 4576 | local keyboard input remains continuously available; | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/input/wzsn-input-ownership-v1.json` | `certification/results/core/C55.AC09.json` |
| C55.AC10 | 4577 | local/Telnet source identity and ownership are resolved outside the Spectrum core; | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/input/wzsn-input-ownership-v1.json` | `certification/results/core/C55.AC10.json` |
| C55.AC11 | 4579 | the Spectrum core receives only normalized physical matrix transitions and master ticks; | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/input/wzsn-input-ownership-v1.json` | `certification/results/core/C55.AC11.json` |
| C55.AC12 | 4581 | local and Telnet equivalent effective transitions are Spectrum-indistinguishable; | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/input/wzsn-input-ownership-v1.json` | `certification/results/core/C55.AC12.json` |
| C55.AC13 | 4583 | releases from one source cannot cancel a key held by the other; | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/input/wzsn-input-ownership-v1.json` | `certification/results/core/C55.AC13.json` |
| C55.AC14 | 4584 | direct keyboard-matrix-scanning software sees Telnet keys correctly; | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/input/wzsn-input-ownership-v1.json` | `certification/results/core/C55.AC14.json` |
| C55.AC15 | 4585 | simultaneous selection of multiple keyboard rows follows the authentic matrix-selection model; | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/input/wzsn-input-ownership-v1.json` | `certification/results/core/C55.AC15.json` |
| C55.AC16 | 4587 | any real matrix ghosting/electrical behavior is validated from hardware evidence before being implemented or ruled out; | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `design/input-hardware-evidence.md` | `certification/results/core/C55.AC16.json` |
| C55.AC17 | 4589 | no ROM/BASIC input shortcut is used; | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/input/wzsn-input-ownership-v1.json` | `certification/results/core/C55.AC17.json` |
| C55.AC18 | 4590 | `KEY DOWN` and `KEY UP` provide explicit physical-key state control; | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/input/wzsn-input-ownership-v1.json` | `certification/results/core/C55.AC18.json` |
| C55.AC19 | 4591 | `KEY PRESS` holds for exactly two active-machine emulated frame periods and refuses to override an existing Telnet hold; | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/input/wzsn-input-ownership-v1.json` | `certification/results/core/C55.AC19.json` |
| C55.AC20 | 4593 | Telnet disconnect releases only Telnet-held keys; | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/input/wzsn-input-ownership-v1.json` | `certification/results/core/C55.AC20.json` |
| C55.AC21 | 4594 | Telnet negotiation bytes cannot become keyboard or application commands; | `tests/fixtures/telnet/wzsn-telnet-negotiation-v1.bin`; `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt` | `certification/results/core/C55.AC21.json` |
| C55.AC22 | 4595 | malformed network input cannot corrupt Spectrum state; | `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt`; `tests/fixtures/telnet/wzsn-telnet-negotiation-v1.bin` | `certification/results/core/C55.AC22.json` |
| C55.AC23 | 4596 | deterministic keyboard replay does not require the network server; | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/headless/wzsn-headless-workload-matrix-v1.json` | `certification/results/core/C55.AC23.json` |
| C55.AC24 | 4597 | the feature works at every runtime speed; | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/traces/wzsn-speed-transition-v1.json` | `certification/results/core/C55.AC24.json` |
| C55.AC25 | 4598 | no third-party networking shared library is introduced; | `certification/manifests/dependencies.json`; `certification/scripts/inspect-dependencies` | `certification/results/core/C55.AC25.json` |
| C55.AC26 | 4599 | the socket/network layer remains outside the deterministic Spectrum core; | `certification/manifests/allowed-dependencies.json`; `certification/manifests/command-authority.json` | `certification/results/core/C55.AC26.json` |
| C55.AC27 | 4600 | normalized live input uses the Section 55.9 FIFO/current-master-tick scheduling rule for both local and Telnet sources; | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/input/wzsn-input-ownership-v1.json` | `certification/results/core/C55.AC27.json` |
| C55.AC28 | 4602 | bounded command parsing recovers deterministically after malformed/overlong input; | `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt` | `certification/results/core/C55.AC28.json` |
| C55.AC29 | 4604 | non-keyboard application-control requests are dispatched through the shared application command registry rather than implemented privately in network code; | `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt`; `certification/manifests/command-authority.json` | `certification/results/core/C55.AC29.json` |
| C55.AC30 | 4607 | the initial transport contains no application-level authentication or encryption and the UI/documentation visibly states that security boundary; | `warajevo-zx-spectrum-next-ui-architecture.md`; `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt` | `certification/results/core/C55.AC30.json` |
| C55.AC31 | 4609 | every Telnet application-control, menu-projection, screenshot, response, and remote-permission requirement in `design/warajevo-zx-spectrum-next-ui-architecture.md` also passes before Phase 15 exits. | `certification/manifests/ui-section48-acceptance.json`; `warajevo-zx-spectrum-next-ui-architecture.md` | `certification/results/core/C55.AC31.json` |

# Part II - Architecture #2: UI correctness tests

## U01. Architecture section 1: UI mission

**Architecture authority:** `U` source lines 38-66, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** UI mission / capability preservation.

**Required artifacts:**

- `tests/fixtures/ui/U01-capability-preservation.json`

**Artifact/source authority:** Project-owned UI workflow fixture derived from Architecture #2 Sections 1 and 20.

**Tests:**

### U01.01

Walk every REQUIRED/REPLACE capability and prove it remains reachable through the modern workflow without reproducing obsolete 1998 navigation; verify ordinary run/open/reset/pause remains unobstructed.

**Evidence output:** `certification/results/ui/U01.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U02. Architecture section 2: Sources and historical authority

**Architecture authority:** `U` source lines 67-101, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Historical authority and disposition.

**Required artifacts:**

- `tests/fixtures/ui/U02-legacy-source-ledger.json`
- `tests/fixtures/ui/U20-legacy-disposition.json`
- `tests/fixtures/ui/U20-legacy-prose-rules.json`

**Artifact/source authority:** Generated from preserved Warajevo 2.50 source/menu inventory plus Architecture #2 Section 20.

**Tests:**

### U02.01

Verify every legacy Warajevo UI command/facility is classified exactly once against Section 20 and that no historical behavior silently acquires authority over hardware/core semantics.

**Evidence output:** `certification/results/ui/U02.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U03. Architecture section 3: Non-negotiable UI principles

**Architecture authority:** `U` source lines 102-167, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Non-negotiable UI principles.

**Required artifacts:**

- `tests/fixtures/ui/U03-ui-principles.json`
- `tests/fixtures/ui/U03-primary-workflow.json`
- `tests/fixtures/ui/U03-format-placement.json`

**Artifact/source authority:** Project-owned invariant/workflow tests copied from the explicit Section-3 principles.

**Tests:**

### U03.01

Assert running machine is primary surface; no top-level format-centric navigation; one semantic command per operation; presentation changes never mutate core timing; advanced tools do not block normal use; every disabled command exposes a stable reason token.

**Evidence output:** `certification/results/ui/U03.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### U03.02

Using `tests/fixtures/ui/U03-primary-workflow.json`, prove that a newly launched WZSN presents the running Spectrum as the primary surface and permits the ordinary workflows named by Section 3.1—see the emulated machine, insert media, reset, change emulation speed, pause/resume, save/load state, and inspect status—without entering a separate historical environment or launching a second emulator executable.

**Evidence output:** `certification/results/ui/U03.02.json`

**Pass rule:** PASS only when every named ordinary workflow is reachable from the continuously running application through the shared application/orchestrator semantics.

### U03.03

Using `tests/fixtures/ui/U03-format-placement.json`, prove that `Z80`, `SNA`, `TAP`, `TZX`, `MDR`, `DCK`, `SLT`, `SIT`, `SNP`, and other file-format names do not become independent top-level navigation merely because they are formats. Format-specific detail may appear only in the relevant media manager, Compatibility Tool, file workflow, or diagnostic surface. Native formats must not be forced through a conversion UI solely to preserve historical navigation.

**Evidence output:** `certification/results/ui/U03.03.json`

**Pass rule:** PASS only when navigation is organized by user intent exactly as Section 3.2 requires and presentation does not create a second semantic implementation.

## U04. Architecture section 4: Shared application command registry

**Architecture authority:** `U` source lines 168-314, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Shared application command registry.

**Required artifacts:**

- `tests/fixtures/ui/U04-command-registry.json`
- `tests/fixtures/ui/U04-fixed-action-bindings.json`
- `tests/fixtures/ui/U04-dynamic-registry-data.json`

**Artifact/source authority:** Project-generated registry snapshot from the compiled application.

**Tests:**

### U04.01

Validate stable lowercase dotted IDs, uniqueness, complete required metadata, parameter/result schemas, availability predicate/reason, one initial permission class, handler identity, machine-state/recordability flags, and legal parameter acquisition per frontend.

**Evidence output:** `certification/results/ui/U04.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### U04.02

For each multi-frontend command, invoke GUI/menu/toolbar/Telnet/test projection and prove all permitted paths reach the same semantic handler and same machine-visible result.

**Evidence output:** `certification/results/ui/U04.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### U04.03

Using `tests/fixtures/ui/U04-fixed-action-bindings.json`, prove fixed-argument menu action IDs are stable registry actions bound to the parameterized semantic command, including at minimum `machine.speed.400` -> `machine.speed.set 400`, the 48K/128K model choices -> `machine.model.set`, and Normal/Instant tape-loading choices -> the shared tape-loading-mode command. For each binding, `DO <fixed-action-id>` and `DO <semantic-command> <fixed-argument>` must reach the same handler and produce the same result when permitted.

**Evidence output:** `certification/results/ui/U04.03.json`

**Pass rule:** PASS only when every frozen fixed-argument binding is exact and equivalent through the shared registry.

### U04.04

Using `tests/fixtures/ui/U04-dynamic-registry-data.json`, populate Recent files with distinguishable absolute host paths and prove those dynamic entries remain host/UI data rather than globally stable registry IDs: they must not create path-bearing stable command IDs, must not appear as stable actions in `MENU TREE`, and must not change the canonical registry-ID set.

**Evidence output:** `certification/results/ui/U04.04.json`

**Pass rule:** PASS only when dynamic Recent data cannot become a stable application API or a path-disclosure mechanism.

## U05. Architecture section 5: Remote permission classes

**Architecture authority:** `U` source lines 315-405, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Remote permission classes.

**Required artifacts:**

- `tests/fixtures/ui/U05-remote-permissions.json`
- `tests/fixtures/ui/U05-permission-negative-cases.json`
- `tests/fixtures/ui/U05-local-only-policy.json`

**Artifact/source authority:** Project-generated registry/policy matrix.

**Tests:**

### U05.01

Assert every command has exactly one of REMOTE_SAFE/HOST_READ/HOST_WRITE/MEDIA_DESTRUCTIVE/APPLICATION_CONTROL/LOCAL_ONLY; initial unauthenticated Telnet executes only REMOTE_SAFE while all other commands remain discoverable; denied commands have zero side effects.

**Evidence output:** `certification/results/ui/U05.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### U05.02

Using `tests/fixtures/ui/U05-permission-negative-cases.json`, exercise one or more commands from every class and prove the frozen class semantics: REMOTE_SAFE may affect the emulated machine but cannot perform arbitrary host reads/writes, destructive media mutation, WZSN termination/restart, or local-OS actions; HOST_READ and HOST_WRITE are denied; MEDIA_DESTRUCTIVE is denied; APPLICATION_CONTROL is denied; LOCAL_ONLY is denied. Include hostile/misclassified-parameter cases so a REMOTE_SAFE command cannot smuggle a host path or destructive operation through generic arguments.

**Evidence output:** `certification/results/ui/U05.02.json`

**Pass rule:** PASS only when permission is enforced by semantic capability, not cosmetic command naming.

### U05.03

Using `tests/fixtures/ui/U05-local-only-policy.json`, attempt every available settings/configuration path that could alter initial Telnet permissions and prove LOCAL_ONLY remains denied and is not user-overridable without an explicit later security-architecture revision. Also prove denied/disabled commands remain visible to `MENU`, `MENU TREE`, `MENU FIND`, and `DESCRIBE`; visibility never grants execution permission.

**Evidence output:** `certification/results/ui/U05.03.json`

**Pass rule:** PASS only when the initial local-only policy cannot be relaxed by ordinary preferences or command-registry data.

## U06. Architecture section 6: Global application layout

**Architecture authority:** `U` source lines 406-430, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Global application layout.

**Required artifacts:**

- `tests/fixtures/ui/U06-layout.json`

**Artifact/source authority:** Project UI automation fixture.

**Tests:**

### U06.01

Verify central running-machine presentation, canonical menu/toolbar/status composition, optional subordinate panels/managers, and absence of any layout dependency that changes machine semantics.

**Evidence output:** `certification/results/ui/U06.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U07. Architecture section 7: Canonical top-level menu tree

**Architecture authority:** `U` source lines 431-579, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Canonical top-level menu tree and IDs.

**Required artifacts:**

- `tests/fixtures/ui/U07-menu-tree.json`
- `tests/fixtures/ui/U07-required-command-ids.json`
- `tests/fixtures/ui/U07-fixed-action-bindings.json`
- `tests/fixtures/ui/U07-state-sensitive-actions.json`
- `tests/fixtures/ui/U07-help-relocation.json`

**Artifact/source authority:** Project-generated frozen menu-tree/command-binding manifests copied from Architecture #2 Section 7 and cross-checked against the shared command-registry contract.

**Tests:**

### U07.01

Compare the live menu/command tree against every required Section-7.1 node/action/non-menu ID exactly; reject missing, duplicate, renamed, or wrong-parent entries.

**Evidence output:** `certification/results/ui/U07.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### U07.02

Compare the live top level to the exact semantic set `File`, `Machine`, `Media`, `View`, `Tools`, `Settings`, `Help`. Reject missing or extra semantic top-level menus and specifically reject resurrection of `TapeFiles`, `Z80Snaps`, `MdriveFiles`, `DockFiles`, `DataBase`, `Setup`, or `DOS` as top-level navigation. Platform-native relocation may change presentation placement only where Section 6/7 expressly allows it; it may not change the canonical semantic tree.

**Evidence output:** `certification/results/ui/U07.02.json`

**Pass rule:** PASS only when the canonical seven-node semantic top level and the explicit legacy-menu exclusions are exact.

### U07.03

Using `U07-fixed-action-bindings.json` and `U07-state-sensitive-actions.json`, verify every fixed-choice menu action in Section 7 binds to the correct parameterized semantic command and fixed argument, including the literal bindings `machine.model.48k` -> `machine.model.set 48k`, `machine.model.128k` -> `machine.model.set 128k`, every `machine.speed.<choice>` -> `machine.speed.set <choice>`, `media.tape.loading_mode.normal` -> `media.tape.loading_mode.set normal`, and `media.tape.loading_mode.instant` -> `media.tape.loading_mode.set instant`. Verify `machine.pause_resume` invokes `machine.pause` while running and `machine.resume` while paused, without becoming a third private pause implementation; explicit Telnet automation remains represented by the non-toggle `PAUSE` and `RESUME` aliases. Verify `file.recent` remains a dynamic local presentation group whose entries call `file.open_run` with the stored local path and whose host paths/path-derived labels are absent from remotely discoverable registry payloads.

**Evidence output:** `certification/results/ui/U07.03.json`

**Pass rule:** PASS only when Section-7 menu-node semantics, fixed bindings, state-sensitive binding, and Recent-data boundary are preserved exactly.

### U07.04

Using `U07-help-relocation.json`, verify the canonical Help subtree is `Help -> Help` (`help.help`) and `About Warajevo ZX Spectrum Next` (`help.about`). On a platform that relocates About to its standard native location, prove the same stable command ID/handler remains in the canonical registry/menu hierarchy and only native presentation placement changes.

**Evidence output:** `certification/results/ui/U07.04.json`

**Pass rule:** PASS only when the Help labels/IDs are exact and platform relocation cannot rename, duplicate, or replace their semantic commands.

## U08. Architecture section 8: File menu

**Architecture authority:** `U` source lines 580-678, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** File menu workflows.

**Required artifacts:**

- `tests/fixtures/tape/wzsn-tap-standard-oneblock.tap`
- `tests/fixtures/tzx/wzsn-tzx-standard-speed.tzx`
- `tests/fixtures/tape/wzsn-tape-44100-mono.wav`
- `tests/fixtures/snapshot/wzsn-sna-48k-canonical.sna`
- `tests/fixtures/snapshot/wzsn-z80-v2-48k.z80`
- `tests/fixtures/mdr/wzsn-mdr-minimal.mdr`
- `tests/fixtures/ui/U08-file-workflows.json`
- `tests/fixtures/ui/U08-recent-missing.json`
- `tests/fixtures/ui/U08-snapshot-destination.json`
- `tests/fixtures/ui/U08-screenshot-crop.json`

**Artifact/source authority:** Project-owned fixtures; formats grounded by Core Phase-7/8 matrices.

**Tests:**

### U08.01

Open/Run each exact native artifact and prove routing: TAP/TZX/WAV->Tape, SNA/Z80->snapshot load, MDR->Microdrive mount; cancel chooser leaves state unchanged; malformed/unsupported input reports controlled error without lossy silent conversion.

**Evidence output:** `certification/results/ui/U08.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### U08.02

Using `tests/fixtures/ui/U08-recent-missing.json`, prove Recent is host-only convenience metadata, never deterministic machine state; `MENU`/`MENU TREE` may expose the `file.recent` group but never recent absolute paths or path-derived labels; a missing recent file is reported cleanly and may be removed from the list without changing canonical machine state.

**Evidence output:** `certification/results/ui/U08.02.json`

**Pass rule:** PASS only when recent-file convenience state is isolated from emulation and unauthenticated discovery cannot disclose recent paths.

### U08.03

Using `tests/fixtures/ui/U08-snapshot-destination.json` plus the exact SNA/Z80 fixtures above, prove `Load Snapshot...`, `Save Snapshot...`, and `Save Snapshot As...` are direct front ends to the shared snapshot subsystem; failed load is atomic; Save uses the current snapshot destination when one exists, otherwise invokes the same GUI parameter-acquisition path as Save As; Save As always obtains a new destination; no snapshot command creates persistent mounted-media state.

**Evidence output:** `certification/results/ui/U08.03.json`

**Pass rule:** PASS only when all Section-8.3 destination and atomicity semantics are exact.

### U08.04

Using `tests/fixtures/ui/U08-screenshot-crop.json`, invoke `host.screenshot.save` through the GUI chooser and prove PNG output uses the shared core screenshot capture service and contains only the Spectrum raster using the selected host-visible crop/border presentation, with no menus, toolbar, status UI, debugger chrome, cursor, or desktop content. Cancel and write-failure paths must not alter machine state.

**Evidence output:** `certification/results/ui/U08.04.json`

**Pass rule:** PASS only when screenshot capture and crop/border semantics match Section 8.4 exactly.

### U08.05

Invoke `application.quit` locally and through initial unauthenticated Telnet. Prove the local action terminates the host application through the shared command while the remote attempt is denied by its non-REMOTE_SAFE permission class and cannot terminate or mutate WZSN.

**Evidence output:** `certification/results/ui/U08.05.json`

**Pass rule:** PASS only when Quit is local host application control and is not remotely executable in the initial policy.

## U09. Architecture section 9: Machine menu

**Architecture authority:** `U` source lines 679-789, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Machine menu.

**Required artifacts:**

- `tests/fixtures/ui/U09-machine-workflows.json`
- `tests/fixtures/ui/U09-pause-master-time.json`
- `tests/fixtures/ui/U09-reset-preservation.json`
- `tests/fixtures/ui/U09-recorded-command-equivalence.json`

**Artifact/source authority:** Project UI automation plus Core deterministic state hashes.

**Tests:**

### U09.01

Test the Model menu exposes only the initially certified 48K and 128K choices, routes both through `machine.model.set`, and performs the defined machine-change/cold-profile workflow rather than hot-swapping incompatible state. Later/unsupported profiles must not appear as selectable initial options.

**Evidence output:** `certification/results/ui/U09.01.json`

**Pass rule:** PASS only when the initial model surface and machine-change semantics are exact.

### U09.02

Using `tests/fixtures/ui/U09-reset-preservation.json`, invoke `machine.reset` while running and while application-paused. Prove authentic profile reset occurs without restarting WZSN, disconnecting Telnet, changing speed/preferences, implicitly ejecting media except where authentic hardware semantics require a state change, or clearing local/Telnet key ownership beyond the Core reset policy; when paused the application remains paused after reset.

**Evidence output:** `certification/results/ui/U09.02.json`

**Pass rule:** PASS only when every Section-9.2 preservation/non-effect rule is demonstrated.

### U09.03

Using `tests/fixtures/ui/U09-pause-master-time.json`, prove `machine.pause`/`machine.resume` are idempotent application-execution controls, not Spectrum signals, and canonical master time does not advance for any duration of application pause.

**Evidence output:** `certification/results/ui/U09.03.json`

**Pass rule:** PASS only when pause freezes execution/master time without mutating Spectrum hardware state.

### U09.04

Exercise exactly 25/50/100/200/400/800/Unlimited through `machine.speed.set`; prove only host pacing changes, no machine reset or master-tick discontinuity occurs, UI reflects host audio audible only 50%..200% inclusive and muted outside that range, and emulated beeper/AY state continues while host-muted.

**Evidence output:** `certification/results/ui/U09.04.json`

**Pass rule:** PASS only when every frozen speed/audio-display rule matches Core semantics.

### U09.05

Using `tests/fixtures/ui/U09-recorded-command-equivalence.json`, invoke reset/model/pause/resume/speed through every permitted front end and through deterministic application/session recording/replay where applicable. Prove the same semantic command and machine-visible result are used regardless of front end.

**Evidence output:** `certification/results/ui/U09.05.json`

**Pass rule:** PASS only when recordable deterministic machine-control commands remain front-end equivalent.

## U10. Architecture section 10: Media menu

**Architecture authority:** `U` source lines 790-821, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Media menu.

**Required artifacts:**

- `tests/fixtures/ui/U10-media-menu.json`

**Artifact/source authority:** Project menu-tree fixture.

**Tests:**

### U10.01

Verify the exact initial Media tree: Tape contains Insert/Eject/Loading Mode Normal+Instant/Trap/Tape Manager; Microdrive contains Drive 1 through Drive 8 plus Microdrive Manager; ZX Printer contains Printer Manager; Dock Cartridge is absent until Timex/DCK support is promoted. Every action binds its Section-7 stable registry ID and no format-specific historical top-level menu reappears.

**Evidence output:** `certification/results/ui/U10.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U11. Architecture section 11: Tape quick controls

**Architecture authority:** `U` source lines 822-872, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Tape quick controls.

**Required artifacts:**

- `tests/fixtures/tape/wzsn-tap-standard-oneblock.tap`
- `tests/fixtures/ui/U11-tape-quick.json`
- `tests/fixtures/ui/U11-no-tape-state.json`
- `tests/fixtures/ui/U11-toolbar-tape-menu.json`

**Artifact/source authority:** Project-owned TAP and UI automation.

**Tests:**

### U11.01

Verify `media.tape.insert <path>` uses GUI parameter acquisition and is optionally exposed by the toolbar; `media.tape.eject` is disabled with the exact registry reason `no-tape-mounted` when empty and works when mounted; `media.tape.loading_mode.set <normal|instant>` shows Normal as the default authenticity-first mode and Instant/Trap as an explicit optional mode, never a hidden optimization. Verify the compact toolbar Tape control reports mounted/unmounted state and offers at least Insert, Eject, Normal, Instant/Trap, and Open Tape Manager, all through the same registry commands.

**Evidence output:** `certification/results/ui/U11.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U12. Architecture section 12: Tape Manager

**Architecture authority:** `U` source lines 873-938, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Tape Manager.

**Required artifacts:**

- `tests/fixtures/tzx/wzsn-tzx-control-flow.tzx`
- `tests/fixtures/tzx/wzsn-tzx-malformed.tzx`
- `tests/fixtures/tape/wzsn-warajevo-native-minimal.tap`
- `tests/fixtures/tape/wzsn-warajevo-native-edge.tap`
- `tests/fixtures/ui/U12-tape-manager.json`

**Artifact/source authority:** Project-owned TZX fixtures designed from the frozen TZX block matrix and project-owned Warajevo-native TAP fixtures defined by the Core media proof. No private third-party media is required for this UI contract.

**Tests:**

### U12.01

For a mounted tape, assert manager presentation exposes source identity/path, format, loading mode, current block/position, play/loading state, block list/type, logical/stored lengths where meaningful, flags/metadata where meaningful, and selection state; this view consumes shared tape state and is not a second emulator.

**Evidence output:** `certification/results/ui/U12.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### U12.02

Exercise reorder, extract selected, add/import, delete, edit data where permitted, copy selected to new tape, and change position. All state/availability comes from shared tape/media services; drag/drop, if present, is only presentation.

**Evidence output:** `certification/results/ui/U12.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### U12.03

With exact project-owned fixtures `tests/fixtures/tape/wzsn-warajevo-native-minimal.tap` and `tests/fixtures/tape/wzsn-warajevo-native-edge.tap`, expose exclude/linearize/implode/decompress/compression-efficiency only under Advanced Warajevo Tape/Compatibility Tools and never as ordinary TAP/TZX operations.

**Evidence output:** `certification/results/ui/U12.03.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### U12.04

Verify legacy Print-to-Screen is replaced by manager view, legacy Print-to-Printer is absent, and any retained `Export Tape Report...` is non-destructive and atomically written.

**Evidence output:** `certification/results/ui/U12.04.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### U12.05

Unsupported mutation, malformed block, read-only media, cancellation and save/write failures leave the original image byte-identical unless a complete atomic replacement has committed.

**Evidence output:** `certification/results/ui/U12.05.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U13. Architecture section 13: Snapshot workflow and Snapshot Inspector

**Architecture authority:** `U` source lines 939-989, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Snapshot workflow and Snapshot Inspector.

**Required artifacts:**

- `tests/fixtures/snapshot/wzsn-sna-48k-canonical.sna`
- `tests/fixtures/snapshot/wzsn-sna-malformed-truncated.sna`
- `tests/fixtures/snapshot/wzsn-z80-v2-128k.z80`
- `tests/fixtures/ui/U13-snapshot-inspector.json`
- `tests/fixtures/ui/U13-snapshot-editor-reuse.json`

**Artifact/source authority:** Project-owned snapshot fixtures from frozen SNA/Z80 matrices.

**Tests:**

### U13.01

Prove snapshot is a load/save artifact and never a continuously mounted medium: no Select/Unselect snapshot state exists. Exercise Load/Save/Save As through the shared snapshot service and verify malformed load is atomic. Open `Tools > Snapshot Inspector...` via stable ID `tools.snapshot_inspector`; for the exact SNA/Z80 fixtures verify format/version, implied machine profile, register summary, paging/port state, AY state where applicable, memory-page inventory, and warnings/unsupported extensions. The inspector must not mutate the live machine. Using `U13-snapshot-editor-reuse.json`, prove offline register/hardware/memory editing, if exposed, reuses the same debugger/state-editor components and semantics rather than a second independent implementation.

**Evidence output:** `certification/results/ui/U13.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U14. Architecture section 14: Microdrive quick controls

**Architecture authority:** `U` source lines 990-1015, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Microdrive quick controls.

**Required artifacts:**

- `tests/fixtures/mdr/wzsn-mdr-minimal.mdr`
- `tests/fixtures/ui/U14-microdrive-quick.json`
- `tests/fixtures/ui/U14-microdrive-status.json`

**Artifact/source authority:** Project MDR fixture and interprocess mount harness.

**Tests:**

### U14.01

For drives 1..8 test `media.microdrive.mount`, `media.microdrive.eject`, and `media.microdrive.set_default`, with independent semantic mounted/unmounted state per slot. Verify the toolbar initially exposes compact `MDV 1` control while the status panel and Microdrive Manager expose all eight slots. Prevent the same writable resolved image from being mounted twice within one process or across processes unless the second access is explicitly read-only under an approved policy.

**Evidence output:** `certification/results/ui/U14.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U15. Architecture section 15: Microdrive Manager

**Architecture authority:** `U` source lines 1016-1084, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Microdrive Manager.

**Required artifacts:**

- `tests/fixtures/mdr/wzsn-mdr-multifile.mdr`
- `tests/fixtures/mdr/wzsn-mdr-write-protect.mdr`
- `tests/fixtures/ui/U15-microdrive-manager.json`
- `tests/fixtures/ui/U15-drive-overview.json`
- `tests/fixtures/ui/U15-advanced-sectors.json`

**Artifact/source authority:** Project-owned MDR fixtures generated from the frozen Interface-1/MDR specification.

**Tests:**

### U15.01

Using `U15-drive-overview.json`, prove all eight slots are displayed and each mounted cartridge shows host image identity, logical cartridge name, sector count, write-protection state, current/default-drive state, and format/validation status. Exercise every whole-cartridge operation: mount/select, eject/unselect, set default, catalog, format, optimize/reorder sectors, view sector allocation, logical rename, write protect, and write unprotect. Exercise logical-file delete/rename/hide/unhide/copy to another mounted cartridge. Using `U15-advanced-sectors.json`, exercise verify/repair logical structure, data-only sector editing, and explicitly permitted whole-sector metadata/checksum editing, with ordinary versus dangerous raw editing visibly distinguished. Destructive operations require explicit confirmation and `MEDIA_DESTRUCTIVE` classification; failures are atomic. Verify `Enlarge MDR to 254 sectors` appears only as a Compatibility Tool, not a primary manager command.

**Evidence output:** `certification/results/ui/U15.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U16. Architecture section 16: ZX Printer UI

**Architecture authority:** `U` source lines 1085-1101, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** ZX Printer UI.

**Required artifacts:**

- `tests/fixtures/printer/wzsn-zxprinter-smoke.tap`
- `tests/fixtures/ui/U16-printer-manager.json`

**Artifact/source authority:** Project-owned ZX Printer stimulus TAP and UI automation.

**Tests:**

### U16.01

Verify authentic ZX Printer Manager is reachable only through `Media > ZX Printer > Printer Manager...` and peripheral settings through `Settings > Peripherals > ZX Printer`; both bind the single Core printer state.

**Evidence output:** `certification/results/ui/U16.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### U16.02

Capture/export logical virtual printer output where implemented and prove host export does not define Spectrum printer timing; no historical LPT/printer-port routing is required or falsely advertised.

**Evidence output:** `certification/results/ui/U16.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U17. Architecture section 17: Later Dock Cartridge UI

**Architecture authority:** `U` source lines 1102-1122, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Later Dock Cartridge UI.

**Required artifacts:**

- `tests/fixtures/ui/U17-dock-absence.json`

**Artifact/source authority:** Project menu/registry snapshot.

**Tests:**

### U17.01

Prove Dock UI is absent from initial build/backlog surfaces unless Timex/DCK support is actually implemented and promoted; no inert placeholder command masquerades as support.

**Evidence output:** `certification/results/ui/U17.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U18. Architecture section 18: View menu

**Architecture authority:** `U` source lines 1123-1149, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** View menu.

**Required artifacts:**

- `tests/fixtures/ui/U18-view.json`
- `tests/fixtures/ui/U18-display-settings-store.json`

**Artifact/source authority:** Project UI automation plus core checkpoint hashes.

**Tests:**

### U18.01

Verify the exact View menu contains Fullscreen, Machine / Media Status, and Display Settings. Enter/leave fullscreen and toggle the status panel without changing canonical machine state or master time. Using `U18-display-settings-store.json`, invoke View > Display Settings and Settings > Display and prove both target the same settings category/store/semantic commands; the View shortcut must not create a duplicate configuration store.

**Evidence output:** `certification/results/ui/U18.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U19. Architecture section 19: Tools menu

**Architecture authority:** `U` source lines 1150-1172, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Tools menu.

**Required artifacts:**

- `tests/fixtures/ui/U19-tools.json`
- `tests/fixtures/ui/U19-required-vs-compatibility.json`

**Artifact/source authority:** Project menu/registry snapshot.

**Tests:**

### U19.01

Verify the exact Tools tree contains Debugger/Monitor, Diagnostics, Snapshot Inspector, and Compatibility Tools with Tape Converter, Snapshot Converter, Spectrum Data Converter, Microdrive Tools, and Legacy Database Converter entries as implemented/allowed. Unimplemented retained compatibility utilities may be hidden or honestly marked unavailable. Using `U19-required-vs-compatibility.json`, prove every Architecture-#1 REQUIRED emulation feature remains reachable through its proper primary UI and is never hidden behind Compatibility Tools merely to avoid implementing it.

**Evidence output:** `certification/results/ui/U19.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U20. Architecture section 20: Complete legacy-menu disposition

**Architecture authority:** `U` source lines 1173-1494, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Complete legacy-menu disposition.

**Required artifacts:**

- `tests/fixtures/ui/U20-legacy-disposition.json`
- `tests/fixtures/ui/U20-legacy-prose-rules.json`
- `certification/manifests/ui-command-registry-v1.json`
- `wzsn-architectures-1-2-developer-tasks.md`

**Artifact/source authority:** Manually transcribed Section-20 ledger checked against preserved Warajevo 2.50 menu inventory, the canonical shared-command registry manifest, the Architecture-1/2 developer backlog for deferred/LATER work, and the exact owning UI/media proof fixtures named per row below.

**Tests:**

### U20.01

For every row of Architecture #2 Section 20, assert exactly one modern disposition and destination; verify DROP items absent, LATER items not falsely active, REQUIRED/REPLACE items reachable, and TIFF/legacy conversions remain subordinate to ordinary PNG workflow.

**Evidence output:** `certification/results/ui/U20.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### U20.02

Using `tests/fixtures/ui/U20-legacy-prose-rules.json`, prove every normative non-table rule in Section 20: authentic Interface-1 serial state remains a machine/peripheral concern rather than resurrected PC RS232 bridging; any future real-hardware bridge requires a new architecture; later live cassette capture is a new Tape Capture workflow rather than the DOS dialog; SNA/Z80 remain native formats independent of Compatibility Tools; the entire Dock branch is absent until Timex/DCK is implemented; a future Library is not a direct port of the historical database schema; historical Fast BASIC/inverse-cursor ROM patch controls are absent from normal initial settings; and historical PC-speaker/AdLib/SoundBlaster, VGA/CGA/Hercules, and hand-tuned ULA-delay host compensations are not exposed as emulated-hardware controls.

**Required artifact:** `tests/fixtures/ui/U20-legacy-prose-rules.json`

**Evidence output:** `certification/results/ui/U20.02.json`

**Pass rule:** PASS only when every Section-20 prose-level disposition is represented exactly and no dropped host-era behavior reappears under another label.

### U20 literal legacy-menu disposition ledger

These are **disposition conformance tests**, not claims that every retained converter is already implemented. A retained/LATER converter that is not yet implemented passes only when its registry/backlog disposition matches Architecture #2 and it is not falsely advertised as available. When a converter is promoted to implemented, its algorithmic correctness is certified under U22 with concrete format fixtures.

| Subtest | Source line | Architecture subsection | Legacy item/conversion | WZSN destination | Initial disposition | Primary proof test(s) | Required proof artifact(s) | Evidence artifact |
|---|---:|---|---|---|---|---|---|---|
| U20.D001 | 1182 | 20.1 | Select | Toolbar / quick action | `Insert Tape...` | U11.01; U12.01-U12.05 | `tests/fixtures/ui/U11-tape-quick.json`; `tests/fixtures/ui/U12-tape-manager.json`; `tests/fixtures/tape/wzsn-tap-standard-oneblock.tap` | `certification/results/ui/U20.D001.json` |
| U20.D002 | 1183 | 20.1 | Unselect | Toolbar / quick action | `Eject Tape` | U11.01; U12.01-U12.05 | `tests/fixtures/ui/U11-tape-quick.json`; `tests/fixtures/ui/U12-tape-manager.json`; `tests/fixtures/tape/wzsn-tap-standard-oneblock.tap` | `certification/results/ui/U20.D002.json` |
| U20.D003 | 1184 | 20.1 | Parameters | Media-manager function | Split among Tape Manager position/properties and visible Normal/Instant loading mode | U11.01; U12.01-U12.05 | `tests/fixtures/ui/U11-tape-quick.json`; `tests/fixtures/ui/U12-tape-manager.json`; `tests/fixtures/tape/wzsn-tap-standard-oneblock.tap` | `certification/results/ui/U20.D003.json` |
| U20.D004 | 1185 | 20.1 | View | Media-manager function | Tape block browser | U11.01; U12.01-U12.05 | `tests/fixtures/ui/U11-tape-quick.json`; `tests/fixtures/ui/U12-tape-manager.json`; `tests/fixtures/tape/wzsn-tap-standard-oneblock.tap` | `certification/results/ui/U20.D004.json` |
| U20.D005 | 1186 | 20.1 | Print > Screen | Dropped legacy host function | Replaced by Tape Manager view | U11.01; U12.01-U12.05 | `tests/fixtures/ui/U11-tape-quick.json`; `tests/fixtures/ui/U12-tape-manager.json`; `tests/fixtures/tape/wzsn-tap-standard-oneblock.tap` | `certification/results/ui/U20.D005.json` |
| U20.D006 | 1187 | 20.1 | Print > Printer | Dropped legacy host function | Not reproduced | U11.01; U12.01-U12.05 | `tests/fixtures/ui/U11-tape-quick.json`; `tests/fixtures/ui/U12-tape-manager.json`; `tests/fixtures/tape/wzsn-tap-standard-oneblock.tap` | `certification/results/ui/U20.D006.json` |
| U20.D007 | 1188 | 20.1 | Print > File | Media-manager function | Optional `Export Tape Report...` | U11.01; U12.01-U12.05 | `tests/fixtures/ui/U11-tape-quick.json`; `tests/fixtures/ui/U12-tape-manager.json`; `tests/fixtures/tape/wzsn-tap-standard-oneblock.tap` | `certification/results/ui/U20.D007.json` |
| U20.D008 | 1189 | 20.1 | Copy to New | Media-manager function | Copy selected blocks into a new image | U11.01; U12.01-U12.05 | `tests/fixtures/ui/U11-tape-quick.json`; `tests/fixtures/ui/U12-tape-manager.json`; `tests/fixtures/tape/wzsn-tap-standard-oneblock.tap` | `certification/results/ui/U20.D008.json` |
| U20.D009 | 1195 | 20.2 | Reorder | Media-manager function | Retain | U12.02; U12.05 | `tests/fixtures/ui/U12-tape-manager.json`; `tests/fixtures/tzx/wzsn-tzx-control-flow.tzx`; `tests/fixtures/tzx/wzsn-tzx-malformed.tzx` | `certification/results/ui/U20.D009.json` |
| U20.D010 | 1196 | 20.2 | Extract | Media-manager function | Retain | U12.02; U12.05 | `tests/fixtures/ui/U12-tape-manager.json`; `tests/fixtures/tzx/wzsn-tzx-control-flow.tzx`; `tests/fixtures/tzx/wzsn-tzx-malformed.tzx` | `certification/results/ui/U20.D010.json` |
| U20.D011 | 1197 | 20.2 | Delete | Media-manager function | Retain | U12.02; U12.05 | `tests/fixtures/ui/U12-tape-manager.json`; `tests/fixtures/tzx/wzsn-tzx-control-flow.tzx`; `tests/fixtures/tzx/wzsn-tzx-malformed.tzx` | `certification/results/ui/U20.D011.json` |
| U20.D012 | 1198 | 20.2 | Add | Media-manager function | Retain | U12.02; U12.05 | `tests/fixtures/ui/U12-tape-manager.json`; `tests/fixtures/tzx/wzsn-tzx-control-flow.tzx`; `tests/fixtures/tzx/wzsn-tzx-malformed.tzx` | `certification/results/ui/U20.D012.json` |
| U20.D013 | 1199 | 20.2 | Edit | Media-manager function | Retain where format permits | U12.02; U12.05 | `tests/fixtures/ui/U12-tape-manager.json`; `tests/fixtures/tzx/wzsn-tzx-control-flow.tzx`; `tests/fixtures/tzx/wzsn-tzx-malformed.tzx` | `certification/results/ui/U20.D013.json` |
| U20.D014 | 1200 | 20.2 | Exclude | Compatibility utility | Warajevo-native tape advanced operation | U12.02; U12.05 | `tests/fixtures/ui/U12-tape-manager.json`; `tests/fixtures/tzx/wzsn-tzx-control-flow.tzx`; `tests/fixtures/tzx/wzsn-tzx-malformed.tzx` | `certification/results/ui/U20.D014.json` |
| U20.D015 | 1201 | 20.2 | Change Position | Media-manager function | Retain; drag/drop or explicit move | U12.02; U12.05 | `tests/fixtures/ui/U12-tape-manager.json`; `tests/fixtures/tzx/wzsn-tzx-control-flow.tzx`; `tests/fixtures/tzx/wzsn-tzx-malformed.tzx` | `certification/results/ui/U20.D015.json` |
| U20.D016 | 1207 | 20.3 | Compress > All blocks | Compatibility utility | Retain with native Warajevo tape support | U12.03 | `tests/fixtures/tape/wzsn-warajevo-native-minimal.tap`; `tests/fixtures/tape/wzsn-warajevo-native-edge.tap`; `tests/fixtures/ui/U12-tape-manager.json` | `certification/results/ui/U20.D016.json` |
| U20.D017 | 1208 | 20.3 | Compress > Linearize only | Compatibility utility | Retain with native Warajevo tape support | U12.03 | `tests/fixtures/tape/wzsn-warajevo-native-minimal.tap`; `tests/fixtures/tape/wzsn-warajevo-native-edge.tap`; `tests/fixtures/ui/U12-tape-manager.json` | `certification/results/ui/U20.D017.json` |
| U20.D018 | 1209 | 20.3 | Compress > Compress Selected | Compatibility utility | Retain with native Warajevo tape support | U12.03 | `tests/fixtures/tape/wzsn-warajevo-native-minimal.tap`; `tests/fixtures/tape/wzsn-warajevo-native-edge.tap`; `tests/fixtures/ui/U12-tape-manager.json` | `certification/results/ui/U20.D018.json` |
| U20.D019 | 1210 | 20.3 | Decompress | Compatibility utility | Retain with native Warajevo tape support | U12.03 | `tests/fixtures/tape/wzsn-warajevo-native-minimal.tap`; `tests/fixtures/tape/wzsn-warajevo-native-edge.tap`; `tests/fixtures/ui/U12-tape-manager.json` | `certification/results/ui/U20.D019.json` |
| U20.D020 | 1211 | 20.3 | Efficiency | Compatibility utility | Retain as analysis/maintenance information | U12.03 | `tests/fixtures/tape/wzsn-warajevo-native-minimal.tap`; `tests/fixtures/tape/wzsn-warajevo-native-edge.tap`; `tests/fixtures/ui/U12-tape-manager.json` | `certification/results/ui/U20.D020.json` |
| U20.D021 | 1217 | 20.4 | Send to Spectrum | Dropped legacy host function | Old PC-to-real-Spectrum bridge, not Interface 1 emulation | U20.02; C24.05 | `tests/fixtures/ui/U20-legacy-prose-rules.json`; `tests/fixtures/interface1/wzsn-if1-serial-state-v1.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/ui/U20.D021.json` |
| U20.D022 | 1218 | 20.4 | Receive from Spectrum | Dropped legacy host function | Old PC-to-real-Spectrum bridge | U20.02; C24.05 | `tests/fixtures/ui/U20-legacy-prose-rules.json`; `tests/fixtures/interface1/wzsn-if1-serial-state-v1.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/ui/U20.D022.json` |
| U20.D023 | 1219 | 20.4 | Send communication program | Dropped legacy host function | DOS-era transfer helper | U20.02; C24.05 | `tests/fixtures/ui/U20-legacy-prose-rules.json`; `tests/fixtures/interface1/wzsn-if1-serial-state-v1.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/ui/U20.D023.json` |
| U20.D024 | 1220 | 20.4 | Configure RS232 | Dropped legacy host function | Old host serial-port configuration | U20.02; C24.05 | `tests/fixtures/ui/U20-legacy-prose-rules.json`; `tests/fixtures/interface1/wzsn-if1-serial-state-v1.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/ui/U20.D024.json` |
| U20.D025 | 1229 | 20.5 | Receive from cassette | Dropped legacy host function initially | Live physical capture is LATER | U20.02; C23.08 | `tests/fixtures/ui/U20-legacy-prose-rules.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/ui/U20.D025.json` |
| U20.D026 | 1230 | 20.5 | Sample from cassette | Dropped legacy host function initially | Live physical capture is LATER | U20.02; C23.08 | `tests/fixtures/ui/U20-legacy-prose-rules.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/ui/U20.D026.json` |
| U20.D027 | 1231 | 20.5 | Advanced setup | Dropped legacy host function initially | Old DOS timing/device calibration not reproduced | U20.02; C23.08 | `tests/fixtures/ui/U20-legacy-prose-rules.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/ui/U20.D027.json` |
| U20.D028 | 1242 | 20.6 | ASCII -> BASIC | Compatibility utility | Retain as source-language/text compatibility conversion; not a main-menu item | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D028.json` |
| U20.D029 | 1243 | 20.6 | BASIC -> ASCII | Compatibility utility | Retain as source-language/text compatibility conversion; not a main-menu item | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D029.json` |
| U20.D030 | 1244 | 20.6 | ASCII -> HiSoft GENS | Compatibility utility | Retain as source-language/text compatibility conversion; not a main-menu item | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D030.json` |
| U20.D031 | 1245 | 20.6 | HiSoft GENS -> ASCII | Compatibility utility | Retain as source-language/text compatibility conversion; not a main-menu item | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D031.json` |
| U20.D032 | 1246 | 20.6 | ASCII -> HiSoft C | Compatibility utility | Retain as source-language/text compatibility conversion; not a main-menu item | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D032.json` |
| U20.D033 | 1247 | 20.6 | HiSoft C -> ASCII | Compatibility utility | Retain as source-language/text compatibility conversion; not a main-menu item | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D033.json` |
| U20.D034 | 1248 | 20.6 | ASCII -> HiSoft Pascal | Compatibility utility | Retain as source-language/text compatibility conversion; not a main-menu item | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D034.json` |
| U20.D035 | 1249 | 20.6 | HiSoft Pascal -> ASCII | Compatibility utility | Retain as source-language/text compatibility conversion; not a main-menu item | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D035.json` |
| U20.D036 | 1250 | 20.6 | ASCII -> Tassword 2 | Compatibility utility | Retain as source-language/text compatibility conversion; not a main-menu item | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D036.json` |
| U20.D037 | 1251 | 20.6 | Tassword 2 -> ASCII | Compatibility utility | Retain as source-language/text compatibility conversion; not a main-menu item | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D037.json` |
| U20.D038 | 1252 | 20.6 | ASCII -> Tassword 3 | Compatibility utility | Retain as source-language/text compatibility conversion; not a main-menu item | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D038.json` |
| U20.D039 | 1253 | 20.6 | Tassword 3 -> ASCII | Compatibility utility | Retain as source-language/text compatibility conversion; not a main-menu item | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D039.json` |
| U20.D040 | 1254 | 20.6 | ASCII -> The Last Word | Compatibility utility | Retain as source-language/text compatibility conversion; not a main-menu item | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D040.json` |
| U20.D041 | 1255 | 20.6 | The Last Word -> ASCII | Compatibility utility | Retain as source-language/text compatibility conversion; not a main-menu item | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D041.json` |
| U20.D042 | 1256 | 20.6 | ASCII -> Machine Lightning | Compatibility utility | Retain as source-language/text compatibility conversion; not a main-menu item | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D042.json` |
| U20.D043 | 1257 | 20.6 | Machine Lightning -> ASCII | Compatibility utility | Retain as source-language/text compatibility conversion; not a main-menu item | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D043.json` |
| U20.D044 | 1258 | 20.6 | ASCII -> Abersoft Forth | Compatibility utility | Retain as source-language/text compatibility conversion; not a main-menu item | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D044.json` |
| U20.D045 | 1259 | 20.6 | Abersoft Forth -> ASCII | Compatibility utility | Retain as source-language/text compatibility conversion; not a main-menu item | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D045.json` |
| U20.D046 | 1260 | 20.6 | ASCII -> Sinclair Logo | Compatibility utility | Retain as source-language/text compatibility conversion; not a main-menu item | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D046.json` |
| U20.D047 | 1261 | 20.6 | Sinclair Logo -> ASCII | Compatibility utility | Retain as source-language/text compatibility conversion; not a main-menu item | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D047.json` |
| U20.D048 | 1267 | 20.7 | SCREEN$ -> color TIFF | Compatibility utility | Preserve only as legacy conversion; ordinary screenshots use PNG | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D048.json` |
| U20.D049 | 1268 | 20.7 | SCREEN$ -> black/white TIFF | Compatibility utility | Preserve only as legacy conversion | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D049.json` |
| U20.D050 | 1277 | 20.8 | Roman & Easy -> Warajevo | Compatibility utility | Retain as explicit compatibility conversion; native runtime support remains separate | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D050.json` |
| U20.D051 | 1278 | 20.8 | Warajevo -> Roman & Easy | Compatibility utility | Retain as explicit compatibility conversion; native runtime support remains separate | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D051.json` |
| U20.D052 | 1279 | 20.8 | Lunter TAP -> Warajevo TAP | Compatibility utility | Retain as explicit compatibility conversion; native runtime support remains separate | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D052.json` |
| U20.D053 | 1280 | 20.8 | Warajevo TAP -> Lunter TAP | Compatibility utility | Retain as explicit compatibility conversion; native runtime support remains separate | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D053.json` |
| U20.D054 | 1281 | 20.8 | Irish SpecEm -> Warajevo TAP | Compatibility utility | Retain as explicit compatibility conversion; native runtime support remains separate | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D054.json` |
| U20.D055 | 1282 | 20.8 | Warajevo TAP -> Irish SpecEm | Compatibility utility | Retain as explicit compatibility conversion; native runtime support remains separate | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D055.json` |
| U20.D056 | 1283 | 20.8 | Polish SP/SPC -> Warajevo TAP | Compatibility utility | Retain as explicit compatibility conversion; native runtime support remains separate | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D056.json` |
| U20.D057 | 1284 | 20.8 | Warajevo TAP -> Polish SP/SPC | Compatibility utility | Retain as explicit compatibility conversion; native runtime support remains separate | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D057.json` |
| U20.D058 | 1285 | 20.8 | Spectrum 2.00 BLK -> Warajevo TAP | Compatibility utility | Retain as explicit compatibility conversion; native runtime support remains separate | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D058.json` |
| U20.D059 | 1286 | 20.8 | Warajevo TAP -> BLK | Compatibility utility | Retain as explicit compatibility conversion; native runtime support remains separate | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D059.json` |
| U20.D060 | 1287 | 20.8 | ZX Garabik LTP -> Warajevo TAP | Compatibility utility | Retain as explicit compatibility conversion; native runtime support remains separate | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D060.json` |
| U20.D061 | 1288 | 20.8 | Warajevo TAP -> LTP | Compatibility utility | Retain as explicit compatibility conversion; native runtime support remains separate | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D061.json` |
| U20.D062 | 1289 | 20.8 | ZX Brukner -> Warajevo TAP | Compatibility utility | Retain as explicit compatibility conversion; native runtime support remains separate | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D062.json` |
| U20.D063 | 1290 | 20.8 | Warajevo TAP -> ZX Brukner | Compatibility utility | Retain as explicit compatibility conversion; native runtime support remains separate | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D063.json` |
| U20.D064 | 1291 | 20.8 | TZX -> Warajevo TAP | Compatibility utility | Retain as explicit compatibility conversion; native runtime support remains separate | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D064.json` |
| U20.D065 | 1292 | 20.8 | Warajevo TAP -> TZX | Compatibility utility | Retain as explicit compatibility conversion; native runtime support remains separate | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D065.json` |
| U20.D066 | 1293 | 20.8 | TZX conversion setup | Compatibility utility | Retain as explicit compatibility conversion; native runtime support remains separate | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D066.json` |
| U20.D067 | 1294 | 20.8 | ZX Museum ZXS -> Warajevo | Compatibility utility | Retain as explicit compatibility conversion; native runtime support remains separate | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D067.json` |
| U20.D068 | 1295 | 20.8 | Warajevo TAP -> ZX Museum ZXS | Compatibility utility | Retain as explicit compatibility conversion; native runtime support remains separate | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D068.json` |
| U20.D069 | 1296 | 20.8 | TR-DOS TRD -> Warajevo TAP | Compatibility utility | Retain as explicit compatibility conversion; native runtime support remains separate | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D069.json` |
| U20.D070 | 1297 | 20.8 | Warajevo TAP -> TRD | Compatibility utility | Retain as explicit compatibility conversion; native runtime support remains separate | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D070.json` |
| U20.D071 | 1298 | 20.8 | ZX32 ZXT/ZXS -> Warajevo TAP | Compatibility utility | Retain as explicit compatibility conversion; native runtime support remains separate | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D071.json` |
| U20.D072 | 1299 | 20.8 | Warajevo TAP -> ZX32 ZXT/ZXS | Compatibility utility | Retain as explicit compatibility conversion; native runtime support remains separate | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D072.json` |
| U20.D073 | 1300 | 20.8 | VOC -> Warajevo TAP | Compatibility utility | Retain as explicit compatibility conversion; native runtime support remains separate | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D073.json` |
| U20.D074 | 1301 | 20.8 | Warajevo TAP -> VOC | Compatibility utility | Retain as explicit compatibility conversion; native runtime support remains separate | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D074.json` |
| U20.D075 | 1307 | 20.9 | Select | Toolbar / quick action | `Load Snapshot...` | U13.01 | `tests/fixtures/ui/U13-snapshot-inspector.json`; `tests/fixtures/snapshot/wzsn-sna-48k-canonical.sna`; `tests/fixtures/snapshot/wzsn-z80-v2-128k.z80` | `certification/results/ui/U20.D075.json` |
| U20.D076 | 1308 | 20.9 | Unselect | Dropped legacy host function | Snapshot is not persistent mounted media | U13.01 | `tests/fixtures/ui/U13-snapshot-inspector.json`; `tests/fixtures/snapshot/wzsn-sna-48k-canonical.sna`; `tests/fixtures/snapshot/wzsn-z80-v2-128k.z80` | `certification/results/ui/U20.D076.json` |
| U20.D077 | 1309 | 20.9 | Rename | Dropped legacy host function | Replaced by `Save Snapshot As...` | U13.01 | `tests/fixtures/ui/U13-snapshot-inspector.json`; `tests/fixtures/snapshot/wzsn-sna-48k-canonical.sna`; `tests/fixtures/snapshot/wzsn-z80-v2-128k.z80` | `certification/results/ui/U20.D077.json` |
| U20.D078 | 1310 | 20.9 | Info | Media-manager function | Snapshot Inspector | U13.01 | `tests/fixtures/ui/U13-snapshot-inspector.json`; `tests/fixtures/snapshot/wzsn-sna-48k-canonical.sna`; `tests/fixtures/snapshot/wzsn-z80-v2-128k.z80` | `certification/results/ui/U20.D078.json` |
| U20.D079 | 1316 | 20.10 | Processor registers | Debugger tool | Shared register editor | U21.01-U21.03; U13.01 | `tests/fixtures/ui/U21-debugger.json`; `tests/fixtures/debugger/wzsn-debugger-state-v1.json`; `tests/fixtures/ui/U13-snapshot-inspector.json` | `certification/results/ui/U20.D079.json` |
| U20.D080 | 1317 | 20.10 | Hardware devices | Debugger tool | Shared hardware/peripheral state inspector/editor | U21.01-U21.03; U13.01 | `tests/fixtures/ui/U21-debugger.json`; `tests/fixtures/debugger/wzsn-debugger-state-v1.json`; `tests/fixtures/ui/U13-snapshot-inspector.json` | `certification/results/ui/U20.D080.json` |
| U20.D081 | 1318 | 20.10 | Memory | Debugger tool | Shared memory inspector/editor | U21.01-U21.03; U13.01 | `tests/fixtures/ui/U21-debugger.json`; `tests/fixtures/debugger/wzsn-debugger-state-v1.json`; `tests/fixtures/ui/U13-snapshot-inspector.json` | `certification/results/ui/U20.D081.json` |
| U20.D082 | 1326 | 20.11 | Spanish SPECTRUM SP -> Z80 | Compatibility utility | Retain as explicit snapshot compatibility conversion; native SNA/Z80 loading remains separate | U22.01-U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-loss-disclosure-128k-to-48k.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D082.json` |
| U20.D083 | 1327 | 20.11 | Z80 -> Spanish SPECTRUM SP | Compatibility utility | Retain as explicit snapshot compatibility conversion; native SNA/Z80 loading remains separate | U22.01-U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-loss-disclosure-128k-to-48k.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D083.json` |
| U20.D084 | 1328 | 20.11 | old VGASPEC SP -> Z80 | Compatibility utility | Retain as explicit snapshot compatibility conversion; native SNA/Z80 loading remains separate | U22.01-U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-loss-disclosure-128k-to-48k.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D084.json` |
| U20.D085 | 1329 | 20.11 | Z80 -> old VGASPEC SP | Compatibility utility | Retain as explicit snapshot compatibility conversion; native SNA/Z80 loading remains separate | U22.01-U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-loss-disclosure-128k-to-48k.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D085.json` |
| U20.D086 | 1330 | 20.11 | Irish SpecEm PRG -> Z80 | Compatibility utility | Retain as explicit snapshot compatibility conversion; native SNA/Z80 loading remains separate | U22.01-U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-loss-disclosure-128k-to-48k.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D086.json` |
| U20.D087 | 1331 | 20.11 | Z80 -> Irish SpecEm PRG | Compatibility utility | Retain as explicit snapshot compatibility conversion; native SNA/Z80 loading remains separate | U22.01-U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-loss-disclosure-128k-to-48k.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D087.json` |
| U20.D088 | 1332 | 20.11 | JPP SNA -> Z80 | Compatibility utility | Retain as explicit snapshot compatibility conversion; native SNA/Z80 loading remains separate | U22.01-U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-loss-disclosure-128k-to-48k.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D088.json` |
| U20.D089 | 1333 | 20.11 | Z80 -> JPP SNA | Compatibility utility | Retain as explicit snapshot compatibility conversion; native SNA/Z80 loading remains separate | U22.01-U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-loss-disclosure-128k-to-48k.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D089.json` |
| U20.D090 | 1334 | 20.11 | SpecEmu-G SEM -> Z80 | Compatibility utility | Retain as explicit snapshot compatibility conversion; native SNA/Z80 loading remains separate | U22.01-U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-loss-disclosure-128k-to-48k.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D090.json` |
| U20.D091 | 1335 | 20.11 | Z80 -> SpecEmu-G SEM | Compatibility utility | Retain as explicit snapshot compatibility conversion; native SNA/Z80 loading remains separate | U22.01-U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-loss-disclosure-128k-to-48k.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D091.json` |
| U20.D092 | 1336 | 20.11 | SP_UKV SNA 128 -> Z80 | Compatibility utility | Retain as explicit snapshot compatibility conversion; native SNA/Z80 loading remains separate | U22.01-U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-loss-disclosure-128k-to-48k.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D092.json` |
| U20.D093 | 1337 | 20.11 | Z80 -> SNA 128 | Compatibility utility | Retain as explicit snapshot compatibility conversion; native SNA/Z80 loading remains separate | U22.01-U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-loss-disclosure-128k-to-48k.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D093.json` |
| U20.D094 | 1338 | 20.11 | Nuclear ZX SNP -> Z80 | Compatibility utility | Retain as explicit snapshot compatibility conversion; native SNA/Z80 loading remains separate | U22.01-U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-loss-disclosure-128k-to-48k.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D094.json` |
| U20.D095 | 1339 | 20.11 | Z80 -> Nuclear ZX SNP | Compatibility utility | Retain as explicit snapshot compatibility conversion; native SNA/Z80 loading remains separate | U22.01-U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-loss-disclosure-128k-to-48k.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D095.json` |
| U20.D096 | 1340 | 20.11 | X128 SLT -> Z80 + TAP | Compatibility utility | Retain as explicit snapshot compatibility conversion; native SNA/Z80 loading remains separate | U22.01-U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-loss-disclosure-128k-to-48k.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D096.json` |
| U20.D097 | 1341 | 20.11 | Z80 + TAP -> X128 SLT | Compatibility utility | Retain as explicit snapshot compatibility conversion; native SNA/Z80 loading remains separate | U22.01-U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-loss-disclosure-128k-to-48k.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D097.json` |
| U20.D098 | 1342 | 20.11 | Spectrum 2.00 SIT -> Z80 | Compatibility utility | Retain as explicit snapshot compatibility conversion; native SNA/Z80 loading remains separate | U22.01-U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-loss-disclosure-128k-to-48k.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D098.json` |
| U20.D099 | 1343 | 20.11 | Z80 -> Spectrum 2.00 SIT | Compatibility utility | Retain as explicit snapshot compatibility conversion; native SNA/Z80 loading remains separate | U22.01-U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-loss-disclosure-128k-to-48k.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D099.json` |
| U20.D100 | 1344 | 20.11 | ZX32 ZXS RIFF -> Z80 | Compatibility utility | Retain as explicit snapshot compatibility conversion; native SNA/Z80 loading remains separate | U22.01-U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-loss-disclosure-128k-to-48k.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D100.json` |
| U20.D101 | 1345 | 20.11 | Z80 -> ZX32 ZXS RIFF | Compatibility utility | Retain as explicit snapshot compatibility conversion; native SNA/Z80 loading remains separate | U22.01-U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-loss-disclosure-128k-to-48k.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D101.json` |
| U20.D102 | 1346 | 20.11 | Any Z80 -> 48K without Interface 1 | Compatibility utility | Retain as explicit snapshot compatibility conversion; native SNA/Z80 loading remains separate | U22.01-U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-loss-disclosure-128k-to-48k.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D102.json` |
| U20.D103 | 1347 | 20.11 | Any Z80 -> 128K without Interface 1 | Compatibility utility | Retain as explicit snapshot compatibility conversion; native SNA/Z80 loading remains separate | U22.01-U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-loss-disclosure-128k-to-48k.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D103.json` |
| U20.D104 | 1348 | 20.11 | Z80 snapshot -> TAP | Compatibility utility | Retain as explicit snapshot compatibility conversion; native SNA/Z80 loading remains separate | U22.01-U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-loss-disclosure-128k-to-48k.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D104.json` |
| U20.D105 | 1357 | 20.12 | Select | Toolbar / quick action | Mount cartridge in selected drive | U14.01; U15.01 | `tests/fixtures/ui/U14-microdrive-quick.json`; `tests/fixtures/ui/U15-microdrive-manager.json`; `tests/fixtures/mdr/wzsn-mdr-multifile.mdr` | `certification/results/ui/U20.D105.json` |
| U20.D106 | 1358 | 20.12 | Unselect | Toolbar / quick action | Eject cartridge | U14.01; U15.01 | `tests/fixtures/ui/U14-microdrive-quick.json`; `tests/fixtures/ui/U15-microdrive-manager.json`; `tests/fixtures/mdr/wzsn-mdr-multifile.mdr` | `certification/results/ui/U20.D106.json` |
| U20.D107 | 1359 | 20.12 | Set default drive (legacy `Default`) | Media-manager function | Set current/default Microdrive | U14.01; U15.01 | `tests/fixtures/ui/U14-microdrive-quick.json`; `tests/fixtures/ui/U15-microdrive-manager.json`; `tests/fixtures/mdr/wzsn-mdr-multifile.mdr` | `certification/results/ui/U20.D107.json` |
| U20.D108 | 1360 | 20.12 | Catalog | Media-manager function | Logical-file view | U14.01; U15.01 | `tests/fixtures/ui/U14-microdrive-quick.json`; `tests/fixtures/ui/U15-microdrive-manager.json`; `tests/fixtures/mdr/wzsn-mdr-multifile.mdr` | `certification/results/ui/U20.D108.json` |
| U20.D109 | 1361 | 20.12 | Format | Media-manager function | Retain; destructive confirmation required | U14.01; U15.01 | `tests/fixtures/ui/U14-microdrive-quick.json`; `tests/fixtures/ui/U15-microdrive-manager.json`; `tests/fixtures/mdr/wzsn-mdr-multifile.mdr` | `certification/results/ui/U20.D109.json` |
| U20.D110 | 1362 | 20.12 | Optimize | Media-manager function | Retain | U14.01; U15.01 | `tests/fixtures/ui/U14-microdrive-quick.json`; `tests/fixtures/ui/U15-microdrive-manager.json`; `tests/fixtures/mdr/wzsn-mdr-multifile.mdr` | `certification/results/ui/U20.D110.json` |
| U20.D111 | 1363 | 20.12 | View | Media-manager function | Sector/allocation view | U14.01; U15.01 | `tests/fixtures/ui/U14-microdrive-quick.json`; `tests/fixtures/ui/U15-microdrive-manager.json`; `tests/fixtures/mdr/wzsn-mdr-multifile.mdr` | `certification/results/ui/U20.D111.json` |
| U20.D112 | 1364 | 20.12 | Rename | Media-manager function | Logical cartridge rename | U14.01; U15.01 | `tests/fixtures/ui/U14-microdrive-quick.json`; `tests/fixtures/ui/U15-microdrive-manager.json`; `tests/fixtures/mdr/wzsn-mdr-multifile.mdr` | `certification/results/ui/U20.D112.json` |
| U20.D113 | 1365 | 20.12 | Write protect | Media-manager function | Retain | U14.01; U15.01 | `tests/fixtures/ui/U14-microdrive-quick.json`; `tests/fixtures/ui/U15-microdrive-manager.json`; `tests/fixtures/mdr/wzsn-mdr-multifile.mdr` | `certification/results/ui/U20.D113.json` |
| U20.D114 | 1366 | 20.12 | Write unprotect | Media-manager function | Retain | U14.01; U15.01 | `tests/fixtures/ui/U14-microdrive-quick.json`; `tests/fixtures/ui/U15-microdrive-manager.json`; `tests/fixtures/mdr/wzsn-mdr-multifile.mdr` | `certification/results/ui/U20.D114.json` |
| U20.D115 | 1372 | 20.13 | Verify | Media-manager function | Advanced/Sectors | U15.01 | `tests/fixtures/ui/U15-advanced-sectors.json`; `tests/fixtures/mdr/wzsn-mdr-multifile.mdr`; `tests/fixtures/mdr/wzsn-mdr-write-protect.mdr` | `certification/results/ui/U20.D115.json` |
| U20.D116 | 1373 | 20.13 | Edit | Media-manager function | Advanced/Sectors; dangerous raw mode explicit | U15.01 | `tests/fixtures/ui/U15-advanced-sectors.json`; `tests/fixtures/mdr/wzsn-mdr-multifile.mdr`; `tests/fixtures/mdr/wzsn-mdr-write-protect.mdr` | `certification/results/ui/U20.D116.json` |
| U20.D117 | 1379 | 20.14 | Delete | Media-manager function | Retain | U15.01 | `tests/fixtures/ui/U15-microdrive-manager.json`; `tests/fixtures/mdr/wzsn-mdr-multifile.mdr`; `tests/fixtures/mdr/wzsn-mdr-write-protect.mdr` | `certification/results/ui/U20.D117.json` |
| U20.D118 | 1380 | 20.14 | Rename | Media-manager function | Retain | U15.01 | `tests/fixtures/ui/U15-microdrive-manager.json`; `tests/fixtures/mdr/wzsn-mdr-multifile.mdr`; `tests/fixtures/mdr/wzsn-mdr-write-protect.mdr` | `certification/results/ui/U20.D118.json` |
| U20.D119 | 1381 | 20.14 | Hide | Media-manager function | Retain | U15.01 | `tests/fixtures/ui/U15-microdrive-manager.json`; `tests/fixtures/mdr/wzsn-mdr-multifile.mdr`; `tests/fixtures/mdr/wzsn-mdr-write-protect.mdr` | `certification/results/ui/U20.D119.json` |
| U20.D120 | 1382 | 20.14 | Unhide | Media-manager function | Retain | U15.01 | `tests/fixtures/ui/U15-microdrive-manager.json`; `tests/fixtures/mdr/wzsn-mdr-multifile.mdr`; `tests/fixtures/mdr/wzsn-mdr-write-protect.mdr` | `certification/results/ui/U20.D120.json` |
| U20.D121 | 1383 | 20.14 | Copy | Media-manager function | Retain | U15.01 | `tests/fixtures/ui/U15-microdrive-manager.json`; `tests/fixtures/mdr/wzsn-mdr-multifile.mdr`; `tests/fixtures/mdr/wzsn-mdr-write-protect.mdr` | `certification/results/ui/U20.D121.json` |
| U20.D122 | 1389 | 20.15 | Enlarge MDR to 254 sectors | Compatibility utility | Retain for legacy interoperability | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/mdr/wzsn-mdr-minimal.mdr`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D122.json` |
| U20.D123 | 1398 | 20.16 | Select Dock | Toolbar / quick action | Absent from initial UI; conditional destination applies only when Timex/DCK support is implemented | U17.01; U22.01 | `tests/fixtures/ui/U17-dock-absence.json`; `wzsn-architectures-1-2-developer-tasks.md`; `tests/fixtures/ui/U22-compatibility-tools.json` | `certification/results/ui/U20.D123.json` |
| U20.D124 | 1399 | 20.16 | Unselect Dock | Toolbar / quick action | Absent from initial UI; conditional destination applies only when Timex/DCK support is implemented | U17.01; U22.01 | `tests/fixtures/ui/U17-dock-absence.json`; `wzsn-architectures-1-2-developer-tasks.md`; `tests/fixtures/ui/U22-compatibility-tools.json` | `certification/results/ui/U20.D124.json` |
| U20.D125 | 1400 | 20.16 | View Dock | Media-manager function | Absent from initial UI; conditional destination applies only when Timex/DCK support is implemented | U17.01; U22.01 | `tests/fixtures/ui/U17-dock-absence.json`; `wzsn-architectures-1-2-developer-tasks.md`; `tests/fixtures/ui/U22-compatibility-tools.json` | `certification/results/ui/U20.D125.json` |
| U20.D126 | 1401 | 20.16 | Merge Dock | Media-manager function | Absent from initial UI; conditional destination applies only when Timex/DCK support is implemented | U17.01; U22.01 | `tests/fixtures/ui/U17-dock-absence.json`; `wzsn-architectures-1-2-developer-tasks.md`; `tests/fixtures/ui/U22-compatibility-tools.json` | `certification/results/ui/U20.D126.json` |
| U20.D127 | 1402 | 20.16 | ROM -> DCK | Compatibility utility | Absent from initial UI; conditional destination applies only when Timex/DCK support is implemented | U17.01; U22.01 | `tests/fixtures/ui/U17-dock-absence.json`; `wzsn-architectures-1-2-developer-tasks.md`; `tests/fixtures/ui/U22-compatibility-tools.json` | `certification/results/ui/U20.D127.json` |
| U20.D128 | 1403 | 20.16 | Binary -> LROS | Compatibility utility | Absent from initial UI; conditional destination applies only when Timex/DCK support is implemented | U17.01; U22.01 | `tests/fixtures/ui/U17-dock-absence.json`; `wzsn-architectures-1-2-developer-tasks.md`; `tests/fixtures/ui/U22-compatibility-tools.json` | `certification/results/ui/U20.D128.json` |
| U20.D129 | 1404 | 20.16 | Binary -> AROS | Compatibility utility | Absent from initial UI; conditional destination applies only when Timex/DCK support is implemented | U17.01; U22.01 | `tests/fixtures/ui/U17-dock-absence.json`; `wzsn-architectures-1-2-developer-tasks.md`; `tests/fixtures/ui/U22-compatibility-tools.json` | `certification/results/ui/U20.D129.json` |
| U20.D130 | 1413 | 20.17 | Select DB directory | Dropped legacy host function | Drop | U20.02; U08.01 | `tests/fixtures/ui/U20-legacy-prose-rules.json`; `tests/fixtures/ui/U08-file-workflows.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/ui/U20.D130.json` |
| U20.D131 | 1414 | 20.17 | Unselect DB directory | Dropped legacy host function | Drop | U20.02; U08.01 | `tests/fixtures/ui/U20-legacy-prose-rules.json`; `tests/fixtures/ui/U08-file-workflows.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/ui/U20.D131.json` |
| U20.D132 | 1415 | 20.17 | Run | Dropped legacy host function | Goal replaced by `Open / Run...`; future Library may supersede | U20.02; U08.01 | `tests/fixtures/ui/U20-legacy-prose-rules.json`; `tests/fixtures/ui/U08-file-workflows.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/ui/U20.D132.json` |
| U20.D133 | 1416 | 20.17 | Edit/Browse | Dropped legacy host function | Drop initial legacy database UI | U20.02; U08.01 | `tests/fixtures/ui/U20-legacy-prose-rules.json`; `tests/fixtures/ui/U08-file-workflows.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/ui/U20.D133.json` |
| U20.D134 | 1417 | 20.17 | Sort | Dropped legacy host function | Drop | U20.02; U08.01 | `tests/fixtures/ui/U20-legacy-prose-rules.json`; `tests/fixtures/ui/U08-file-workflows.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/ui/U20.D134.json` |
| U20.D135 | 1418 | 20.17 | Mark sort priority > Conditional | Dropped legacy host function | Drop | U20.02; U08.01 | `tests/fixtures/ui/U20-legacy-prose-rules.json`; `tests/fixtures/ui/U08-file-workflows.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/ui/U20.D135.json` |
| U20.D136 | 1419 | 20.17 | Mark sort priority > Swap marker | Dropped legacy host function | Drop | U20.02; U08.01 | `tests/fixtures/ui/U20-legacy-prose-rules.json`; `tests/fixtures/ui/U08-file-workflows.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/ui/U20.D136.json` |
| U20.D137 | 1420 | 20.17 | Mark sort priority > Unmark all | Dropped legacy host function | Drop | U20.02; U08.01 | `tests/fixtures/ui/U20-legacy-prose-rules.json`; `tests/fixtures/ui/U08-file-workflows.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/ui/U20.D137.json` |
| U20.D138 | 1421 | 20.17 | Report | Dropped legacy host function | Drop | U20.02; U08.01 | `tests/fixtures/ui/U20-legacy-prose-rules.json`; `tests/fixtures/ui/U08-file-workflows.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/ui/U20.D138.json` |
| U20.D139 | 1432 | 20.18 | Warajevo -> SpecPic | Compatibility utility | Retain as low-priority compatibility conversion | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D139.json` |
| U20.D140 | 1433 | 20.18 | SpecPic -> Warajevo | Compatibility utility | Retain as low-priority compatibility conversion | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D140.json` |
| U20.D141 | 1434 | 20.18 | Warajevo -> SGD | Compatibility utility | Retain as low-priority compatibility conversion | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D141.json` |
| U20.D142 | 1435 | 20.18 | SGD -> Warajevo | Compatibility utility | Retain as low-priority compatibility conversion | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D142.json` |
| U20.D143 | 1436 | 20.18 | Warajevo -> ZX Rainbow | Compatibility utility | Retain as low-priority compatibility conversion | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D143.json` |
| U20.D144 | 1437 | 20.18 | ZX Rainbow -> Warajevo | Compatibility utility | Retain as low-priority compatibility conversion | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D144.json` |
| U20.D145 | 1438 | 20.18 | Warajevo -> SpecBase | Compatibility utility | Retain as low-priority compatibility conversion | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D145.json` |
| U20.D146 | 1439 | 20.18 | SpecBase -> Warajevo | Compatibility utility | Retain as low-priority compatibility conversion | U22.01; U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U20.D146.json` |
| U20.D147 | 1445 | 20.19 | Emulator | Main menu | `Machine > Model` | U23.01-U23.06; U09.01-U09.04 | `tests/fixtures/ui/U23-settings.json`; `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/ui/U09-machine-workflows.json` | `certification/results/ui/U20.D147.json` |
| U20.D148 | 1446 | 20.19 | Video | Settings | `Settings > Display` | U23.01-U23.06; U09.01-U09.04 | `tests/fixtures/ui/U23-settings.json`; `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/ui/U09-machine-workflows.json` | `certification/results/ui/U20.D148.json` |
| U20.D149 | 1447 | 20.19 | Speed | Toolbar / quick action + main menu | `Machine > Emulation Speed` | U23.01-U23.06; U09.01-U09.04 | `tests/fixtures/ui/U23-settings.json`; `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/ui/U09-machine-workflows.json` | `certification/results/ui/U20.D149.json` |
| U20.D150 | 1448 | 20.19 | Test | Debugger tool | Split into relevant diagnostics/test infrastructure | U23.01-U23.06; U09.01-U09.04 | `tests/fixtures/ui/U23-settings.json`; `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/ui/U09-machine-workflows.json` | `certification/results/ui/U20.D150.json` |
| U20.D151 | 1449 | 20.19 | Network | Settings | `Settings > Peripherals > Networking` when `Interface-1` is selected | U23.01-U23.06; U09.01-U09.04 | `tests/fixtures/ui/U23-settings.json`; `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/ui/U09-machine-workflows.json` | `certification/results/ui/U20.D151.json` |
| U20.D152 | 1450 | 20.19 | Sound | Settings | `Settings > Audio` | U23.01-U23.06; U09.01-U09.04 | `tests/fixtures/ui/U23-settings.json`; `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/ui/U09-machine-workflows.json` | `certification/results/ui/U20.D152.json` |
| U20.D153 | 1451 | 20.19 | Interface 1 | Settings | `Settings > Peripherals > Networking` radio choice `Interface-1` | U23.01-U23.06; U09.01-U09.04 | `tests/fixtures/ui/U23-settings.json`; `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/ui/U09-machine-workflows.json` | `certification/results/ui/U20.D153.json` |
| U20.D154 | 1452 | 20.19 | Joystick | Settings | `Settings > Input > Joystick` | U23.01-U23.06; U09.01-U09.04 | `tests/fixtures/ui/U23-settings.json`; `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/ui/U09-machine-workflows.json` | `certification/results/ui/U20.D154.json` |
| U20.D155 | 1453 | 20.19 | Printer | Settings | `Settings > Peripherals > ZX Printer` | U23.01-U23.06; U09.01-U09.04 | `tests/fixtures/ui/U23-settings.json`; `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/ui/U09-machine-workflows.json` | `certification/results/ui/U20.D155.json` |
| U20.D156 | 1454 | 20.19 | ROM | Settings / Machine | ROM/firmware selection in Settings; machine model not hidden here | U23.01-U23.06; U09.01-U09.04 | `tests/fixtures/ui/U23-settings.json`; `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/ui/U09-machine-workflows.json` | `certification/results/ui/U20.D156.json` |
| U20.D157 | 1455 | 20.19 | MS Windows | Dropped legacy host function | DOS/V86 compatibility machinery removed | U23.01-U23.06; U09.01-U09.04 | `tests/fixtures/ui/U23-settings.json`; `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/ui/U09-machine-workflows.json` | `certification/results/ui/U20.D157.json` |
| U20.D158 | 1470 | 20.20 | About | Main menu | `Help > About Warajevo ZX Spectrum Next` | U07.01; U08.01 | `tests/fixtures/ui/U07-menu-tree.json`; `certification/manifests/ui-command-registry-v1.json`; `tests/fixtures/ui/U08-file-workflows.json` | `certification/results/ui/U20.D158.json` |
| U20.D159 | 1471 | 20.20 | Directory | Dropped legacy host function | Native file dialogs replace | U07.01; U08.01 | `tests/fixtures/ui/U07-menu-tree.json`; `certification/manifests/ui-command-registry-v1.json`; `tests/fixtures/ui/U08-file-workflows.json` | `certification/results/ui/U20.D159.json` |
| U20.D160 | 1472 | 20.20 | Change directory | Dropped legacy host function | Native file dialogs replace | U07.01; U08.01 | `tests/fixtures/ui/U07-menu-tree.json`; `certification/manifests/ui-command-registry-v1.json`; `tests/fixtures/ui/U08-file-workflows.json` | `certification/results/ui/U20.D160.json` |
| U20.D161 | 1473 | 20.20 | OS Shell | Dropped legacy host function | Do not embed a shell launcher | U07.01; U08.01 | `tests/fixtures/ui/U07-menu-tree.json`; `certification/manifests/ui-command-registry-v1.json`; `tests/fixtures/ui/U08-file-workflows.json` | `certification/results/ui/U20.D161.json` |
| U20.D162 | 1474 | 20.20 | Exit | Main menu | `File > Quit` | U07.01; U08.01 | `tests/fixtures/ui/U07-menu-tree.json`; `certification/manifests/ui-command-registry-v1.json`; `tests/fixtures/ui/U08-file-workflows.json` | `certification/results/ui/U20.D162.json` |
| U20.D163 | 1480 | 20.21 | Start button | Toolbar / quick action | `Open / Run...` | U06.01; U18.01; U24.01; U25.01 | `tests/fixtures/ui/U06-layout.json`; `tests/fixtures/ui/U18-view.json`; `tests/fixtures/ui/U24-toolbar.json`; `tests/fixtures/ui/U25-status-panel.json` | `certification/results/ui/U20.D163.json` |
| U20.D164 | 1481 | 20.21 | F10 Emulator | Toolbar / quick action | Run/Resume semantics; no second emulator executable | U06.01; U18.01; U24.01; U25.01 | `tests/fixtures/ui/U06-layout.json`; `tests/fixtures/ui/U18-view.json`; `tests/fixtures/ui/U24-toolbar.json`; `tests/fixtures/ui/U25-status-panel.json` | `certification/results/ui/U20.D164.json` |
| U20.D165 | 1482 | 20.21 | F1 Help | Main menu/shortcut | `Help` | U06.01; U18.01; U24.01; U25.01 | `tests/fixtures/ui/U06-layout.json`; `tests/fixtures/ui/U18-view.json`; `tests/fixtures/ui/U24-toolbar.json`; `tests/fixtures/ui/U25-status-panel.json` | `certification/results/ui/U20.D165.json` |
| U20.D166 | 1483 | 20.21 | Alt+F4 Exit | Native shortcut/main menu | `File > Quit` | U06.01; U18.01; U24.01; U25.01 | `tests/fixtures/ui/U06-layout.json`; `tests/fixtures/ui/U18-view.json`; `tests/fixtures/ui/U24-toolbar.json`; `tests/fixtures/ui/U25-status-panel.json` | `certification/results/ui/U20.D166.json` |
| U20.D167 | 1484 | 20.21 | Ctrl+F4 Close window | Native window behavior | No emulator semantics | U06.01; U18.01; U24.01; U25.01 | `tests/fixtures/ui/U06-layout.json`; `tests/fixtures/ui/U18-view.json`; `tests/fixtures/ui/U24-toolbar.json`; `tests/fixtures/ui/U25-status-panel.json` | `certification/results/ui/U20.D167.json` |
| U20.D168 | 1485 | 20.21 | Alt+F5 Zoom | Native/fullscreen/view behavior | Replace | U06.01; U18.01; U24.01; U25.01 | `tests/fixtures/ui/U06-layout.json`; `tests/fixtures/ui/U18-view.json`; `tests/fixtures/ui/U24-toolbar.json`; `tests/fixtures/ui/U25-status-panel.json` | `certification/results/ui/U20.D168.json` |
| U20.D169 | 1486 | 20.21 | Ctrl+F5 Resize | Native window behavior | Replace | U06.01; U18.01; U24.01; U25.01 | `tests/fixtures/ui/U06-layout.json`; `tests/fixtures/ui/U18-view.json`; `tests/fixtures/ui/U24-toolbar.json`; `tests/fixtures/ui/U25-status-panel.json` | `certification/results/ui/U20.D169.json` |
| U20.D170 | 1487 | 20.21 | Heap/free-RAM indicator | Dropped legacy host function | Not user machine state | U06.01; U18.01; U24.01; U25.01 | `tests/fixtures/ui/U06-layout.json`; `tests/fixtures/ui/U18-view.json`; `tests/fixtures/ui/U24-toolbar.json`; `tests/fixtures/ui/U25-status-panel.json` | `certification/results/ui/U20.D170.json` |
| U20.D171 | 1488 | 20.21 | Names window | Status panel | Replace with Machine/Media Status | U06.01; U18.01; U24.01; U25.01 | `tests/fixtures/ui/U06-layout.json`; `tests/fixtures/ui/U18-view.json`; `tests/fixtures/ui/U24-toolbar.json`; `tests/fixtures/ui/U25-status-panel.json` | `certification/results/ui/U20.D171.json` |
| U20.D172 | 1489 | 20.21 | Generated command parameters | Dropped legacy host function | No DOS child-emulator command line | U06.01; U18.01; U24.01; U25.01 | `tests/fixtures/ui/U06-layout.json`; `tests/fixtures/ui/U18-view.json`; `tests/fixtures/ui/U24-toolbar.json`; `tests/fixtures/ui/U25-status-panel.json` | `certification/results/ui/U20.D172.json` |
| U20.D173 | 1490 | 20.21 | Emulator executable filename | Dropped legacy host function | One integrated application | U06.01; U18.01; U24.01; U25.01 | `tests/fixtures/ui/U06-layout.json`; `tests/fixtures/ui/U18-view.json`; `tests/fixtures/ui/U24-toolbar.json`; `tests/fixtures/ui/U25-status-panel.json` | `certification/results/ui/U20.D173.json` |
| U20.D174 | 1491 | 20.21 | Temporary directory display | Dropped legacy host function | Host implementation detail | U06.01; U18.01; U24.01; U25.01 | `tests/fixtures/ui/U06-layout.json`; `tests/fixtures/ui/U18-view.json`; `tests/fixtures/ui/U24-toolbar.json`; `tests/fixtures/ui/U25-status-panel.json` | `certification/results/ui/U20.D174.json` |
## U21. Architecture section 21: Debugger / Monitor

**Architecture authority:** `U` source lines 1495-1529, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Debugger / Monitor UI.

**Required artifacts:**

- `tests/fixtures/debugger/wzsn-debugger-state-v1.json`
- `tests/fixtures/ui/U21-debugger.json`

**Artifact/source authority:** Project debugger state fixture and UI automation.

**Tests:**

### U21.01

Open/close Debugger/Monitor without creating a second core; verify shared-state surfaces for execution control, registers, flags, interrupt state, disassembly, memory view/editor, breakpoints/watchpoints, I/O/peripherals, paging, ULA/raster, AY, keyboard/joystick and trace/log views as those Phase-11 workflows are implemented.

**Evidence output:** `certification/results/ui/U21.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### U21.02

Idle debugger observation is non-intrusive; supported register/memory/hardware edits use the shared core mutation contract and subsequent execution matches the explicitly edited state.

**Evidence output:** `certification/results/ui/U21.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### U21.03

Legacy 128K-port-from-48K detection belongs under Diagnostics, not generic Settings/Test; automated quit-after-N-seconds belongs to headless/application test infrastructure and is absent from normal UI.

**Evidence output:** `certification/results/ui/U21.03.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U22. Architecture section 22: Compatibility Tools

**Architecture authority:** `U` source lines 1530-1569, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Compatibility Tools.

**Required artifacts:**

- `tests/fixtures/ui/U22-compatibility-tools.json`
- `tests/fixtures/snapshot/wzsn-z80-v2-128k.z80`
- `tests/fixtures/ui/U22-loss-disclosure-128k-to-48k.json`
- `tests/fixtures/ui/U22-conversion-provenance.json`

**Artifact/source authority:** Project-generated converter fixtures based on Section-20 disposition.

**Tests:**

### U22.01

Verify category structure Tape Converter / Snapshot Converter / Spectrum Data Converter / Microdrive Tools / Legacy Database Converter; a later/unavailable converter is absent or honestly disabled and cannot masquerade as complete.

**Evidence output:** `certification/results/ui/U22.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### U22.02

Formats natively supported by WZSN open natively through Open/Run; explicit conversion remains a separate preservation action.

**Evidence output:** `certification/results/ui/U22.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### U22.03

Using exact source `tests/fixtures/snapshot/wzsn-z80-v2-128k.z80` and `U22-loss-disclosure-128k-to-48k.json`, exercise the retained `Any Z80 -> 48K without Interface 1` conversion path when implemented. Before any output write, disclose every represented 128K/AY/paging/Interface-1 or other state/capability that the frozen converter contract says will be discarded; cancel writes nothing. If that converter is not yet implemented, the test is BLOCKED/LATER and the UI must not advertise it as working.

**Evidence output:** `certification/results/ui/U22.03.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### U22.04

Using `U22-conversion-provenance.json`, every generated conversion evidence record must identify exact source artifact, source format, destination format, converter revision, output artifact, warnings/loss disclosure, and deterministic options sufficient to reproduce and understand the generated regression artifact.

**Evidence output:** `certification/results/ui/U22.04.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U23. Architecture section 23: Settings architecture

**Architecture authority:** `U` source lines 1570-1716, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Settings architecture.

**Required artifacts:**

- `tests/fixtures/ui/U23-settings.json`
- `tests/fixtures/networking/wzsn-networking-modes-v1.json`
- `tests/fixtures/rom/zx48-certification.rom`
- `tests/fixtures/rom/zx128-certification-0.rom`
- `tests/fixtures/ui/U23-host-setting-ownership.json`
- `tests/fixtures/ui/U23-networking-transition.json`
- `tests/fixtures/ui/U23-telnet-settings.json`
- `tests/fixtures/networking/wzsn-ear-mic-bootstrap-test.tap`

**Artifact/source authority:** Project settings/networking fixtures; ROM bytes legally supplied and pinned by Core profile evidence.

**Tests:**

### U23.01

Using `U23-host-setting-ownership.json`, prove the exact Settings category tree. Display controls only host scaling/crop/fullscreen/presentation and cannot alter ULA timing, contention, FLASH, or canonical raster. Audio controls host output only; beeper/AY remain machine truth and DOS PC-speaker/AdLib/SoundBlaster choices are absent. Input owns host-to-normalized mapping including Kempston and never stores host key identities in the core. ROM/Firmware selection is explicit, validates frozen hash/licensing identity, and does not hide machine-profile selection inside ROM settings. ZX Printer settings configure virtual presentation/output, never DOS LPT routing.

**Evidence output:** `certification/results/ui/U23.01.json`

**Pass rule:** PASS only when every settings category has the frozen machine/host ownership boundary.

### U23.02

Using `wzsn-networking-modes-v1.json`, prove Networking is exactly one radio group None/Interface-1/Ear+Mic backed solely by `machine.networking.set`; no combination can activate IF1 and Ear+Mic together. Prove the exact meaning of all three modes, no Ear+Mic networking-ROM paging or RAM auto-install, and no mutation on rejected profile/availability requests.

**Evidence output:** `certification/results/ui/U23.02.json`

**Pass rule:** PASS only when networking state is structurally single-valued and mode meanings exactly match Core Section 24.

### U23.03

Using `U23-networking-transition.json`, change networking mode while running and paused and prove the shared cold-reconfiguration path discards current Spectrum RAM/device/hook state, retains the same machine model and selected new mode, is distinct from ordinary Reset, preserves application pause, and displays a clear RAM-loss warning before local commit. Leaving Interface-1 with dirty writable Microdrive media must require successful local commit or cancel; unauthenticated remote mode change is available only when no host write/confirmation is required and otherwise returns a registry disabled reason such as `dirty-media-requires-local-resolution`. Successful departure detaches Microdrive slots.

**Evidence output:** `certification/results/ui/U23.03.json`

**Pass rule:** PASS only when local/remote transition and dirty-media semantics are exact and atomic.

### U23.04

Prove Ear+Mic remains unavailable with a registry reason until Architecture #3 integration passes and is unavailable on 128K or uncertified 48K variants with a stable reason such as `requires-48k-issue2`, leaving the current machine unchanged. In Interface-1 mode, only subordinate settings justified by the frozen peripheral architecture may appear; historical PSWait/FreeNet/BusyWait host timing knobs must not reappear automatically.

**Evidence output:** `certification/results/ui/U23.04.json`

**Pass rule:** PASS only when availability and subordinate-setting rules are exact.

### U23.05

Using `tests/fixtures/networking/wzsn-ear-mic-bootstrap-test.tap` as a project-owned synthetic bootstrap waveform fixture, prove Ear+Mic disables ordinary Tape controls with reason `cassette-owned-by-ear-mic-network`. When the downstream Architecture-3 action is present, `Bootstrap Ear+Mic Stack` must enter the Architecture-3 `BOOTSTRAP_TAPE` path and emit a tape waveform; this UI test must fail if the action injects RAM, calls ROM routines, or pages a ROM. Until Architecture #3 implements that downstream action, this subtest is BLOCKED_GATE rather than guessed.

**Evidence output:** `certification/results/ui/U23.05.json`

**Pass rule:** PASS only after the downstream integration artifact exists and the UI remains waveform-only.

### U23.06

Using `U23-telnet-settings.json`, verify the Telnet Keyboard & Remote Control page shows service enabled-by-architecture state, base 30740, range 30740-32787, selected port or unavailable, IPv4 and IPv6 UP/DOWN, normal/degraded/unavailable listener state, ACTIVE/NONE client state, plaintext/no-auth warning, and initial permission summary. The selected Control Port is session-only, never persisted, has no initial manual override, and listener startup is automatic. No UI text may imply plaintext/no-auth is secure.

**Evidence output:** `certification/results/ui/U23.06.json`

**Pass rule:** PASS only when every Section-23.7 field and non-persistence/security rule is exact.

## U24. Architecture section 24: Toolbar architecture

**Architecture authority:** `U` source lines 1717-1759, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Toolbar architecture.

**Required artifacts:**

- `tests/fixtures/ui/U24-toolbar.json`
- `tests/fixtures/ui/U24-screenshot-workflow.json`

**Artifact/source authority:** Project UI automation.

**Tests:**

### U24.01

Compare exact semantic toolbar set [Open/Run, Pause/Resume, Reset, Speed, Tape, Load Snapshot, Save Snapshot, MDV1, Screenshot, Fullscreen, Debugger]; text/icons are presentation only; each uses the shared registry handler and availability predicate. The selected speed value is always visible, changing it invokes `machine.speed.set`, and status/audio-muted indication updates immediately. Using `U24-screenshot-workflow.json`, prove toolbar Screenshot enters the GUI save-screenshot destination workflow and does not use the Telnet temporary-path naming rule.

**Evidence output:** `certification/results/ui/U24.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U25. Architecture section 25: Machine / Media status panel

**Architecture authority:** `U` source lines 1760-1826, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Machine / Media status panel.

**Required artifacts:**

- `tests/fixtures/ui/U25-status-panel.json`
- `tests/fixtures/ui/U25-control-port-interaction.json`

**Artifact/source authority:** Project state matrix UI fixture.

**Tests:**

### U25.01

For representative machine/media/network states assert the optional full panel exposes machine model, speed/audio, tape identity/mode, all eight Microdrive slots, networking mode, Control Port, IPv4/IPv6 listener status and active-client state while obsolete EXE/command-options/heap/DOS-temp trivia never appears. Even with the panel hidden, status-line indicators must make model, speed, running/paused, audio on/muted-by-speed, mounted tape, primary Microdrive, networking mode, Control Port, and listener/client state readily visible. The exact field is `Control Port: <five-digit-port>` or `Control Port: unavailable`. Using `U25-control-port-interaction.json`, activate that indicator and prove it opens the Telnet settings/status page through the shared UI route.

**Evidence output:** `certification/results/ui/U25.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U26. Architecture section 26: Telnet console role

**Architecture authority:** `U` source lines 1827-1845, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Telnet console role.

**Required artifacts:**

- `tests/fixtures/ui/U26-telnet-role.json`

**Artifact/source authority:** Headless/application projection test.

**Tests:**

### U26.01

Verify Telnet has exactly two roles: Core-owned hardware-level keyboard source and UI-owned application-control console projected through the shared registry. Transport/one-client/wildcard/IAC/threading/matrix injection remain Core Section-55 responsibilities; application grammar remains UI responsibility. No Telnet control command may write Spectrum RAM directly, call ROM routines, bypass the application/orchestrator, or depend on GUI widgets for execution.

**Evidence output:** `certification/results/ui/U26.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U27. Architecture section 27: Initial Telnet application grammar

**Architecture authority:** `U` source lines 1846-1918, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Initial Telnet application grammar.

**Required artifacts:**

- `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt`
- `tests/fixtures/ui/U27-telnet-grammar.txt`

**Artifact/source authority:** Project-owned line corpus copied from Section 27.

**Tests:**

### U27.01

Accept exactly HELP/STATUS, KEY family, RELEASE ALL, PAUSE/RESUME/RESET, MODEL 48K/128K, SPEED seven values, SCREENSHOT, MENU family, DESCRIBE, DO; input command words/IDs/model/speed/key names are ASCII case-insensitive; output canonicalizes as specified; keyboard success/error responses exactly OK/BAD_KEY/BAD_STATE.

**Evidence output:** `certification/results/ui/U27.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U28. Architecture section 28: Telnet line grammar and bounded parsing

**Architecture authority:** `U` source lines 1919-1964, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Telnet line grammar and bounded parsing.

**Required artifacts:**

- `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt`
- `tests/fixtures/telnet/wzsn-telnet-overlong-1025.bin`

**Artifact/source authority:** Project-generated byte corpus.

**Tests:**

### U28.01

Test ASCII control-byte rejection, CRLF and LF termination, exactly 1024 decoded bytes maximum excluding IAC/terminator, LINE_TOO_LONG recovery through next terminator, spaces/tabs, quoted strings with only \\ and \" escapes, stable ID character set, and zero shell/environment/wildcard/substitution expansion.

**Evidence output:** `certification/results/ui/U28.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U29. Architecture section 29: Telnet HELP and STATUS

**Architecture authority:** `U` source lines 1965-2001, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Telnet HELP and STATUS.

**Required artifacts:**

- `tests/fixtures/ui/U29-help-expected.txt`
- `tests/fixtures/ui/U29-status-cases.json`

**Artifact/source authority:** Project golden transcripts.

**Tests:**

### U29.01

HELP returns the special-command grammar, points the client to `MENU TREE` and `DESCRIBE` for the complete registry, and terminates with `END`. STATUS is `REMOTE_SAFE`, side-effect-free, and begins with the exact `PROTOCOL=1 CONTROL_PORT IPV4 IPV6 CLIENT MODEL STATE SPEED AUDIO NETWORKING` field order/value domains. Exercise `AUDIO=ON`, `AUDIO=MUTED`, and `AUDIO=UNAVAILABLE` explicitly: `MUTED` must include the Core runtime-speed mute policy, while `UNAVAILABLE` means no usable host audio output exists. Add approved trailing `name=value` fields and prove existing field names/meanings do not change; optional mounted-media fields may expose only approved type/slot/sanitized basename data and never arbitrary absolute host media paths.

**Evidence output:** `certification/results/ui/U29.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U30. Architecture section 30: Telnet RESET

**Architecture authority:** `U` source lines 2002-2032, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Telnet RESET.

**Required artifacts:**

- `tests/fixtures/ui/U30-reset.json`

**Artifact/source authority:** Project state/input trace.

**Tests:**

### U30.01

Invoke `RESET` and `DO machine.reset` from equivalent starting states and verify the exact `OK RESET` success response, authentic Spectrum reset rather than WZSN restart, Telnet connection preservation, selected speed and host configuration preservation, and the mounted-media plus local/Telnet held-key/pending-`KEY PRESS` semantics frozen by the Core architecture; the command must be valid while running or paused and a paused application must remain paused. Send `RESET HARD` and representative second-reset variants and prove they are not recognized as an alternate reset meaning: they receive the protocol's controlled invalid-command handling and cause no machine mutation.

**Evidence output:** `certification/results/ui/U30.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U31. Architecture section 31: Telnet PAUSE and RESUME

**Architecture authority:** `U` source lines 2033-2055, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Telnet PAUSE and RESUME.

**Required artifacts:**

- `tests/fixtures/ui/U31-pause-resume.json`

**Artifact/source authority:** Project command-state trace.

**Tests:**

### U31.01

Verify PAUSE/RESUME idempotence, exact response contract, same shared handler as GUI/toolbar/DO, and no unintended machine-state mutation beyond scheduler pause state.

**Evidence output:** `certification/results/ui/U31.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U32. Architecture section 32: Telnet MODEL

**Architecture authority:** `U` source lines 2056-2085, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Telnet MODEL.

**Required artifacts:**

- `tests/fixtures/ui/U32-model.json`

**Artifact/source authority:** Project machine-profile state fixture.

**Tests:**

### U32.01

Prove `MODEL 48K` and `MODEL 128K` are REMOTE_SAFE aliases for `DO machine.model.set 48k|128k`; exact success responses are `OK MODEL 48K` / `OK MODEL 128K`; unsupported values return `ERR BAD_MODEL` without mutation. Valid changes use the same GUI model-change/reset workflow, preserve application pause until explicit Resume, and reevaluate all state predicates including Ear+Mic availability.

**Evidence output:** `certification/results/ui/U32.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U33. Architecture section 33: Telnet SPEED

**Architecture authority:** `U` source lines 2086-2118, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Telnet SPEED.

**Required artifacts:**

- `tests/fixtures/ui/U33-speed.json`

**Artifact/source authority:** Project speed-state matrix.

**Tests:**

### U33.01

Prove all seven SPEED aliases are REMOTE_SAFE and map to `machine.speed.set`; accept only 25/50/100/200/400/800/UNLIMITED, return exact canonical `OK SPEED <value>` on success and `ERR BAD_SPEED` on invalid input. Valid changes must produce exactly the same host pacing, tape wall-clock duration, audio-mute presentation, and unchanged canonical Spectrum timing as GUI/toolbar speed changes.

**Evidence output:** `certification/results/ui/U33.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U34. Architecture section 34: Telnet SCREENSHOT

**Architecture authority:** `U` source lines 2119-2238, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Telnet SCREENSHOT.

**Required artifacts:**

- `tests/fixtures/video/wzsn-screenshot-raster-v1.rgba`
- `tests/fixtures/ui/U34-screenshot-collisions.json`
- `tests/fixtures/ui/U34-screenshot-failures.json`

**Artifact/source authority:** Project golden raster and filesystem race/failure harness.

**Tests:**

### U34.01

Verify SCREENSHOT is the REMOTE_SAFE alias for `host.screenshot.temp`; client-supplied destination arguments are rejected. Compare PNG pixels against `wzsn-screenshot-raster-v1.rgba` and prove no UI/chrome/cursor/desktop pixels. Destination is the OS temp directory. Freeze host-local naming time and verify exact `ZX-Screen-YYYYMMDDHHMMSSmmm.png`; force collisions and prove atomic exclusive-create suffixes `-1`, `-2`, ... with no overwrite or cross-process race. Capture the most recently completed eligible raster at the next safe owner-path boundary; paused capture uses the currently displayed completed raster; screenshot never advances master time or becomes deterministic machine input. Success is sent only after encode/close/path availability as `OK SCREENSHOT <absolute-host-path>`, with path occupying the unescaped remainder of the line. Using `U34-screenshot-failures.json`, force no-raster/cannot-create-file/encode-failed/write-failed and verify `ERR SCREENSHOT <reason>` with zero Spectrum mutation.

**Evidence output:** `certification/results/ui/U34.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U35. Architecture section 35: Telnet MENU projection

**Architecture authority:** `U` source lines 2239-2315, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Telnet MENU projection.

**Required artifacts:**

- `tests/fixtures/ui/U07-menu-tree.json`
- `tests/fixtures/ui/U35-menu-transcripts.txt`

**Artifact/source authority:** Project golden tree/transcripts from compiled registry.

**Tests:**

### U35.01

Verify bare MENU returns exactly the canonical top-level IDs/labels then END. Verify MENU TREE traverses the complete registry and every record uses the frozen ITEM field form/order including PARENT/TYPE/STATE/REMOTE/CLASS/LABEL plus conditional REASON/COMMAND/ARGS; denied commands remain discoverable and dynamic recent absolute paths/path-derived labels are absent. Verify `MENU <id>` returns the requested node plus immediate children or command metadata and unknown ID returns `ERR BAD_COMMAND_ID`. Verify `MENU FIND <text>` is case-insensitive over stable IDs, canonical labels, and short descriptions, is side-effect free, and terminates correctly. For no-tape and other changing states, prove enabled/disabled/reason comes from the same GUI predicate and remains independent of Telnet permission.

**Evidence output:** `certification/results/ui/U35.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U36. Architecture section 36: Telnet DESCRIBE

**Architecture authority:** `U` source lines 2316-2347, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Telnet DESCRIBE.

**Required artifacts:**

- `tests/fixtures/ui/U36-describe.json`

**Artifact/source authority:** Project registry-derived expected metadata.

**Tests:**

### U36.01

For every command ID verify ID/LABEL/DESCRIPTION/PARAMETERS/ENABLED/DISABLED_REASON/REMOTE_CLASS/REMOTE_ALLOWED/AFFECTS_MACHINE_STATE/RECORDABLE, END termination, side-effect-free REMOTE_SAFE operation, and no current private host paths/arguments.

**Evidence output:** `certification/results/ui/U36.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U37. Architecture section 37: Telnet DO

**Architecture authority:** `U` source lines 2348-2422, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Telnet DO.

**Required artifacts:**

- `tests/fixtures/ui/U37-do-cases.json`

**Artifact/source authority:** Project command invocation matrix.

**Tests:**

### U37.01

Verify exact evaluation order parse -> resolve ID -> validate arguments -> query enabled state -> query Telnet permission -> enqueue semantic command -> return result, with no handler invocation before all checks pass. Disabled returns exact `ERR BAD_STATE <id> <reason>`; denied returns exact `DENIED <id> <class>` and must not open a file chooser, confirmation dialog, or hidden GUI interaction. Success returns at least `OK DO <id>` plus schema-valid fields. `DO host.screenshot.temp` returns `OK DO host.screenshot.temp PATH="<absolute-host-path>"` for the same created file used by dedicated SCREENSHOT. Prove RESET/SPEED/SCREENSHOT aliases and DO forms converge on one handler/effect.

**Evidence output:** `certification/results/ui/U37.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U38. Architecture section 38: Telnet errors and response framing

**Architecture authority:** `U` source lines 2423-2457, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Telnet errors and response framing.

**Required artifacts:**

- `tests/fixtures/ui/U38-errors.txt`

**Artifact/source authority:** Project byte-exact golden transcript.

**Tests:**

### U38.01

Exercise every initial error code BAD_COMMAND/BAD_COMMAND_ID/BAD_ARGUMENT/BAD_KEY/BAD_MODEL/BAD_SPEED/BAD_STATE/LINE_TOO_LONG/SCREENSHOT; unknown special command exact response; multiline END; single-line CRLF; no terminal escape/control sequences.

**Evidence output:** `certification/results/ui/U38.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U39. Architecture section 39: Telnet security boundary

**Architecture authority:** `U` source lines 2458-2493, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Telnet security boundary.

**Required artifacts:**

- `tests/fixtures/ui/U05-remote-permissions.json`
- `tests/fixtures/ui/U39-security.json`

**Artifact/source authority:** Project policy matrix/security test.

**Tests:**

### U39.01

With plaintext/no-auth/all-interface listener, prove only REMOTE_SAFE executes; no hidden allow-all switch; HOST_READ/HOST_WRITE/destructive/local quit denied; screenshot allowed only because destination controlled; all denied operations remain discoverable and side-effect free.

**Evidence output:** `certification/results/ui/U39.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U40. Architecture section 40: GUI/Telnet command-state model

**Architecture authority:** `U` source lines 2494-2531, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** GUI/Telnet command-state model.

**Required artifacts:**

- `tests/fixtures/ui/U40-command-state-matrix.json`

**Artifact/source authority:** Project state matrix.

**Tests:**

### U40.01

For enabled/disabled and remote allowed/denied combinations verify application availability and Telnet permission are independent axes represented consistently by GUI and Telnet and recomputed without stale state. Explicitly verify the architecture examples: no-tape Eject = DISABLED(no-tape-mounted) + REMOTE_SAFE; Save Snapshot As = ENABLED + HOST_WRITE/DENIED; mounted writable Microdrive Format = ENABLED + MEDIA_DESTRUCTIVE/DENIED; Reset = ENABLED + REMOTE_SAFE/ALLOWED.

**Evidence output:** `certification/results/ui/U40.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U41. Architecture section 41: UI threading and timing

**Architecture authority:** `U` source lines 2532-2560, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** UI threading and timing.

**Required artifacts:**

- `tests/fixtures/ui/U41-threading.json`
- `tests/fixtures/ui/U41-modal-time.json`
- `tests/fixtures/ui/U41-long-media-worker.json`

**Artifact/source authority:** Project concurrency harness plus canonical core hashes.

**Tests:**

### U41.01

Stress GUI, toolbar, Telnet and application-test commands arriving from different host contexts and prove every state-changing semantic command is serialized through the one-owner application path before machine mutation. Using `U41-modal-time.json`, hold a blocking file dialog/modal operation while application-paused and prove canonical master time does not advance; opening/closing the chooser is never a Spectrum timing event. Using `U41-long-media-worker.json`, run long conversion/media maintenance on background workers and prove workers process host-only data, never mutate live Spectrum state directly, and correctness/atomic file behavior is preserved even when presentation responsiveness degrades.

**Evidence output:** `certification/results/ui/U41.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U42. Architecture section 42: Keyboard focus and command shortcuts

**Architecture authority:** `U` source lines 2561-2590, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Keyboard focus and shortcuts.

**Required artifacts:**

- `tests/fixtures/ui/U42-focus-input.json`
- `tests/fixtures/ui/U42-modal-text-input.json`

**Artifact/source authority:** Project host-input trace.

**Tests:**

### U42.01

With Spectrum viewport focus, prove ordinary mapped Spectrum keys enter only through the host input arbiter. Exercise explicitly defined application shortcuts while keys are held and prove they cannot leave stale Spectrum presses. Using `U42-modal-text-input.json`, give a modal text/input control focus and prove host typing is consumed by that control and does not silently reach the Spectrum unless an explicitly tested pass-through control is used. Exercise focus gain/loss, dialogs, fullscreen and manager transitions with local keys held; the documented focus policy releases/preserves local ownership without stuck matrix keys, while independently owned Telnet keys remain active and separately resolved.

**Evidence output:** `certification/results/ui/U42.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U43. Architecture section 43: Accessibility and keyboard operability

**Architecture authority:** `U` source lines 2591-2619, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Accessibility and keyboard operability.

**Required artifacts:**

- `tests/fixtures/ui/U43-keyboard-workflows.json`

**Artifact/source authority:** Project keyboard workflow and toolkit accessibility inspection.

**Tests:**

### U43.01

Complete, without a mouse, every literal minimum workflow: menu traversal, Open/Run, pause/resume, reset, speed selection, tape insert/eject/loading mode, snapshot load/save, fullscreen toggle, opening debugger/settings/status, and confirm/cancel of destructive local operations. Verify visible keyboard focus, keyboard activation/cancel, readable disabled-state explanations where practical, non-color-only status signaling, and accessibility names/roles/state through platform-native APIs supplied by the selected toolkit. The Phase-12 accessibility gate artifact must document that toolkit support and the certification evidence must record platform-specific results.

**Evidence output:** `certification/results/ui/U43.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U44. Architecture section 44: Settings persistence

**Architecture authority:** `U` source lines 2620-2668, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Settings persistence.

**Required artifacts:**

- `tests/fixtures/ui/wzsn-settings-valid.json`
- `tests/fixtures/ui/wzsn-settings-corrupt.json`
- `tests/fixtures/ui/U44-settings-race.json`
- `tests/fixtures/ui/U44-media-path-alias.json`
- `tests/fixtures/ui/U44-output-race.json`

**Artifact/source authority:** Project settings fixtures and multi-process fault harness.

**Tests:**

### U44.01

Verify only allowed host preferences persist (window geometry, fullscreen/display presentation, audio preference, host input mappings, optional last speed by product policy, status-panel visibility, recent list); persistent host configuration is never canonical Spectrum state. ROM/machine-critical certification identity cannot silently drift from preferences, and media insertion state is not automatically restored absent a separately designed/tested workflow. Concurrent settings writes are serialized and atomically replaced; last-writer-wins is acceptable only after a complete valid noncritical write. Selected Control Port never persists. Using `U44-media-path-alias.json`, prove resolved-file identity prevents a second writable media owner even through path aliases. Using `U44-output-race.json`, race snapshots/conversions/exports and prove exclusive destination ownership or atomic write-and-replace prevents partial/interleaved output; race Telnet screenshots and prove atomic no-clobber suffix creation across processes.

**Evidence output:** `certification/results/ui/U44.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U45. Architecture section 45: Error and confirmation policy

**Architecture authority:** `U` source lines 2669-2704, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Error and confirmation policy.

**Required artifacts:**

- `tests/fixtures/ui/U45-error-confirmation.json`
- `tests/fixtures/ui/U45-no-silent-fallback.json`

**Artifact/source authority:** Project failure/confirmation matrix.

**Tests:**

### U45.01

Exercise failed file open, malformed snapshot, unsupported format, screenshot write failure, and Telnet bind failure and prove each reports error without corrupting live machine state. For Microdrive format, block/file delete, raw-sector edit, and existing-output overwrite, require explicit local confirmation unless the operation is demonstrably reversible and clearly presented. Over Telnet, prove any destructive/privileged command that would require local confirmation is denied instead of synthesizing a GUI prompt. Using `U45-no-silent-fallback.json`, force every forbidden fallback individually: machine model must not change silently; Control Port must never leave 30740-32787; Normal tape must not silently become trap; lossy conversion must not be selected silently; write failure must not redirect output silently; existing Telnet screenshot must never be overwritten.

**Evidence output:** `certification/results/ui/U45.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U46. Architecture section 46: Application test projection

**Architecture authority:** `U` source lines 2705-2729, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Application test projection.

**Required artifacts:**

- `tests/fixtures/ui/U46-equivalence.json`

**Artifact/source authority:** Project command-dispatch instrumentation.

**Tests:**

### U46.01

Invoke the registry directly without Sokol/Telnet when transport/presentation is not under test and prove the registry is the application integration-test API. For every literal equivalence family in Section 46—Reset, 400% speed, Tape Eject, and screenshot—execute all listed GUI/toolbar/Telnet/DO/direct-test projections and compare handler ID, arguments, availability decision, result, and canonical machine state. For screenshot specifically, GUI and Telnet must share the same canonical raster capture and PNG encoding path while differing only in host destination acquisition (GUI chosen path versus controlled Telnet temp path).

**Evidence output:** `certification/results/ui/U46.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U47. Architecture section 47: Required UI regression tests

**Architecture authority:** `U` source lines 2730-2835, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Required UI regression tests.

**Required artifacts:**

- `tests/fixtures/ui/U47-required-regression-manifest.json`

**Artifact/source authority:** Manually transcribed Section-47 list plus automated completeness check.

**Tests:**

### U47.01

Execute every literal test item listed in Architecture #2 Section 47 exactly once or more; the manifest maps each line to concrete test ID/artifact/evidence and fails on missing/unmapped entries.

**Evidence output:** `certification/results/ui/U47.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### U47 literal required-regression ledger

| Subtest | Literal Section-47 item | Primary proof test(s) | Required input/proof artifact(s) | Evidence artifact |
|---|---|---|---|---|
| U47.R001 | start with default supported machine and no media | U06.01; U25.01 | `tests/fixtures/ui/U06-layout.json`; `tests/fixtures/ui/U25-status-panel.json` | `certification/results/ui/U47.R001.json` |
| U47.R002 | 48K <-> 128K model change/reset workflow | U09.01-U09.04 | `tests/fixtures/ui/U09-machine-workflows.json`; `tests/fixtures/ui/U09-reset-preservation.json` | `certification/results/ui/U47.R002.json` |
| U47.R003 | pause/resume from menu, toolbar, registry, and Telnet | U31.01; U24.01; U46.01 | `tests/fixtures/ui/U31-pause-resume.json`; `tests/fixtures/ui/U24-toolbar.json`; `tests/fixtures/ui/U46-equivalence.json` | `certification/results/ui/U47.R003.json` |
| U47.R004 | reset from menu, toolbar, registry, and Telnet | U30.01; U24.01; U46.01 | `tests/fixtures/ui/U30-reset.json`; `tests/fixtures/ui/U24-toolbar.json`; `tests/fixtures/ui/U46-equivalence.json` | `certification/results/ui/U47.R004.json` |
| U47.R005 | all seven emulation-speed values from menu, toolbar, registry, and Telnet | U33.01; U24.01; U46.01 | `tests/fixtures/ui/U33-speed.json`; `tests/fixtures/ui/U24-toolbar.json`; `tests/fixtures/ui/U46-equivalence.json` | `certification/results/ui/U47.R005.json` |
| U47.R006 | speed change while running | U33.01; U09.01-U09.04 | `tests/fixtures/ui/U33-speed.json`; `tests/fixtures/ui/U09-machine-workflows.json`; `tests/fixtures/ui/U09-pause-master-time.json` | `certification/results/ui/U47.R006.json` |
| U47.R007 | speed change while paused | U33.01; U09.01-U09.04 | `tests/fixtures/ui/U33-speed.json`; `tests/fixtures/ui/U09-machine-workflows.json`; `tests/fixtures/ui/U09-pause-master-time.json` | `certification/results/ui/U47.R007.json` |
| U47.R008 | correct audio-muted status outside 50%-200% | U25.01; U33.01 | `tests/fixtures/ui/U25-status-panel.json`; `tests/fixtures/ui/U33-speed.json` | `certification/results/ui/U47.R008.json` |
| U47.R009 | Open / Run native format routing | U08.01 | `tests/fixtures/ui/U08-file-workflows.json`; `tests/fixtures/tape/wzsn-tap-standard-oneblock.tap`; `tests/fixtures/tzx/wzsn-tzx-standard-speed.tzx`; `tests/fixtures/snapshot/wzsn-sna-48k-canonical.sna`; `tests/fixtures/mdr/wzsn-mdr-minimal.mdr` | `certification/results/ui/U47.R009.json` |
| U47.R010 | cancel file chooser without state mutation | U08.02 | `tests/fixtures/ui/U08-file-workflows.json` | `certification/results/ui/U47.R010.json` |
| U47.R011 | malformed/unsupported file error path | U08.03; U12.05; U13.01 | `tests/fixtures/ui/U08-file-workflows.json`; `tests/fixtures/tzx/wzsn-tzx-malformed.tzx`; `tests/fixtures/snapshot/wzsn-sna-malformed-truncated.sna` | `certification/results/ui/U47.R011.json` |
| U47.R012 | snapshot atomic load failure | U13.01 | `tests/fixtures/snapshot/wzsn-sna-malformed-truncated.sna`; `tests/fixtures/ui/U13-snapshot-inspector.json` | `certification/results/ui/U47.R012.json` |
| U47.R013 | snapshot save/save-as | U08.04; U13.01 | `tests/fixtures/ui/U08-snapshot-destination.json`; `tests/fixtures/snapshot/wzsn-sna-48k-canonical.sna`; `tests/fixtures/snapshot/wzsn-z80-v2-128k.z80` | `certification/results/ui/U47.R013.json` |
| U47.R014 | Tape insert/eject | U11.01 | `tests/fixtures/tape/wzsn-tap-standard-oneblock.tap`; `tests/fixtures/ui/U11-tape-quick.json`; `tests/fixtures/ui/U11-toolbar-tape-menu.json` | `certification/results/ui/U47.R014.json` |
| U47.R015 | Normal default | U11.01 | `tests/fixtures/tape/wzsn-tap-standard-oneblock.tap`; `tests/fixtures/ui/U11-tape-quick.json`; `tests/fixtures/ui/U11-toolbar-tape-menu.json` | `certification/results/ui/U47.R015.json` |
| U47.R016 | Instant/Trap explicit selection | U11.01 | `tests/fixtures/tape/wzsn-tap-standard-oneblock.tap`; `tests/fixtures/ui/U11-tape-quick.json`; `tests/fixtures/ui/U11-toolbar-tape-menu.json` | `certification/results/ui/U47.R016.json` |
| U47.R017 | Tape Manager enabled-state transitions | U12.01 | `tests/fixtures/ui/U12-tape-manager.json`; `tests/fixtures/tzx/wzsn-tzx-control-flow.tzx` | `certification/results/ui/U47.R017.json` |
| U47.R018 | block mutation atomic/error behavior | U12.02; U12.05 | `tests/fixtures/ui/U12-tape-manager.json`; `tests/fixtures/tzx/wzsn-tzx-malformed.tzx`; `tests/fixtures/tape/wzsn-warajevo-native-edge.tap` | `certification/results/ui/U47.R018.json` |
| U47.R019 | Microdrive mount/eject for drives 1-8 | U14.01; U15.01 | `tests/fixtures/ui/U14-microdrive-quick.json`; `tests/fixtures/ui/U15-drive-overview.json`; `tests/fixtures/mdr/wzsn-mdr-multifile.mdr` | `certification/results/ui/U47.R019.json` |
| U47.R020 | same writable image duplicate-mount prevention | C28.03; U44.01 | `tests/fixtures/concurrency/wzsn-multi-instance-plan-v1.json`; `tests/fixtures/ui/U44-media-path-alias.json`; `tests/fixtures/mdr/wzsn-mdr-multifile.mdr` | `certification/results/ui/U47.R020.json` |
| U47.R021 | format destructive confirmation | U15.01 | `tests/fixtures/ui/U15-microdrive-manager.json`; `tests/fixtures/mdr/wzsn-mdr-write-protect.mdr` | `certification/results/ui/U47.R021.json` |
| U47.R022 | logical-file and sector-manager state | U15.01 | `tests/fixtures/ui/U15-microdrive-manager.json`; `tests/fixtures/ui/U15-advanced-sectors.json`; `tests/fixtures/mdr/wzsn-mdr-multifile.mdr` | `certification/results/ui/U47.R022.json` |
| U47.R023 | ZX Printer manager availability | U16.01 | `tests/fixtures/printer/wzsn-zxprinter-smoke.tap`; `tests/fixtures/ui/U16-printer-manager.json` | `certification/results/ui/U47.R023.json` |
| U47.R024 | Debugger opening without second core | U21.01-U21.03 | `tests/fixtures/debugger/wzsn-debugger-state-v1.json`; `tests/fixtures/ui/U21-debugger.json` | `certification/results/ui/U47.R024.json` |
| U47.R025 | Fullscreen without core-state change | U18.01 | `tests/fixtures/ui/U18-view.json` | `certification/results/ui/U47.R025.json` |
| U47.R026 | status panel correctness | U25.01 | `tests/fixtures/ui/U25-status-panel.json` | `certification/results/ui/U47.R026.json` |
| U47.R027 | Telnet listener/client/bind status presentation | U23.06; U25.01 | `tests/fixtures/ui/U23-telnet-settings.json`; `tests/fixtures/ui/U25-status-panel.json`; `tests/fixtures/ui/U25-control-port-interaction.json` | `certification/results/ui/U47.R027.json` |
| U47.R028 | Control Port status text always visible | U23.06; U25.01 | `tests/fixtures/ui/U23-telnet-settings.json`; `tests/fixtures/ui/U25-status-panel.json`; `tests/fixtures/ui/U25-control-port-interaction.json` | `certification/results/ui/U47.R028.json` |
| U47.R029 | first instance obtains 30740 when free | C55.01; U23.06; U25.01 | `tests/fixtures/telnet/wzsn-control-port-race-v1.json`; `tests/fixtures/ui/U23-telnet-settings.json`; `tests/fixtures/ui/U25-status-panel.json` | `certification/results/ui/U47.R029.json` |
| U47.R030 | second/third concurrent instances obtain first free ascending candidates | C55.01; U23.06; U25.01 | `tests/fixtures/telnet/wzsn-control-port-race-v1.json`; `tests/fixtures/ui/U23-telnet-settings.json`; `tests/fixtures/ui/U25-status-panel.json` | `certification/results/ui/U47.R030.json` |
| U47.R031 | occupied candidate is skipped without using a port outside 30740-32787 | C55.01; U23.06; U25.01 | `tests/fixtures/telnet/wzsn-control-port-race-v1.json`; `tests/fixtures/ui/U23-telnet-settings.json`; `tests/fixtures/ui/U25-status-panel.json` | `certification/results/ui/U47.R031.json` |
| U47.R032 | simultaneous-start race cannot produce duplicate numeric Control Port ownership | C55.01; U23.06; U25.01 | `tests/fixtures/telnet/wzsn-control-port-race-v1.json`; `tests/fixtures/ui/U23-telnet-settings.json`; `tests/fixtures/ui/U25-status-panel.json` | `certification/results/ui/U47.R032.json` |
| U47.R033 | full 2048-port exhaustion reports unavailable without terminating WZSN | C55.01; U23.06; U25.01 | `tests/fixtures/telnet/wzsn-control-port-race-v1.json`; `tests/fixtures/ui/U23-telnet-settings.json`; `tests/fixtures/ui/U25-status-panel.json` | `certification/results/ui/U47.R033.json` |
| U47.R034 | selected Control Port is not persisted across launches | C55.01; U23.06; U25.01 | `tests/fixtures/telnet/wzsn-control-port-race-v1.json`; `tests/fixtures/ui/U23-telnet-settings.json`; `tests/fixtures/ui/U25-status-panel.json` | `certification/results/ui/U47.R034.json` |
| U47.R035 | IPv4/IPv6 split-family collision rejects the candidate numeric port | C55.01; U23.06; U25.01 | `tests/fixtures/telnet/wzsn-control-port-race-v1.json`; `tests/fixtures/ui/U23-telnet-settings.json`; `tests/fixtures/ui/U25-status-panel.json` | `certification/results/ui/U47.R035.json` |
| U47.R036 | networking radio group has exactly None / Interface-1 / Ear+Mic | U23.02-U23.04 | `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/ui/U23-networking-transition.json`; `tests/fixtures/rom/zx48-certification.rom`; `tests/fixtures/rom/zx128-certification-0.rom` | `certification/results/ui/U47.R036.json` |
| U47.R037 | networking mode is structurally mutually exclusive | U23.02-U23.04 | `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/ui/U23-networking-transition.json`; `tests/fixtures/rom/zx48-certification.rom`; `tests/fixtures/rom/zx128-certification-0.rom` | `certification/results/ui/U47.R037.json` |
| U47.R038 | None disables Interface 1, Microdrive, ZX Net, and Ear+Mic attachment | U23.02-U23.04 | `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/ui/U23-networking-transition.json`; `tests/fixtures/rom/zx48-certification.rom`; `tests/fixtures/rom/zx128-certification-0.rom` | `certification/results/ui/U47.R038.json` |
| U47.R039 | Interface-1 enables authentic IF1/Microdrive/ZX Net and disables Ear+Mic | U23.02-U23.04 | `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/ui/U23-networking-transition.json`; `tests/fixtures/rom/zx48-certification.rom`; `tests/fixtures/rom/zx128-certification-0.rom` | `certification/results/ui/U47.R039.json` |
| U47.R040 | Ear+Mic disables Interface 1/Microdrive/ZX Net and does not page a networking ROM | U23.02-U23.04 | `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/ui/U23-networking-transition.json`; `tests/fixtures/rom/zx48-certification.rom`; `tests/fixtures/rom/zx128-certification-0.rom` | `certification/results/ui/U47.R040.json` |
| U47.R041 | Ear+Mic selection does not auto-install the distributed RAM stack | U23.02-U23.04 | `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/ui/U23-networking-transition.json`; `tests/fixtures/rom/zx48-certification.rom`; `tests/fixtures/rom/zx128-certification-0.rom` | `certification/results/ui/U47.R041.json` |
| U47.R042 | Ear+Mic disables ordinary Tape transport while routed network owns EAR/MIC | U23.02-U23.04 | `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/ui/U23-networking-transition.json`; `tests/fixtures/rom/zx48-certification.rom`; `tests/fixtures/rom/zx128-certification-0.rom` | `certification/results/ui/U47.R042.json` |
| U47.R043 | Architecture-3 Bootstrap Ear+Mic Stack uses BOOTSTRAP_TAPE waveform path | U23.05 | `tests/fixtures/networking/wzsn-ear-mic-bootstrap-test.tap`; `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `zx48-mic-ear-router-network-architecture.md` | `certification/results/ui/U47.R043.json` |
| U47.R044 | networking-mode change uses cold reconfiguration, clears old RAM/hooks, and preserves paused application state | U23.02-U23.04 | `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/ui/U23-networking-transition.json`; `tests/fixtures/rom/zx48-certification.rom`; `tests/fixtures/rom/zx128-certification-0.rom` | `certification/results/ui/U47.R044.json` |
| U47.R045 | Ear+Mic unavailable-before-Architecture-3 reason propagates through registry/UI | U23.02-U23.04 | `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/ui/U23-networking-transition.json`; `tests/fixtures/rom/zx48-certification.rom`; `tests/fixtures/rom/zx128-certification-0.rom` | `certification/results/ui/U47.R045.json` |
| U47.R046 | Ear+Mic on 128K/non-Issue2 profile is disabled with requires-48k-issue2 reason | U23.02-U23.04 | `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/ui/U23-networking-transition.json`; `tests/fixtures/rom/zx48-certification.rom`; `tests/fixtures/rom/zx128-certification-0.rom` | `certification/results/ui/U47.R046.json` |
| U47.R047 | cross-process writable-media exclusion | C28.03; U44.01 | `tests/fixtures/concurrency/wzsn-multi-instance-plan-v1.json`; `tests/fixtures/ui/U44-media-path-alias.json` | `certification/results/ui/U47.R047.json` |
| U47.R048 | cross-process settings atomic-write safety | C28.03; U44.01 | `tests/fixtures/concurrency/wzsn-multi-instance-plan-v1.json`; `tests/fixtures/ui/U44-settings-race.json` | `certification/results/ui/U47.R048.json` |
| U47.R049 | cross-process Telnet screenshot no-clobber creation | C28.02; U34.01 | `tests/fixtures/concurrency/wzsn-multi-instance-plan-v1.json`; `tests/fixtures/ui/U34-screenshot-collisions.json` | `certification/results/ui/U47.R049.json` |
| U47.R050 | command-registry stable-ID uniqueness | U04.01 | `tests/fixtures/ui/U04-command-registry.json`; `certification/manifests/ui-command-registry-v1.json` | `certification/results/ui/U47.R050.json` |
| U47.R051 | required menu-node/action and non-menu semantic IDs from Section 7.1 exist | U07.01 | `tests/fixtures/ui/U07-required-command-ids.json`; `tests/fixtures/ui/U07-menu-tree.json` | `certification/results/ui/U47.R051.json` |
| U47.R052 | Snapshot Inspector reachable through Tools and shared state-inspection code | U19.01; U13.01 | `tests/fixtures/ui/U19-tools.json`; `tests/fixtures/ui/U13-snapshot-inspector.json`; `tests/fixtures/ui/U13-snapshot-editor-reuse.json` | `certification/results/ui/U47.R052.json` |
| U47.R053 | one handler path for equivalent GUI/toolbar/Telnet/test commands | U46.01 | `tests/fixtures/ui/U46-equivalence.json`; `certification/manifests/ui-command-registry-v1.json` | `certification/results/ui/U47.R053.json` |
| U47.R054 | identical enabled predicate across GUI and MENU output | U04.01-U04.04; U40.01 | `tests/fixtures/ui/U04-command-registry.json`; `tests/fixtures/ui/U40-command-state-matrix.json` | `certification/results/ui/U47.R054.json` |
| U47.R055 | disabled-reason propagation | U04.01-U04.04; U40.01 | `tests/fixtures/ui/U04-command-registry.json`; `tests/fixtures/ui/U40-command-state-matrix.json` | `certification/results/ui/U47.R055.json` |
| U47.R056 | remote permission-class enforcement | U05.01-U05.03; U39.01 | `tests/fixtures/ui/U05-remote-permissions.json`; `tests/fixtures/ui/U05-permission-negative-cases.json`; `tests/fixtures/ui/U05-local-only-policy.json`; `tests/fixtures/ui/U39-security.json` | `certification/results/ui/U47.R056.json` |
| U47.R057 | MENU root | U35.01 | `tests/fixtures/ui/U07-menu-tree.json`; `tests/fixtures/ui/U35-menu-transcripts.txt` | `certification/results/ui/U47.R057.json` |
| U47.R058 | MENU TREE complete registry traversal | U35.01 | `tests/fixtures/ui/U07-menu-tree.json`; `tests/fixtures/ui/U35-menu-transcripts.txt` | `certification/results/ui/U47.R058.json` |
| U47.R059 | MENU <id> | U35.01 | `tests/fixtures/ui/U07-menu-tree.json`; `tests/fixtures/ui/U35-menu-transcripts.txt` | `certification/results/ui/U47.R059.json` |
| U47.R060 | MENU FIND | U35.01 | `tests/fixtures/ui/U07-menu-tree.json`; `tests/fixtures/ui/U35-menu-transcripts.txt` | `certification/results/ui/U47.R060.json` |
| U47.R061 | DESCRIBE | U36.01 | `tests/fixtures/ui/U36-describe.json` | `certification/results/ui/U47.R061.json` |
| U47.R062 | DO success | U37.01; U38.01 | `tests/fixtures/ui/U37-do-cases.json`; `tests/fixtures/ui/U38-errors.txt`; `tests/fixtures/ui/U05-remote-permissions.json` | `certification/results/ui/U47.R062.json` |
| U47.R063 | DO bad ID | U37.01; U38.01 | `tests/fixtures/ui/U37-do-cases.json`; `tests/fixtures/ui/U38-errors.txt`; `tests/fixtures/ui/U05-remote-permissions.json` | `certification/results/ui/U47.R063.json` |
| U47.R064 | DO bad argument | U37.01; U38.01 | `tests/fixtures/ui/U37-do-cases.json`; `tests/fixtures/ui/U38-errors.txt`; `tests/fixtures/ui/U05-remote-permissions.json` | `certification/results/ui/U47.R064.json` |
| U47.R065 | DO disabled command | U37.01; U38.01 | `tests/fixtures/ui/U37-do-cases.json`; `tests/fixtures/ui/U38-errors.txt`; `tests/fixtures/ui/U05-remote-permissions.json` | `certification/results/ui/U47.R065.json` |
| U47.R066 | DO denied command | U37.01; U38.01 | `tests/fixtures/ui/U37-do-cases.json`; `tests/fixtures/ui/U38-errors.txt`; `tests/fixtures/ui/U05-remote-permissions.json` | `certification/results/ui/U47.R066.json` |
| U47.R067 | RESET exact response and connection preservation | U30.01; C55.03 | `tests/fixtures/ui/U30-reset.json`; `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt` | `certification/results/ui/U47.R067.json` |
| U47.R068 | PAUSE/RESUME idempotence | U31.01 | `tests/fixtures/ui/U31-pause-resume.json` | `certification/results/ui/U47.R068.json` |
| U47.R069 | MODEL valid/invalid values | U32.01 | `tests/fixtures/ui/U32-model.json` | `certification/results/ui/U47.R069.json` |
| U47.R070 | SPEED valid/invalid values | U33.01 | `tests/fixtures/ui/U33-speed.json` | `certification/results/ui/U47.R070.json` |
| U47.R071 | SCREENSHOT while running | U34.01 | `tests/fixtures/video/wzsn-screenshot-raster-v1.rgba`; `tests/fixtures/ui/U34-screenshot-collisions.json`; `tests/fixtures/ui/U34-screenshot-failures.json` | `certification/results/ui/U47.R071.json` |
| U47.R072 | SCREENSHOT while paused | U34.01 | `tests/fixtures/video/wzsn-screenshot-raster-v1.rgba`; `tests/fixtures/ui/U34-screenshot-collisions.json`; `tests/fixtures/ui/U34-screenshot-failures.json` | `certification/results/ui/U47.R072.json` |
| U47.R073 | SCREENSHOT PNG pixel-source equivalence | U34.01 | `tests/fixtures/video/wzsn-screenshot-raster-v1.rgba`; `tests/fixtures/ui/U34-screenshot-collisions.json`; `tests/fixtures/ui/U34-screenshot-failures.json` | `certification/results/ui/U47.R073.json` |
| U47.R074 | SCREENSHOT temp-directory placement | U34.01 | `tests/fixtures/video/wzsn-screenshot-raster-v1.rgba`; `tests/fixtures/ui/U34-screenshot-collisions.json`; `tests/fixtures/ui/U34-screenshot-failures.json` | `certification/results/ui/U47.R074.json` |
| U47.R075 | SCREENSHOT timestamp filename pattern | U34.01 | `tests/fixtures/video/wzsn-screenshot-raster-v1.rgba`; `tests/fixtures/ui/U34-screenshot-collisions.json`; `tests/fixtures/ui/U34-screenshot-failures.json` | `certification/results/ui/U47.R075.json` |
| U47.R076 | SCREENSHOT collision suffix/no overwrite | U34.01 | `tests/fixtures/video/wzsn-screenshot-raster-v1.rgba`; `tests/fixtures/ui/U34-screenshot-collisions.json`; `tests/fixtures/ui/U34-screenshot-failures.json` | `certification/results/ui/U47.R076.json` |
| U47.R077 | SCREENSHOT failure without machine mutation | U34.01 | `tests/fixtures/video/wzsn-screenshot-raster-v1.rgba`; `tests/fixtures/ui/U34-screenshot-collisions.json`; `tests/fixtures/ui/U34-screenshot-failures.json` | `certification/results/ui/U47.R077.json` |
| U47.R078 | Telnet HELP/STATUS response contracts | U29.01 | `tests/fixtures/ui/U29-help-expected.txt`; `tests/fixtures/ui/U29-status-cases.json` | `certification/results/ui/U47.R078.json` |
| U47.R079 | 1024-byte line limit/recovery | U28.01; U39.01 | `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt`; `tests/fixtures/telnet/wzsn-telnet-overlong-1025.bin`; `tests/fixtures/ui/U39-security.json` | `certification/results/ui/U47.R079.json` |
| U47.R080 | quoted DO argument parsing | U28.01; U39.01 | `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt`; `tests/fixtures/telnet/wzsn-telnet-overlong-1025.bin`; `tests/fixtures/ui/U39-security.json` | `certification/results/ui/U47.R080.json` |
| U47.R081 | no shell/environment expansion | U28.01; U39.01 | `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt`; `tests/fixtures/telnet/wzsn-telnet-overlong-1025.bin`; `tests/fixtures/ui/U39-security.json` | `certification/results/ui/U47.R081.json` |
| U47.R082 | multi-line END termination | U28.01; U39.01 | `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt`; `tests/fixtures/telnet/wzsn-telnet-overlong-1025.bin`; `tests/fixtures/ui/U39-security.json` | `certification/results/ui/U47.R082.json` |
| U47.R083 | no terminal escape/control pollution | U28.01; U39.01 | `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt`; `tests/fixtures/telnet/wzsn-telnet-overlong-1025.bin`; `tests/fixtures/ui/U39-security.json` | `certification/results/ui/U47.R083.json` |
| U47.R084 | keyboard-only operation of required local workflows | U43.01 | `tests/fixtures/ui/U43-keyboard-workflows.json` | `certification/results/ui/U47.R084.json` |
| U47.R085 | focus transitions without stuck Spectrum keys | U42.01 | `tests/fixtures/ui/U42-focus-input.json`; `tests/fixtures/ui/U42-modal-text-input.json` | `certification/results/ui/U47.R085.json` |
| U47.R086 | local focus change while Telnet keys remain independently owned | U42.01 | `tests/fixtures/ui/U42-focus-input.json`; `tests/fixtures/ui/U42-modal-text-input.json` | `certification/results/ui/U47.R086.json` |
## U48. Architecture section 48: UI acceptance contract

**Architecture authority:** `U` source lines 2836-2951, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** UI acceptance contract 1-67.

**Required artifacts:**

- `certification/manifests/ui-section48-acceptance.json`
- `certification/manifests/ui-section48-proof-inputs.json`

**Artifact/source authority:** `ui-section48-acceptance.json` records the 67 criterion results. `ui-section48-proof-inputs.json` is generated from the literal U48 ledger below and records, for each criterion ID, the exact lower-level test IDs and proof-artifact paths required before that criterion can PASS; result JSONs are not proof inputs.

**Tests:**

### U48.01

Execute one explicit acceptance record for each of the 67 numbered Section-48 criteria, each pointing to lower-level tests/evidence; criteria 1..67 must appear exactly once and none may PASS solely on aggregate assertion.

**Evidence output:** `certification/results/ui/U48.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### U48 acceptance-item forensic ledger

| Test | Source line | Literal acceptance criterion | Primary proof test(s) | Required proof artifact(s) | Evidence artifact |
|---|---:|---|---|---|---|
| U48.AC01 | 2841 | the semantic top-level menu tree is `File`, `Machine`, `Media`, `View`, `Tools`, `Settings`, `Help`; | U07.01 | `tests/fixtures/ui/U07-menu-tree.json` | `certification/results/ui/U48.AC01.json` |
| U48.AC02 | 2843 | the running Spectrum display is the primary application surface; | U06.01 | `tests/fixtures/ui/U06-layout.json` | `certification/results/ui/U48.AC02.json` |
| U48.AC03 | 2844 | one shared command registry owns every cross-front-end semantic operation; | U04.01-U04.04; U46.01 | `tests/fixtures/ui/U04-command-registry.json`; `tests/fixtures/ui/U04-fixed-action-bindings.json`; `tests/fixtures/ui/U46-equivalence.json`; `certification/manifests/ui-command-registry-v1.json` | `certification/results/ui/U48.AC03.json` |
| U48.AC04 | 2845 | stable command IDs are unique and independent of cosmetic GUI wording; | U04.01-U04.04; U46.01 | `tests/fixtures/ui/U04-command-registry.json`; `tests/fixtures/ui/U04-fixed-action-bindings.json`; `tests/fixtures/ui/U46-equivalence.json`; `certification/manifests/ui-command-registry-v1.json` | `certification/results/ui/U48.AC04.json` |
| U48.AC05 | 2846 | GUI menus, toolbar, Telnet control, and application tests use shared semantic handlers rather than private duplicate implementations; | U04.01-U04.04; U46.01 | `tests/fixtures/ui/U04-command-registry.json`; `tests/fixtures/ui/U04-fixed-action-bindings.json`; `tests/fixtures/ui/U46-equivalence.json`; `certification/manifests/ui-command-registry-v1.json` | `certification/results/ui/U48.AC05.json` |
| U48.AC06 | 2848 | command availability predicates and disabled reasons come from the registry; | U04.01-U04.04; U46.01 | `tests/fixtures/ui/U04-command-registry.json`; `tests/fixtures/ui/U04-fixed-action-bindings.json`; `tests/fixtures/ui/U46-equivalence.json`; `certification/manifests/ui-command-registry-v1.json` | `certification/results/ui/U48.AC06.json` |
| U48.AC07 | 2849 | Telnet reports the same availability state as the GUI; | U40.01; U35.01 | `tests/fixtures/ui/U40-command-state-matrix.json`; `tests/fixtures/ui/U35-menu-transcripts.txt` | `certification/results/ui/U48.AC07.json` |
| U48.AC08 | 2850 | remote permission class is independent from application availability; | U05.01-U05.03; U39.01 | `tests/fixtures/ui/U05-remote-permissions.json`; `tests/fixtures/ui/U05-permission-negative-cases.json`; `tests/fixtures/ui/U05-local-only-policy.json`; `tests/fixtures/ui/U39-security.json` | `certification/results/ui/U48.AC08.json` |
| U48.AC09 | 2851 | the initial Telnet service allows only `REMOTE_SAFE` command execution; | U05.01-U05.03; U39.01 | `tests/fixtures/ui/U05-remote-permissions.json`; `tests/fixtures/ui/U05-permission-negative-cases.json`; `tests/fixtures/ui/U05-local-only-policy.json`; `tests/fixtures/ui/U39-security.json` | `certification/results/ui/U48.AC09.json` |
| U48.AC10 | 2852 | the entire command/menu tree remains discoverable over Telnet even when a command is denied remotely; | U05.01-U05.03; U39.01 | `tests/fixtures/ui/U05-remote-permissions.json`; `tests/fixtures/ui/U05-permission-negative-cases.json`; `tests/fixtures/ui/U05-local-only-policy.json`; `tests/fixtures/ui/U39-security.json` | `certification/results/ui/U48.AC10.json` |
| U48.AC11 | 2854 | `Open / Run...` is the universal initial user entry for supported media; | U08.01 | `tests/fixtures/ui/U08-file-workflows.json`; `tests/fixtures/tape/wzsn-tap-standard-oneblock.tap`; `tests/fixtures/snapshot/wzsn-sna-48k-canonical.sna`; `tests/fixtures/mdr/wzsn-mdr-minimal.mdr` | `certification/results/ui/U48.AC11.json` |
| U48.AC12 | 2855 | Tape Insert/Eject/Loading Mode are direct quick operations; | U11.01 | `tests/fixtures/ui/U11-tape-quick.json`; `tests/fixtures/tape/wzsn-tap-standard-oneblock.tap` | `certification/results/ui/U48.AC12.json` |
| U48.AC13 | 2856 | Normal tape loading is visibly the default; | U11.01 | `tests/fixtures/ui/U11-tape-quick.json`; `tests/fixtures/tape/wzsn-tap-standard-oneblock.tap` | `certification/results/ui/U48.AC13.json` |
| U48.AC14 | 2857 | detailed tape manipulation lives in Tape Manager; | U12.01-U12.05 | `tests/fixtures/ui/U12-tape-manager.json`; `tests/fixtures/tzx/wzsn-tzx-control-flow.tzx`; `tests/fixtures/tape/wzsn-warajevo-native-minimal.tap` | `certification/results/ui/U48.AC14.json` |
| U48.AC15 | 2858 | detailed Microdrive manipulation lives in Microdrive Manager; | U15.01 | `tests/fixtures/ui/U15-microdrive-manager.json`; `tests/fixtures/mdr/wzsn-mdr-multifile.mdr` | `certification/results/ui/U48.AC15.json` |
| U48.AC16 | 2859 | snapshots use load/save semantics rather than persistent select/unselect; | U08.04; U13.01 | `tests/fixtures/ui/U08-snapshot-destination.json`; `tests/fixtures/ui/U13-snapshot-inspector.json`; `tests/fixtures/snapshot/wzsn-sna-48k-canonical.sna`; `tests/fixtures/snapshot/wzsn-z80-v2-128k.z80` | `certification/results/ui/U48.AC16.json` |
| U48.AC17 | 2860 | register/hardware/memory editing reuses debugger/state-inspection machinery; | U21.01-U21.03 | `tests/fixtures/debugger/wzsn-debugger-state-v1.json`; `tests/fixtures/ui/U21-debugger.json` | `certification/results/ui/U48.AC17.json` |
| U48.AC18 | 2861 | legacy conversion features are grouped under Compatibility Tools; | U22.01-U22.04 | `tests/fixtures/ui/U22-compatibility-tools.json`; `tests/fixtures/ui/U22-conversion-provenance.json` | `certification/results/ui/U48.AC18.json` |
| U48.AC19 | 2862 | historical DOS shell/directory/heap/child-EXE UI is absent; | U20.01-U20.02; U20.D001-U20.D174 | `tests/fixtures/ui/U20-legacy-disposition.json`; `tests/fixtures/ui/U20-legacy-prose-rules.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/ui/U48.AC19.json` |
| U48.AC20 | 2863 | the historical database application is not reproduced as an initial top-level subsystem; | U20.01-U20.02; U20.D001-U20.D174 | `tests/fixtures/ui/U20-legacy-disposition.json`; `tests/fixtures/ui/U20-legacy-prose-rules.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/ui/U48.AC20.json` |
| U48.AC21 | 2865 | Dock UI is absent until Timex/DCK support is actually implemented; | U17.01 | `tests/fixtures/ui/U17-dock-absence.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/ui/U48.AC21.json` |
| U48.AC22 | 2866 | the toolbar contains the canonical quick controls in Section 24; | U24.01 | `tests/fixtures/ui/U24-toolbar.json` | `certification/results/ui/U48.AC22.json` |
| U48.AC23 | 2867 | the emulation-speed selector exposes exactly 25, 50, 100, 200, 400, 800, and Unlimited; | U24.01; U33.01 | `tests/fixtures/ui/U24-toolbar.json`; `tests/fixtures/ui/U33-speed.json` | `certification/results/ui/U48.AC23.json` |
| U48.AC24 | 2869 | speed state is visible and audio-muted-by-speed state is visible; | U25.01; U33.01 | `tests/fixtures/ui/U25-status-panel.json`; `tests/fixtures/ui/U33-speed.json` | `certification/results/ui/U48.AC24.json` |
| U48.AC25 | 2870 | Machine Reset invokes the authentic machine reset and does not restart WZSN; | U30.01; U46.01; C55.03 | `tests/fixtures/ui/U30-reset.json`; `tests/fixtures/ui/U46-equivalence.json`; `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt` | `certification/results/ui/U48.AC25.json` |
| U48.AC26 | 2871 | GUI Reset, toolbar Reset, Telnet `RESET`, and `DO machine.reset` converge on the same handler; | U30.01; U46.01; C55.03 | `tests/fixtures/ui/U30-reset.json`; `tests/fixtures/ui/U46-equivalence.json`; `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt` | `certification/results/ui/U48.AC26.json` |
| U48.AC27 | 2873 | Telnet remains connected across `RESET`; | U30.01; U46.01; C55.03 | `tests/fixtures/ui/U30-reset.json`; `tests/fixtures/ui/U46-equivalence.json`; `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt` | `certification/results/ui/U48.AC27.json` |
| U48.AC28 | 2874 | `PAUSE` and `RESUME` are idempotent shared-command aliases; | U31.01 | `tests/fixtures/ui/U31-pause-resume.json` | `certification/results/ui/U48.AC28.json` |
| U48.AC29 | 2875 | `MODEL 48K` and `MODEL 128K` use the same model-change/reset workflow as the GUI; | U32.01; U09.01-U09.04 | `tests/fixtures/ui/U32-model.json`; `tests/fixtures/ui/U09-machine-workflows.json` | `certification/results/ui/U48.AC29.json` |
| U48.AC30 | 2877 | Telnet `SPEED` uses the same runtime-speed handler as GUI/toolbar control; | U33.01; U46.01 | `tests/fixtures/ui/U33-speed.json`; `tests/fixtures/ui/U46-equivalence.json` | `certification/results/ui/U48.AC30.json` |
| U48.AC31 | 2878 | Telnet `SCREENSHOT` writes PNG to the OS temporary directory; | U34.01 | `tests/fixtures/video/wzsn-screenshot-raster-v1.rgba`; `tests/fixtures/ui/U34-screenshot-collisions.json`; `tests/fixtures/ui/U34-screenshot-failures.json`; `tests/fixtures/ui/U24-screenshot-workflow.json` | `certification/results/ui/U48.AC31.json` |
| U48.AC32 | 2879 | Telnet screenshots use `ZX-Screen-YYYYMMDDHHMMSSmmm.png` and collision suffixes rather than overwriting; | U34.01 | `tests/fixtures/video/wzsn-screenshot-raster-v1.rgba`; `tests/fixtures/ui/U34-screenshot-collisions.json`; `tests/fixtures/ui/U34-screenshot-failures.json`; `tests/fixtures/ui/U24-screenshot-workflow.json` | `certification/results/ui/U48.AC32.json` |
| U48.AC33 | 2881 | Telnet `SCREENSHOT` reports the absolute saved path only after a successful completed write; | U34.01 | `tests/fixtures/video/wzsn-screenshot-raster-v1.rgba`; `tests/fixtures/ui/U34-screenshot-collisions.json`; `tests/fixtures/ui/U34-screenshot-failures.json`; `tests/fixtures/ui/U24-screenshot-workflow.json` | `certification/results/ui/U48.AC33.json` |
| U48.AC34 | 2883 | screenshot capture excludes host application chrome and does not mutate or advance Spectrum state; | U34.01 | `tests/fixtures/video/wzsn-screenshot-raster-v1.rgba`; `tests/fixtures/ui/U34-screenshot-collisions.json`; `tests/fixtures/ui/U34-screenshot-failures.json`; `tests/fixtures/ui/U24-screenshot-workflow.json` | `certification/results/ui/U48.AC34.json` |
| U48.AC35 | 2885 | GUI screenshot save and Telnet screenshot use the same capture/PNG path; | U34.01 | `tests/fixtures/video/wzsn-screenshot-raster-v1.rgba`; `tests/fixtures/ui/U34-screenshot-collisions.json`; `tests/fixtures/ui/U34-screenshot-failures.json`; `tests/fixtures/ui/U24-screenshot-workflow.json` | `certification/results/ui/U48.AC35.json` |
| U48.AC36 | 2886 | `MENU`, `MENU TREE`, `MENU <id>`, `MENU FIND`, `DESCRIBE`, and `DO` operate against the shared registry; | U35.01; U36.01; U37.01 | `tests/fixtures/ui/U35-menu-transcripts.txt`; `tests/fixtures/ui/U36-describe.json`; `tests/fixtures/ui/U37-do-cases.json`; `tests/fixtures/ui/U07-menu-tree.json` | `certification/results/ui/U48.AC36.json` |
| U48.AC37 | 2888 | state-disabled registry commands are represented consistently in GUI and Telnet discovery; | U40.01 | `tests/fixtures/ui/U40-command-state-matrix.json` | `certification/results/ui/U48.AC37.json` |
| U48.AC38 | 2890 | host-read, host-write, destructive-media, application-control, and local-only commands are denied by initial unauthenticated Telnet policy; | U05.01-U05.03; U39.01 | `tests/fixtures/ui/U05-remote-permissions.json`; `tests/fixtures/ui/U05-permission-negative-cases.json`; `tests/fixtures/ui/U05-local-only-policy.json`; `tests/fixtures/ui/U39-security.json` | `certification/results/ui/U48.AC38.json` |
| U48.AC39 | 2892 | the Telnet application parser is bounded to 1024-byte decoded command lines and deterministically recovers after overflow; | U28.01 | `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt`; `tests/fixtures/telnet/wzsn-telnet-overlong-1025.bin` | `certification/results/ui/U48.AC39.json` |
| U48.AC40 | 2894 | Telnet generic arguments are never interpreted by an OS shell; | U39.01 | `tests/fixtures/ui/U39-security.json`; `tests/fixtures/ui/U37-do-cases.json` | `certification/results/ui/U48.AC40.json` |
| U48.AC41 | 2895 | required local workflows are keyboard-operable without a mouse; | U43.01 | `tests/fixtures/ui/U43-keyboard-workflows.json` | `certification/results/ui/U48.AC41.json` |
| U48.AC42 | 2896 | focus changes do not leave stuck local Spectrum matrix keys; | U42.01 | `tests/fixtures/ui/U42-focus-input.json`; `tests/fixtures/ui/U42-modal-text-input.json` | `certification/results/ui/U48.AC42.json` |
| U48.AC43 | 2897 | local GUI focus does not disable independently owned Telnet keys; | U42.01 | `tests/fixtures/ui/U42-focus-input.json`; `tests/fixtures/ui/U42-modal-text-input.json` | `certification/results/ui/U48.AC43.json` |
| U48.AC44 | 2898 | fullscreen, manager windows, dialogs, and status presentation do not alter canonical machine state; | U41.01; U18.01; U25.01 | `tests/fixtures/ui/U41-threading.json`; `tests/fixtures/ui/U41-modal-time.json`; `tests/fixtures/ui/U18-view.json`; `tests/fixtures/ui/U25-status-panel.json` | `certification/results/ui/U48.AC44.json` |
| U48.AC45 | 2900 | cancellation and non-destructive error paths leave machine/media state unchanged where the operation has not committed; | U45.01 | `tests/fixtures/ui/U45-error-confirmation.json`; `tests/fixtures/ui/U45-no-silent-fallback.json` | `certification/results/ui/U48.AC45.json` |
| U48.AC46 | 2902 | destructive local media actions require explicit confirmation; | U15.01; U45.01 | `tests/fixtures/ui/U15-microdrive-manager.json`; `tests/fixtures/mdr/wzsn-mdr-write-protect.mdr`; `tests/fixtures/ui/U45-error-confirmation.json` | `certification/results/ui/U48.AC46.json` |
| U48.AC47 | 2903 | platform-specific menu relocation does not change semantic command IDs; | U07.01; U49.01 | `tests/fixtures/ui/U07-menu-tree.json`; `certification/gates/ui-phase12-menu-presentation.md`; `certification/manifests/ui-command-registry-v1.json` | `certification/results/ui/U48.AC47.json` |
| U48.AC48 | 2904 | the complete legacy-item disposition in Section 20 is represented in the backlog with no unclassified legacy menu command; | U20.01; C49.02 | `tests/fixtures/ui/U20-legacy-disposition.json`; `wzsn-architectures-1-2-developer-tasks.md` | `certification/results/ui/U48.AC48.json` |
| U48.AC49 | 2906 | Phase-12 UI toolkit selection documents static-link/single-binary fit, keyboard operation, and accessibility support; | U49.01 | `certification/gates/ui-phase12-toolkit.md`; `certification/gates/ui-phase12-accessibility.md`; `certification/gates/ui-phase12-platform-integration.md` | `certification/results/ui/U48.AC49.json` |
| U48.AC50 | 2908 | all Section 47 required regression tests applicable to the implemented milestone pass; | U47.01; U47.R001-U47.R086 | `tests/fixtures/ui/U47-required-regression-manifest.json`; `certification/results/ui/U47.01.json` | `certification/results/ui/U48.AC50.json` |
| U48.AC51 | 2910 | successful Telnet keyboard commands return the Section 27.1 response and invalid/held-key cases return the frozen error responses; | C55.03; U27.01 | `tests/fixtures/telnet/wzsn-telnet-key-events-v1.json`; `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt`; `tests/fixtures/ui/U27-telnet-grammar.txt` | `certification/results/ui/U48.AC51.json` |
| U48.AC52 | 2912 | reset/model changes invoked while paused leave the application paused; | U30.01; U32.01; U09.02-U09.04 | `tests/fixtures/ui/U30-reset.json`; `tests/fixtures/ui/U32-model.json`; `tests/fixtures/ui/U09-pause-master-time.json`; `tests/fixtures/ui/U09-reset-preservation.json` | `certification/results/ui/U48.AC52.json` |
| U48.AC53 | 2913 | `MENU TREE` uses the Section 35 record format and does not disclose dynamic Recent-file absolute paths; | U35.01; U04.03 | `tests/fixtures/ui/U35-menu-transcripts.txt`; `tests/fixtures/ui/U04-dynamic-registry-data.json`; `tests/fixtures/ui/U07-menu-tree.json` | `certification/results/ui/U48.AC53.json` |
| U48.AC54 | 2915 | `STATUS`/`DESCRIBE` do not expose arbitrary absolute host media paths under the initial unauthenticated policy; | U29.01; U36.01; U39.01 | `tests/fixtures/ui/U29-status-cases.json`; `tests/fixtures/ui/U36-describe.json`; `tests/fixtures/ui/U39-security.json` | `certification/results/ui/U48.AC54.json` |
| U48.AC55 | 2917 | `Tools > Snapshot Inspector...` exists and reuses shared state-inspection machinery rather than a separate snapshot-state implementation; | U19.01; U13.01 | `tests/fixtures/ui/U19-tools.json`; `tests/fixtures/ui/U13-snapshot-inspector.json`; `tests/fixtures/ui/U13-snapshot-editor-reuse.json` | `certification/results/ui/U48.AC55.json` |
| U48.AC56 | 2919 | every required Section 7.1 menu/action and non-menu semantic command ID is registered with the frozen metadata contract; | U07.01; U04.01 | `tests/fixtures/ui/U07-required-command-ids.json`; `tests/fixtures/ui/U07-menu-tree.json`; `tests/fixtures/ui/U04-command-registry.json` | `certification/results/ui/U48.AC56.json` |
| U48.AC57 | 2921 | the status line always exposes `Control Port: <number>` for an available listener or `Control Port: unavailable` after full range exhaustion; | U25.01 | `tests/fixtures/ui/U25-status-panel.json`; `tests/fixtures/ui/U25-control-port-interaction.json` | `certification/results/ui/U48.AC57.json` |
| U48.AC58 | 2923 | each WZSN process uses the first bindable candidate in 30740-32787 and simultaneous processes cannot acquire the same numeric Control Port; | C55.01; U23.06; U25.01 | `tests/fixtures/telnet/wzsn-control-port-race-v1.json`; `tests/fixtures/ui/U23-telnet-settings.json`; `tests/fixtures/ui/U25-status-panel.json` | `certification/results/ui/U48.AC58.json` |
| U48.AC59 | 2925 | the selected Control Port is session state and is never persisted; | U23.06; U44.01 | `tests/fixtures/ui/U23-telnet-settings.json`; `tests/fixtures/ui/wzsn-settings-valid.json`; `tests/fixtures/ui/U44-settings-race.json` | `certification/results/ui/U48.AC59.json` |
| U48.AC60 | 2926 | the Networking settings surface is one radio group with exactly `None`, `Interface-1`, and `Ear+Mic`, backed by `machine.networking.set`; | U23.02-U23.04 | `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/ui/U23-networking-transition.json`; `tests/fixtures/rom/zx48-certification.rom`; `tests/fixtures/rom/zx128-certification-0.rom` | `certification/results/ui/U48.AC60.json` |
| U48.AC61 | 2928 | `Interface-1` and `Ear+Mic` cannot be active simultaneously in UI state, registry state, or core state; | U23.02-U23.04 | `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/ui/U23-networking-transition.json`; `tests/fixtures/rom/zx48-certification.rom`; `tests/fixtures/rom/zx128-certification-0.rom` | `certification/results/ui/U48.AC61.json` |
| U48.AC62 | 2930 | `Ear+Mic` performs no networking-ROM paging and does not automatically install the Architecture-#3 resident RAM stack; | U23.02-U23.04 | `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/ui/U23-networking-transition.json`; `tests/fixtures/rom/zx48-certification.rom`; `tests/fixtures/rom/zx128-certification-0.rom` | `certification/results/ui/U48.AC62.json` |
| U48.AC63 | 2932 | networking-mode changes use the shared cold machine-reconfiguration path, do not preserve old Spectrum RAM/hooks/device state, and preserve only the application paused/running state; | U23.03 | `tests/fixtures/ui/U23-networking-transition.json`; `tests/fixtures/networking/wzsn-networking-modes-v1.json` | `certification/results/ui/U48.AC63.json` |
| U48.AC64 | 2935 | multi-instance settings writes, writable-media claims, and Telnet screenshot creation satisfy the interprocess safety contract without corrupting shared host data; | C28.03; U44.01; U34.01 | `tests/fixtures/concurrency/wzsn-multi-instance-plan-v1.json`; `tests/fixtures/ui/U44-settings-race.json`; `tests/fixtures/ui/U44-media-path-alias.json`; `tests/fixtures/ui/U44-output-race.json`; `tests/fixtures/ui/U34-screenshot-collisions.json` | `certification/results/ui/U48.AC64.json` |
| U48.AC65 | 2938 | `machine.networking.set` has an explicit remote-permission class and, when invoked through an allowed front end, reaches the same cold-reconfiguration handler as the Networking radio group; dirty Interface-1 media makes the remote command unavailable until resolved locally; | U23.03; U05.01-U05.03; U46.01 | `tests/fixtures/ui/U23-networking-transition.json`; `tests/fixtures/ui/U05-remote-permissions.json`; `tests/fixtures/ui/U46-equivalence.json` | `certification/results/ui/U48.AC65.json` |
| U48.AC66 | 2942 | in `Ear+Mic` mode the ordinary Tape transport cannot drive/consume the same cassette signals concurrently with the routed virtual network, and the Architecture-3 bootstrap action uses the explicit `BOOTSTRAP_TAPE` waveform path rather than RAM injection; | U23.05 | `tests/fixtures/networking/wzsn-ear-mic-bootstrap-test.tap`; `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `zx48-mic-ear-router-network-architecture.md` | `certification/results/ui/U48.AC66.json` |
| U48.AC67 | 2946 | `Ear+Mic` is unavailable on 128K or any 48K profile/variant not certified for Architecture #3's Issue-2 target, with a stable disabled reason and no machine-state mutation on a rejected request. | U23.04 | `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/rom/zx48-certification.rom`; `tests/fixtures/rom/zx128-certification-0.rom`; `zx48-mic-ear-router-network-architecture.md` | `certification/results/ui/U48.AC67.json` |
## U49. Architecture section 49: Phase-12 UI implementation gate

**Architecture authority:** `U` source lines 2952-2976, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Phase-12 UI implementation gate.

**Required artifacts:**

- `certification/gates/ui-phase12-toolkit.md`
- `certification/gates/ui-phase12-platform-integration.md`
- `certification/gates/ui-phase12-font-text.md`
- `certification/gates/ui-phase12-menu-presentation.md`
- `certification/gates/ui-phase12-file-dialog.md`
- `certification/gates/ui-phase12-accessibility.md`
- `certification/gates/ui-phase12-persistence.md`
- `certification/gates/ui-phase12-command-api.h`
- `certification/gates/ui-phase12-result-errors.md`

**Artifact/source authority:** Project gate artifacts; filenames are normative certification evidence.

**Tests:**

### U49.01

Before Phase-12 implementation tickets, require every named gate artifact above to freeze toolkit+exact revision, platform integration, font/text rendering, native-versus-in-window menu presentation per platform, file-dialog implementation, accessibility support, window/panel persistence, exact registry C API, and exact command result/error representation. Compare those implementation choices against Architecture #2 and prove none changes the frozen semantic menu tree, command IDs, manager ownership, Telnet grammar, or acceptance requirements. Phase 12 exits only after every applicable pre-Telnet GUI workflow and registry test passes.

**Evidence output:** `certification/results/ui/U49.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U50. Architecture section 50: Phase-15 Telnet-control gate

**Architecture authority:** `U` source lines 2977-2999, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Phase-15 Telnet-control gate.

**Required artifacts:**

- `certification/manifests/core-section55-acceptance.json`
- `certification/results/core/C55.05.json`
- `tests/fixtures/telnet/wzsn-telnet-command-lines-v1.txt`
- `tests/fixtures/ui/U27-telnet-grammar.txt`
- `tests/fixtures/ui/U35-menu-transcripts.txt`
- `tests/fixtures/ui/U36-describe.json`
- `tests/fixtures/ui/U37-do-cases.json`
- `tests/fixtures/ui/U05-remote-permissions.json`
- `tests/fixtures/ui/U30-reset.json`
- `tests/fixtures/ui/U31-pause-resume.json`
- `tests/fixtures/ui/U32-model.json`
- `tests/fixtures/ui/U33-speed.json`
- `tests/fixtures/video/wzsn-screenshot-raster-v1.rgba`
- `tests/fixtures/ui/U34-screenshot-collisions.json`
- `tests/fixtures/ui/U34-screenshot-failures.json`
- `tests/fixtures/ui/U25-status-panel.json`
- `tests/fixtures/ui/U46-equivalence.json`
- `certification/manifests/ui-command-registry-v1.json`

**Artifact/source authority:** Core Section-55 acceptance plus the exact project-owned application-control grammar, registry-projection, permission, command-equivalence, screenshot, and status fixtures required by Architecture #2 Section 50. Aggregate Phase-15 evidence is valid only after these lower-level artifacts pass.

**Tests:**

### U50.01

Require Core Section-55 transport/keyboard acceptance from `certification/manifests/core-section55-acceptance.json` plus `certification/results/core/C55.05.json`; special aliases from `tests/fixtures/ui/U27-telnet-grammar.txt`; MENU/DESCRIBE/DO registry projection from `tests/fixtures/ui/U35-menu-transcripts.txt`, `tests/fixtures/ui/U36-describe.json`, and `tests/fixtures/ui/U37-do-cases.json`; remote permission policy from `tests/fixtures/ui/U05-remote-permissions.json`; RESET/SPEED/MODEL/PAUSE/RESUME equivalence from the U30/U31/U32/U33 fixtures plus `tests/fixtures/ui/U46-equivalence.json`; screenshot temp-path/PNG/reporting from the U34 raster/collision/failure fixtures; and GUI/Telnet listener-status agreement from `tests/fixtures/ui/U25-status-panel.json` before Phase-15 exit. For every application-control command that works over Telnet, require evidence of the equivalent shared-registry and GUI/test path where applicable; a command may not be marked complete solely because its Telnet path succeeds.

**Evidence output:** `certification/results/ui/U50.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U51. Architecture section 51: Ticket-derivation contract for UI work

**Architecture authority:** `U` source lines 3000-3028, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Ticket-derivation contract.

**Required artifacts:**

- `certification/manifests/ui-ticket-contract.json`
- `wzsn-architectures-1-2-developer-tasks.md`

**Artifact/source authority:** Project backlog/ticket linter.

**Tests:**

### U51.01

Audit every UI implementation ticket in `wzsn-architectures-1-2-developer-tasks.md` against `certification/manifests/ui-ticket-contract.json` for every required field: architecture section(s), stable command ID(s), GUI surface(s), Telnet projection/permission class where applicable, core/system dependency, state availability predicate, disabled reason(s), parameter acquisition path, success result, cancel path, error path, machine-state/timing effect or explicit no-effect, keyboard accessibility path, and automated regression evidence. Reject any ticket that invents a new semantic command when an existing registry command covers it, and reject any UI-only presentation ticket that modifies core behavior to simplify implementation.

**Evidence output:** `certification/results/ui/U51.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U52. Architecture section 52: Initial non-goals

**Architecture authority:** `U` source lines 3029-3050, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Initial UI non-goals.

**Required artifacts:**

- `tests/fixtures/ui/U52-nongoals.json`

**Artifact/source authority:** Project registry/menu/feature-flag negative test.

**Tests:**

### U52.01

Assert each literal initial non-goal independently: no requirement to reproduce Turbo Vision appearance; DOS directory/shell functions; historical database application; every preservation converter in the first executable milestone; Timex/DCK UI before Timex support; live physical cassette capture before LATER promotion; mobile/touch-specific layout; WebAssembly/browser UI; plugin marketplace; arbitrary remote host-file operations; remote destructive media operations; remote WZSN termination; or ROM/BASIC text injection as a keyboard-matrix substitute. Where a non-goal is nevertheless implemented later, it must not be used to weaken any required initial workflow or security boundary.

**Evidence output:** `certification/results/ui/U52.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## U53. Architecture section 53: Frozen UI baseline

**Architecture authority:** `U` source lines 3051-3141, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Frozen UI baseline.

**Required artifacts:**

- `certification/manifests/ui-baseline.json`
- `tests/fixtures/ui/U07-menu-tree.json`
- `tests/fixtures/ui/U04-command-registry.json`
- `certification/manifests/ui-command-registry-v1.json`
- `tests/fixtures/ui/U24-toolbar.json`
- `tests/fixtures/ui/U33-speed.json`
- `tests/fixtures/networking/wzsn-networking-modes-v1.json`
- `tests/fixtures/ui/U23-networking-transition.json`
- `tests/fixtures/networking/wzsn-ear-mic-bootstrap-test.tap`
- `tests/fixtures/video/wzsn-screenshot-raster-v1.rgba`
- `tests/fixtures/ui/U34-screenshot-collisions.json`
- `tests/fixtures/telnet/wzsn-control-port-race-v1.json`
- `tests/fixtures/ui/U25-status-panel.json`
- `tests/fixtures/ui/U05-remote-permissions.json`
- `tests/fixtures/ui/U39-security.json`
- `tests/fixtures/concurrency/wzsn-multi-instance-plan-v1.json`
- `tests/fixtures/ui/U44-settings-race.json`
- `tests/fixtures/ui/U44-media-path-alias.json`
- `tests/fixtures/ui/U44-output-race.json`
- `tests/fixtures/ui/U20-legacy-disposition.json`
- `certification/manifests/ui-section48-acceptance.json`

**Artifact/source authority:** The baseline manifest is reconciled against Architecture #2 and independently against the exact lower-level project fixtures that prove each frozen UI group. The manifest cannot certify itself.

**Tests:**

### U53.01

Populate `certification/manifests/ui-baseline.json` with one explicit record for every literal frozen decision in Section 53, preserving its subsection/group and value. Reconcile each group against the named lower-level artifacts above: primary navigation against U07; interaction/registry architecture against U04; toolbar against U24; speed set/visibility against U33; networking/cold-reconfiguration/profile/cassette/bootstrap against U23; screenshot source/destination/naming/no-clobber against U34; Telnet/Control-Port and permission/security against C55/U25/U05/U39 artifacts; multi-instance host safety against C28/U44 artifacts; and legacy-UI migration against U20. Finally require `certification/manifests/ui-section48-acceptance.json` to show all applicable acceptance criteria passing. Missing or extra baseline records fail; any implementation drift requires architecture revision before certification.

**Evidence output:** `certification/results/ui/U53.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

# Part III - Architecture #3: ZX48 MIC/EAR correctness tests

## M01. Architecture section 1: Executive Summary

**Architecture authority:** `M` source lines 72-151, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Executive architecture / end-to-end concept.

**Required artifacts:**

- `tests/fixtures/micear/M01-system-topology.json`
- `certification/hardware/micear/M06-port-wiring-record.json`
- `certification/hardware/micear/M06-cross-port-isolation.csv`
- `tests/fixtures/micear/M01-router-responsibilities.json`
- `tests/fixtures/micear/M01-basic-forms.json`
- `tests/fixtures/micear/M04-lexical-conventions.json`
- `tests/fixtures/micear/zx48-micear-stack.tap`
- `tests/fixtures/micear/M28-async-message-vectors.json`
- `tests/fixtures/micear/M28-virtual-two-node.json`

**Artifact/source authority:** Project-owned virtual fixtures and resident-stack TAP; the physical active-star/no-tied-output claims use the real Issue-2/router-port measurements required by M06. `zx48-micear-stack.tap` is built from the same Spectrum-resident sources intended for physical distribution and is not an emulator-only substitute.

**Tests:**

### M01.01

Using `M01-system-topology.json` plus the M06 wiring/isolation records, prove the frozen physical topology: original ZX Spectrum 48K Issue-2 endpoint, MIC as uplink, EAR as downlink, signal plus return on each mono socket, one independent router port per station, active-star routing, no tied Spectrum MIC outputs, and no tied router EAR drivers.

**Evidence output:** `certification/results/micear/M01.01.json`

### M01.02

Using `M01-router-responsibilities.json`, exercise and independently observe the Raspberry Pi logical responsibilities named by the Executive Summary: routing, station naming, address resolution, message queueing, object/file staging, status management, and administration. Correlate transmitted/received pulse timestamps with controller evidence and prove Linux process scheduling never supplies Spectrum-facing pulse capture/generation timing.

**Evidence output:** `certification/results/micear/M01.02.json`

### M01.03

Using `M01-basic-forms.json`, parse and execute the Executive-Summary Interface-1-style forms with the exact literal selector vocabulary shown by the architecture: `FORMAT "n";...`, `SAVE *"n";... CODE`, `LOAD *"n";... CODE`, and `OPEN #...;"n";...`, followed by `PRINT #`/`CLOSE #` on the opened stream. Prove the accepted station-designator shape is `N = "x.y" or "AAAAAA"` subject to later frozen reservation rules, and prove neither a modern replacement selector nor modern commands such as `NET`, `MSG`, `WHO`, `FILES`, and `GET` are introduced as Spectrum-visible commands.

**Evidence output:** `certification/results/micear/M01.03.json`

### M01.04

Using `M04-lexical-conventions.json` and the router registry, prove station names are exactly six alphanumeric characters, canonicalized/stored according to the later Section-4/11 rules, unique within the router registry, and mapped to a two-byte area-plus-node address. Duplicate-name registration and malformed names must fail without corrupting the existing registry.

**Evidence output:** `certification/results/micear/M01.04.json`

### M01.05

Load `zx48-micear-stack.tap` through the ordinary pulse-level tape/bootstrap path into the certified 48K Issue-2 profile. Prove the high-memory RAM stack—not a paged networking ROM—hooks the normal 48K BASIC error path, recognizes the Interface-1-style front end from RAM, and that active Interface 1 is absent and cannot coexist or chain with the Ear+Mic BASIC extension.

**Evidence output:** `certification/results/micear/M01.05.json`

### M01.06

Using `M28-async-message-vectors.json` and `M28-virtual-two-node.json` with the resident stack loaded, commit an incoming message while the destination is not performing full-payload receive. Prove the router queues it asynchronously, the destination receives only the slow EAR attention indication in interrupt time, the resident interrupt detector sets the message-waiting flag without receiving the full payload, and full message data is retrieved later and cooperatively through the Interface-1-style network stream while precise EAR polling is active.

**Evidence output:** `certification/results/micear/M01.06.json`

**Pass rule:** PASS only when every M01.01-M01.06 result is exact and reproducible and together covers every frozen Executive-Summary claim. Any unexplained mismatch is FAIL. If a later section intentionally gates a value used by an Executive-Summary form (for example a reserved address range), that dependent assertion is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## M02. Architecture section 2: Scope

**Architecture authority:** `M` source lines 152-239, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Included/excluded scope, compatibility boundary, and Interface-1 mutual exclusion.

**Required artifacts:**

- `tests/fixtures/micear/M02-scope-included.json`
- `tests/fixtures/micear/M02-scope-excluded.json`
- `tests/fixtures/networking/wzsn-networking-modes-v1.json`
- `tests/fixtures/micear/zx48-micear-stack.tap`
- `tests/fixtures/micear/M17-command-semantics.json`
- `tests/fixtures/micear/M18-errors.json`
- `tests/fixtures/micear/M28-async-message-vectors.json`
- `tests/fixtures/micear/M28-transfer-vectors.json`
- `tests/fixtures/micear/M28-virtual-two-node.json`
- `certification/gates/micear/D10.md`
- `certification/gates/micear/D16.md`
- `certification/manifests/micear/D20-closed-contract.json`

**Artifact/source authority:** Project scope manifests generated directly from Architecture #3 Section 2, plus the named lower-level project fixtures. D10 controls not-yet-frozen reserved address ranges; D16 controls the final RAM footprint; the architecture-derived D20 closed-contract manifest records the frozen initial broadcast exclusion. Final electrical component values and final bit rate remain governed by their later electrical/timing gates and are tested as deliberately unresolved here rather than guessed.

**Tests:**

### M02.01 - included-scope ledger

Using `M02-scope-included.json`, require an explicit lower-level test/evidence mapping for every included Section-2.1 capability in source order: Issue-2-only physical target; two mono MIC/EAR signal+return connections; central active router; Raspberry Pi router; external deterministic timing/interface subsystem; tape-loaded high-memory stack; Interface-1-style BASIC front end; two-byte area+node addresses; unique six-character names; routed point-to-point communication; reserved address space only where frozen by D10; stream-oriented messages; `SAVE *`/`LOAD *`/`VERIFY *`/`MERGE *` object transfer; asynchronous queue+attention; and Interface-1-like success/error feedback. Exercise the named command, transfer, async, and two-node fixtures so the mapping is executable rather than documentation-only.

**Evidence output:** `certification/results/micear/M02.01.json`

### M02.02 - excluded-scope ledger

Using `M02-scope-excluded.json`, verify the initial implementation/certification does **not** claim or silently implement as Architecture-3 baseline behavior: 128K; +2/+2A/+2B/+3; Interface-1 electrical compatibility; simultaneous active Interface-1/Ear+Mic front ends; ZX-Net wire compatibility; replacement ROM hardware; expansion-port hardware; tied MIC outputs; shared analog cassette bus; full-duplex operation at one Spectrum; unsolicited complete-packet background reception; modern BASIC vocabulary; background multitasking; Internet routing; encryption; delivery to powered-off/disconnected Spectrums; or initial broadcast delivery. `D20-closed-contract.json` must reproduce the frozen initial broadcast exclusion unless a later explicit architecture revision supplies complete semantics. Also verify final electrical component values, final bit rate, and final RAM footprint are **not certified as frozen by Section 2**; the RAM-footprint assertion remains BLOCKED until D16 and the electrical/rate values remain governed by their named later gates.

**Evidence output:** `certification/results/micear/M02.02.json`

### M02.03 - compatibility boundary

Exercise `M17-command-semantics.json` over the MIC/EAR virtual path and prove compatibility is intentionally at the Interface-1 **user-facing programming model**, not Interface-1 physical networking: the Spectrum-visible syntax/feedback is IF1-like while transport remains routed cassette-level MIC/EAR signalling through an active star. No test result may label Architecture #3 as ZX-Net wire compatible or Interface-1 electrically compatible.

**Evidence output:** `certification/results/micear/M02.03.json`

### M02.04 - mutual-exclusion and no-ROM rule

Reject physical Architecture-3 certification on anything except documented ZX Spectrum 48K Issue 2. In WZSN exercise the exact single networking-mode state set `NONE`, `INTERFACE1`, and `EAR_MIC`: `NONE` and `INTERFACE1` must disconnect Architecture #3, while Architecture-3 fidelity is attachable only in `EAR_MIC` on the Architecture-3-certified 48K Issue-2 profile. Prove `INTERFACE1` and `EAR_MIC` cannot coexist, and selecting `EAR_MIC` guarantees Interface 1, Microdrive, and original ZX Net are inactive. Selecting `EAR_MIC` alone must not install the resident stack and must never page an Architecture-3 ROM. In the separate execution case that explicitly loads `zx48-micear-stack.tap`, prove the RAM-resident BASIC front end operates with physical/virtual Interface 1 absent, with no handler chaining, shadow-ROM cooperation, or dual ownership of the `"n"` syntax.

**Evidence output:** `certification/results/micear/M02.04.json`

**Pass rule:** PASS only when M02.01-M02.04 together account for every included capability, every exclusion, the compatibility statement, and the mutual-exclusion rule exactly as frozen. Any unclaimed future feature must remain absent or explicitly outside certification. Any unexplained mismatch is FAIL; any value intentionally deferred by the architecture is BLOCKED_GATE until its named gate exists and may not be guessed.

## M03. Architecture section 3: Goals and Non-goals

**Architecture authority:** `M` source lines 240-312, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** One-for-one proof of G1-G12 and NG1-NG10.

**Required artifacts:**

- `tests/fixtures/micear/M03-goals-nongoals.json`
- `tests/fixtures/micear/M02-scope-included.json`
- `tests/fixtures/micear/M02-scope-excluded.json`
- `certification/hardware/micear/M06-port-wiring-record.json`
- `certification/hardware/micear/M06-cross-port-isolation.csv`
- `tests/fixtures/micear/M05-component-boundaries.json`
- `tests/fixtures/micear/M09-controller-stress.json`
- `tests/fixtures/micear/M11-address-registration.json`
- `tests/fixtures/micear/M17-command-semantics.json`
- `tests/fixtures/micear/M18-errors.json`
- `tests/fixtures/micear/M28-async-message-vectors.json`
- `tests/fixtures/micear/M19-attention.json`
- `tests/fixtures/micear/zx48-micear-stack.tap`
- `tests/fixtures/micear/M22-stack-api.json`
- `tests/fixtures/micear/M29-two-node-virtual.json`
- `tests/fixtures/networking/wzsn-networking-modes-v1.json`

**Artifact/source authority:** `M03-goals-nongoals.json` is the source-order objective ledger generated from Section 3. Each row below names the concrete lower-level project artifact that must produce evidence; the ledger itself is not allowed to substitute for execution of the referenced proof.

**Tests:**

### M03.01 - literal G/NG objective ledger

| Objective | Frozen requirement | Required proof artifact(s) |
|---|---|---|
| G1 | unmodified Issue-2 48K Spectrum | `M02-scope-included.json`; `M06-port-wiring-record.json` |
| G2 | runtime link uses only MIC/EAR cassette sockets | `M06-port-wiring-record.json` |
| G3 | many Spectrums connect safely without tied outputs | `M06-cross-port-isolation.csv` |
| G4 | Interface-1-like user experience | `M17-command-semantics.json` |
| G5 | human-readable unique station names | `M11-address-registration.json` |
| G6 | compact hierarchical area+node addresses | `M11-address-registration.json` |
| G7 | meaningful success plus malformed/unknown/unavailable/busy/timeout/corruption/version/rejected-transfer/verification feedback | `M18-errors.json` |
| G8 | incoming messages queue asynchronously at router | `M28-async-message-vectors.json` |
| G9 | attention notification without command-line/BASIC/compatible-state corruption while resident interrupt contract remains active | `M19-attention.json`; `zx48-micear-stack.tap` |
| G10 | stack distributed and installed as tape | `zx48-micear-stack.tap` |
| G11 | reusable network core separated from BASIC front end | `M05-component-boundaries.json`; `M22-stack-api.json` |
| G12 | same protocol usable virtually through MIC edge/EAR level model | `M29-two-node-virtual.json` |
| NG1 | do not reproduce Interface-1 hardware bus | `M02-scope-excluded.json` |
| NG2 | do not reproduce every Interface-1 firmware bug | `M02-scope-excluded.json` |
| NG3 | do not add Spectrum multitasking | `M02-scope-excluded.json` |
| NG4 | do not receive complete packet in background | `M28-async-message-vectors.json`; `M19-attention.json` |
| NG5 | no arbitrary user-selected source address without router validation | `M11-address-registration.json` |
| NG6 | Pi GPIO/Linux is not the real-time pulse engine | `M09-controller-stress.json` |
| NG7 | no modern filesystem shell on Spectrum | `M02-scope-excluded.json` |
| NG8 | no commands outside Interface-1 front end | `M17-command-semantics.json` |
| NG9 | no transparent coexistence/chaining with active Interface 1 | `wzsn-networking-modes-v1.json`; `zx48-micear-stack.tap` |
| NG10 | no dedicated Ear+Mic networking ROM; resident RAM stack remains implementation | `wzsn-networking-modes-v1.json`; `zx48-micear-stack.tap` |

For each row, execute the referenced lower-level proof and require `M03-goals-nongoals.json` to record the lower-level test ID, result artifact, and PASS/BLOCKED_GATE state. A row cannot be marked PASS from documentation review alone. Negative objectives must include an active negative test or implementation-boundary inspection demonstrating the forbidden behavior is absent; absence from the UI alone is insufficient.

**Evidence output:** `certification/results/micear/M03.01.json`

**Pass rule:** PASS only when all 22 G/NG rows have executable lower-level evidence and no objective is inferred from another merely similar requirement. Any unexplained mismatch is FAIL. A lower-level decision intentionally deferred by the architecture propagates BLOCKED_GATE; no objective may be declared met by assuming the deferred production behavior.

## M04. Architecture section 4: Terms and Conventions

**Architecture authority:** `M` source lines 313-444, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** One-for-one validation of the Section-4 terminology and lexical contracts.

**Required artifacts:**

- `tests/fixtures/micear/M04-lexical-conventions.json`
- `tests/fixtures/micear/M02-scope-included.json`
- `certification/manifests/micear/D12-closed-contract.json`
- `tests/fixtures/micear/M05-component-boundaries.json`
- `certification/hardware/micear/M06-port-wiring-record.json`
- `tests/fixtures/micear/M09-controller-stress.json`
- `tests/fixtures/micear/M11-address-registration.json`
- `tests/fixtures/micear/M17-command-semantics.json`
- `tests/fixtures/micear/M28-transfer-vectors.json`
- `tests/fixtures/micear/M28-async-message-vectors.json`

**Artifact/source authority:** Project lexical and lower-level behavioral fixtures. D12 is the later architecture decision that closes lowercase handling; Section 4 itself freezes uppercase canonical storage and the six-character alphabet but does not independently create a lowercase policy.

**Tests:**

### M04.01 - literal term/convention ledger

| Term | Required proof | Artifact(s) |
|---|---|---|
| Spectrum | unqualified `Spectrum` in Architecture #3 resolves to original 48K Issue 2 | `M04-lexical-conventions.json`; `M02-scope-included.json` |
| Router | logical router service is Raspberry-Pi-side service, not timing controller or Spectrum stack | `M05-component-boundaries.json` |
| Timing controller | external deterministic hardware converts conditioned MIC edges↔framed bytes and framed bytes↔conditioned EAR edges | `M09-controller-stress.json` |
| Router port | one independent bidirectional logical connection contains MIC RX, EAR TX, port identity, status counters, and queue binding | `M05-component-boundaries.json`; `M06-port-wiring-record.json` |
| Station | exactly one Spectrum registered with router | `M11-address-registration.json` |
| Station name | exactly six characters from A-Z/0-9; canonical storage uppercase; valid and invalid examples behave exactly as specified | `M04-lexical-conventions.json`; `M11-address-registration.json` |
| Station address | exactly two logical bytes (area,node), rendered as decimal `x.y`, including 1.1/1.3/12.42 vectors | `M04-lexical-conventions.json`; `M11-address-registration.json` |
| Station designator N | quoted ordinary-station form is exactly `"x.y"` or `"AAAAAA"`; `"ROUTER"` and `"0.0"` are router endpoints, never ordinary stations | `M04-lexical-conventions.json`; `M11-address-registration.json` |
| Message | router-staged character stream becomes committed by closing an output channel | `M17-command-semantics.json`; `M28-async-message-vectors.json` |
| Spectrum object | transferable classes are BASIC, BASIC+LINE, numeric array, character array, CODE, SCREEN$ under normal SAVE/LOAD semantics | `M28-transfer-vectors.json`; `M17-command-semantics.json` |
| Asynchronous | router may accept/queue and signal pending traffic asynchronously, but full Spectrum payload retrieval is cooperative and does not occur while unrelated code continues | `M28-async-message-vectors.json` |

The lexical fixture must include the architecture's named valid station examples (`MASTER`, `GAME01`, `LAB002`, `ZX0048`) and invalid examples (`GAME1`, `GAME-1`, `GAME_1`, `GAME001`) plus ordinary designators `"1.3"` and `"GAME02"`. Uppercase canonical storage is required directly by Section 4. If lowercase input is exercised, its normalization is accepted only because D12 is explicitly closed to normalize before validation; the result must cite `certification/manifests/micear/D12-closed-contract.json` rather than attributing that rule to Section 4.

**Evidence output:** `certification/results/micear/M04.01.json`

**Pass rule:** PASS only when all eleven terms have the exact Section-4 meaning and executable evidence. A parser or implementation using a different definition under the same term is FAIL even if later end-to-end tests happen to pass. Any later-gated behavior remains governed by its named gate and is not inferred from terminology.

## M05. Architecture section 5: System Architecture

**Architecture authority:** `M` source lines 445-524, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Top-level layer boundaries, router trust model, and one-for-one responsibility ownership.

**Required artifacts:**

- `tests/fixtures/micear/M05-component-boundaries.json`
- `tests/fixtures/micear/M05-trust-boundary-vectors.json`
- `tests/fixtures/micear/M09-controller-stress.json`
- `tests/fixtures/micear/M10-router-api.json`
- `tests/fixtures/micear/M11-address-registration.json`
- `tests/fixtures/micear/M14-packet-vectors.json`
- `tests/fixtures/micear/M22-stack-api.json`
- `certification/gates/micear/D07.md`
- `certification/gates/micear/D08.md`
- `certification/gates/micear/D36.md`
- `certification/gates/micear/P0-protocol-negotiation.md`

**Artifact/source authority:** Project component instrumentation and adversarial packet vectors. D07/D08/D36 and the Phase-0 negotiation artifact freeze the packet-integrity/size/header/version facts needed to distinguish trusted validated frames from untrusted input; tests do not invent those values before their gates.

**Tests:**

### M05.01 - top-level layer contract

Instrument one end-to-end transaction and prove it crosses the frozen layer order without bypass: BASIC front end -> resident high-memory stack -> packet/link services -> Issue-2 port-FE cassette interface -> independent router-port electronics -> deterministic timing controller -> Raspberry Pi router. Verify the BASIC vocabulary, framing/sequence/CRC/retry/turnaround services, MIC output through port FE, EAR input through port-FE bit 6, and router registry/resolution/queue/routing/storage/admin layers are owned by the named component rather than collapsed into an emulator-only shortcut.

**Evidence output:** `certification/results/micear/M05.01.json`

### M05.02 - router trust boundary

After D07/D08/D36 and Phase-0 protocol negotiation are frozen, feed `M05-trust-boundary-vectors.json` through the real router codec path. Accept identity only from the physical/virtual router-port binding plus router registry; accept frame data only after required CRC validation. Deliberately spoof Spectrum-supplied source address and source name, malformed/unvalidated packet length, unsupported/unvalidated protocol version, CRC corruption, and payload over the configured limit. Each must be rejected or normalized according to the frozen protocol without allowing the client to select an authenticated source identity.

**Evidence output:** `certification/results/micear/M05.02.json`

### M05.03 - Spectrum responsibility boundary

Using `M22-stack-api.json` plus pulse-level integration, prove the Spectrum side: generates/receives pulse-coded traffic; presents Interface-1-like BASIC semantics; stores only the bounded active state/buffers assigned to it; explicitly initiates full-payload receive rather than background reception; and exposes the frozen callable machine-code API. Router persistence, global registry, and Linux/host resources must not leak into resident Spectrum state.

**Evidence output:** `certification/results/micear/M05.03.json`

### M05.04 - timing-controller responsibility boundary

Using `M09-controller-stress.json`, prove the timing controller measures MIC edges, generates EAR edges, owns deterministic per-port timing, prevents Linux scheduling jitter from entering the physical layer, and reports link diagnostics. It must not become the authoritative station registry, message store, object store, or administrative service.

**Evidence output:** `certification/results/micear/M05.04.json`

### M05.05 - Raspberry-Pi responsibility boundary

Using `M10-router-api.json` and `M11-address-registration.json`, prove the Raspberry Pi router owns station registry/uniqueness, name->area.node mapping, port->authenticated-station mapping, routing, message queues, object staging, status, administration, and logs. Correlate with controller traces to prove these logical responsibilities do not move Spectrum-facing edge timing into ordinary Linux process scheduling.

**Evidence output:** `certification/results/micear/M05.05.json`

**Pass rule:** PASS only when M05.01-M05.05 establish the exact frozen layer and trust/responsibility boundaries. Any cross-layer shortcut, spoofable source identity, unvalidated frame acceptance, or Linux-owned pulse timing is FAIL. Packet values intentionally deferred remain BLOCKED_GATE until the named gate artifact is frozen.

## M06. Architecture section 6: Physical Topology

**Architecture authority:** `M` source lines 525-589, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Physical active-star topology.

**Required artifacts:**

- `certification/hardware/micear/M06-port-wiring-record.json`
- `certification/hardware/micear/M06-cross-port-isolation.csv`

**Artifact/source authority:** Required real Issue-2 hardware + production/prototype router ports; oscilloscope/logic capture artifacts are project measurements.

**Tests:**

### M06.01

Bench-verify two mono TS connections per station (Spectrum MIC->router RX, router TX->Spectrum EAR), shared sleeve/return treatment, no tied MIC outputs, independent active-star paths, half-duplex per Spectrum, and simultaneous activity on separate station ports without waveform/identity contamination.

**Evidence output:** `certification/results/micear/M06.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## M07. Architecture section 7: Spectrum-side Electrical Boundary

**Architecture authority:** `M` source lines 590-666, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Spectrum port-FE electrical boundary, conditioning requirements, and prototype/production grounding authority.

**Required artifacts:**

- `certification/gates/micear/D01.md`
- `certification/gates/micear/D02.md`
- `tests/fixtures/micear/zx48-micear-diagnostic.tap`
- `certification/hardware/micear/M07-portfe-waveform-correlation.csv`
- `certification/hardware/micear/M07-input-impedance.csv`
- `certification/hardware/micear/M07-protection-levels.csv`
- `certification/hardware/micear/M07-amplitude-insertion-stress.csv`
- `certification/hardware/micear/M07-grounding-power-record.json`

**Artifact/source authority:** Project diagnostic TAP plus real unmodified Issue-2 bench captures. D01 freezes the analog prototype schematic/protection limits. D02 separately freezes the production isolation method and ratings; the proof must not treat prototype shared-ground permission as the production decision.

**Tests:**

### M07.01 - frozen Spectrum-side signal source/sink

Run `zx48-micear-diagnostic.tap` on an unmodified Issue-2 machine and correlate Z80/port-FE activity with physical socket capture. Prove uplink transitions originate from the Spectrum cassette/audio output controlled through port FE and appear as the MIC waveform; prove downlink is observed by the Spectrum specifically through EAR state on port-FE bit 6 while the timing-sensitive polling loop runs. No expansion-port or replacement-ROM path may satisfy this test.

**Evidence output:** `certification/results/micear/M07.01.json`

### M07.02 - analog conditioning and protection

BLOCKED until D01 is frozen. Against the D01 schematic, measure the complete uplink receive requirements: high input impedance; AC coupling when required by the frozen circuit; input protection; amplitude limiting; threshold detection; hysteresis; plug-insertion-transient tolerance; and tolerance across the frozen modest amplitude-variation test range. Measure the complete downlink requirements: controlled audio-level output; required AC coupling; current limiting; output protection; repeatable transition timing; adjustable/selectable bring-up amplitude; and safe plug insertion/removal. Explicitly inspect the physical netlist/PCB and prove no Raspberry Pi, MCU, CPLD, or FPGA raw logic pin connects directly to MIC or EAR.

**Evidence output:** `certification/results/micear/M07.02.json`

### M07.03 - grounding and isolation

For a supervised small prototype, `M07-grounding-power-record.json` must document verified power-supply relationships before a shared router-side ground is used; the prototype test must not generalize that permission to production. Production/release certification is BLOCKED until D02 freezes the final isolation method, isolation voltage, isolated-power topology, and connector-protection network. After D02, inject the approved common-mode/ground-fault cases and prove the production design prevents ground-loop/fault propagation according to the frozen ratings.

**Evidence output:** `certification/results/micear/M07.03.json`

**Pass rule:** PASS only when M07.01-M07.03 prove the exact port-FE boundary and every named conditioning/safety requirement at the applicable prototype or production gate. A raw logic connection, missing named protection/conditioning behavior, or production certification based only on the provisional shared-ground prototype is FAIL. Unfrozen D01/D02-dependent values remain BLOCKED_GATE.

## M08. Architecture section 8: Router Port Electronics

**Architecture authority:** `M` source lines 667-744, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Exact router-port receive/transmit chains, status observability, isolation, and phase-specific port scaling.

**Required artifacts:**

- `certification/gates/micear/D01.md`
- `certification/gates/micear/D02.md`
- `certification/gates/micear/D03.md`
- `certification/gates/micear/D04.md`
- `certification/hardware/micear/M08-port-chain-netlist.json`
- `certification/hardware/micear/M08-status-signals.json`
- `certification/hardware/micear/M08-fault-isolation.csv`
- `certification/hardware/micear/M08-port-count-builds.json`

**Artifact/source authority:** Project schematic/netlist, PCB/cable specification, controller/router status capture, and real bench measurements. D01 freezes the analog port schematic; D02/D03 freeze production isolation/cabling; D04 freezes production/reference port count where Section 8 leaves it open.

**Tests:**

### M08.01 - uplink receive chain

BLOCKED until D01 supplies the applicable schematic. Verify by netlist inspection and injected MIC waveform that the receive path is ordered exactly as the frozen functional chain: Spectrum MIC -> protection/current limiting -> AC coupling or level restoration -> amplifier/high-gain buffer -> Schmitt comparator -> digital edge-capture input. Each functional stage must be identifiable in the implementation or an explicitly proven equivalent circuit; no raw bypass may skip the required conditioning/protection semantics.

**Evidence output:** `certification/results/micear/M08.01.json`

### M08.02 - downlink transmit chain

BLOCKED until D01 supplies the applicable schematic. Verify by controller-driven waveform and netlist inspection that the transmit path is ordered functionally as: timer/waveform engine -> logic output -> level conversion/waveform shaping -> current-limited protected driver -> Spectrum EAR. Measure the final EAR waveform and correlate its transitions with controller timestamps; a logic-level direct-drive bypass is FAIL.

**Evidence output:** `certification/results/micear/M08.02.json`

### M08.03 - mandatory per-port status

Using `M08-status-signals.json`, exercise one port through disconnected, idle, receive, framing-error, CRC-error, queued-downlink, active-downlink, bound-station, and threshold-diagnostic states. Prove the port exposes at minimum: cable-present/inferred-activity; last MIC-edge timestamp; current decoded state; uplink framing-error count; uplink CRC-error count; downlink queue depth; downlink active state; current station binding; and analog-threshold diagnostics when the selected hardware provides them. Status counters/identity must be per-port and must not alias another port.

**Evidence output:** `certification/results/micear/M08.03.json`

### M08.04 - isolation and phase-specific port count

For release safety, BLOCKED until D02/D03 and applicable D04 are frozen. Inject shorts/opens/ground offsets/faults within approved bench limits and prove one port cannot back-drive or corrupt another and that the frozen cable/isolation ratings are met. Separately verify the implementation sequence from `M08-port-count-builds.json`: Phase 2 permits one supervised diagnostic port; Phase 3 requires two independent ports; Phase 5 reference build provides at least four; Phase 7 production PCB count is the value frozen under D04 from measured controller/isolation/power/connector/cost constraints. Confirm the logical protocol contains no embedded fixed physical port count and scales by replication of the independent port contract.

**Evidence output:** `certification/results/micear/M08.04.json`

**Pass rule:** PASS only when M08.01-M08.04 prove the complete functional chains, all required status observability, independent-port safety, protocol port-count independence, and the exact phase-specific scaling rule. Any unfrozen D01/D02/D03/D04-dependent production fact remains BLOCKED_GATE rather than being inferred from a prototype.

## M09. Architecture section 9: Deterministic Timing Controller

**Architecture authority:** `M` source lines 745-828, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Real-time ownership, complete per-port controller responsibilities, Pi/controller framing, and bounded buffering/failure semantics.

**Required artifacts:**

- `certification/gates/micear/D41.md`
- `certification/gates/micear/D42.md`
- `certification/gates/micear/D44.md`
- `certification/gates/micear/D17.md`
- `tests/fixtures/micear/M09-controller-stress.json`
- `tests/fixtures/micear/M09-controller-protocol-vectors.json`
- `tests/fixtures/micear/M09-attention-mode-vectors.json`
- `tests/fixtures/micear/M09-controller-fault-injection.json`
- `certification/hardware/micear/M09-linux-jitter-stress.csv`

**Artifact/source authority:** Project controller firmware/hardware, deterministic synthetic edge/protocol/fault vectors, and measured timing captures. D41 selects the prototype timing-controller technology; D42 freezes the Pi/controller transport and digital framing; D44 freezes phase-applicable buffering/flow-control capacity. D17 is required only when certifying the exact final attention waveform, not to prove that the controller owns the attention-generation/suppression state machine.

**Tests:**

### M09.01 - deterministic real-time owner and technology independence

BLOCKED until D41 selects the prototype controller technology. Run pulse capture/generation while imposing controlled Linux scheduling stalls from `M09-linux-jitter-stress.csv`; prove ordinary Pi/Linux user space neither measures nor generates the Spectrum cassette pulse train. The chosen MCU timer/input-capture, RP2040 PIO, CPLD, FPGA, or other deterministic device may implement the real-time layer, but the on-wire protocol and resident stack behavior must not depend on vendor-only timing semantics absent from the architecture.

**Evidence output:** `certification/results/micear/M09.01.json`

### M09.02 - complete per-port controller responsibilities

Using `M09-controller-stress.json` and `M09-attention-mode-vectors.json`, prove for every exercised port that the controller: timestamps MIC transitions; classifies pulse widths; detects pilot/sync/framing; assembles validated link frames or reports frame errors; queues received frames to the Pi; accepts outbound frames from the Pi; emits deterministic EAR transitions; owns slow-attention generation; suppresses attention while packet RX, packet TX, or router-generated tape/bootstrap is active; enforces the frozen idle state; enters safe idle on Pi/controller transport loss, controller reset, watchdog expiry, or unrecoverable underrun; and reports overrun, underrun, reset, and link-loss faults. Exact attention pulse values remain BLOCKED until D17, but ownership/suppression behavior must pass before then.

**Evidence output:** `certification/results/micear/M09.02.json`

### M09.03 - Pi/controller transport and framing

BLOCKED until D42 is frozen. Exercise the selected SPI/USB/UART/other framed digital transport with `M09-controller-protocol-vectors.json`. Every accepted controller-protocol record must carry the frozen equivalents of port number, operation type, frame length, frame payload, monotonic sequence number, controller status, and CRC/other integrity check. Inject wrong port, wrong operation, truncated/oversized length, sequence discontinuity, status fault, and bad integrity field; each must fail deterministically without creating Spectrum-facing pulse corruption or cross-port identity confusion.

**Evidence output:** `certification/results/micear/M09.03.json`

### M09.04 - buffering, flow control, and explicit exhaustion

At the maximum physical rate applicable to the phase, BLOCKED until D44 freezes the capacity/flow-control contract. Use `M09-controller-fault-injection.json` to saturate RX and TX paths while injecting short Linux stalls. Prove complete timing-critical work remains buffered independently of those stalls; capacity meets the frozen stress requirement; flow control is bounded; and exhaustion/underrun fails the affected transaction explicitly and drives safe idle as required rather than corrupting a partially emitted pulse train. Fault reporting must identify the applicable overrun/underrun/reset/link-loss condition.

**Evidence output:** `certification/results/micear/M09.04.json`

**Pass rule:** PASS only when M09.01-M09.04 cover every Section-9 responsibility and the selected transport/capacity gates. Linux-owned pulse timing, omitted controller status/fault behavior, unvalidated digital framing, or silent pulse corruption on buffer exhaustion is FAIL. Unfrozen D41/D42/D44/D17-dependent values remain BLOCKED_GATE.

## M10. Architecture section 10: Raspberry Pi Router Software

**Architecture authority:** `M` source lines 829-930, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Router-daemon module ownership, complete logical function set, administration boundary, and persistent atomicity.

**Required artifacts:**

- `certification/gates/micear/D24.md`
- `certification/gates/micear/D28.md`
- `certification/gates/micear/D35.md`
- `certification/gates/micear/D38.md`
- `certification/gates/micear/D50.md`
- `tests/fixtures/micear/M10-module-boundaries.json`
- `tests/fixtures/micear/M10-router-api.json`
- `tests/fixtures/micear/M10-admin-vectors.json`
- `tests/fixtures/micear/zx48-router-test.sqlite`
- `tests/fixtures/micear/M10-persistence-crash.json`

**Artifact/source authority:** Project daemon/test database and deterministic API/admin/crash vectors. D35 freezes router daemon/module decomposition, D50 its language/build system, D28 the administrative mechanism/reachability policy, D38 the concrete persistence mapping, and D24 the phase-applicable restart/database recovery behavior.

**Tests:**

### M10.01 - process model and module responsibilities

After D35/D50, inspect and instrument the primary `zx48-routerd` build. Prove one logical router daemon owns the required architectural responsibilities and that the frozen module decomposition accounts for: port manager, station registry, name resolver, address allocator, packet router, stream/message service, object transfer service, queue manager, storage manager, timing-controller adapter, administration API, and diagnostics/logging. Exact source-file split may follow D35, but no responsibility may disappear or be silently moved into Spectrum firmware or controller timing code.

**Evidence output:** `certification/results/micear/M10.01.json`

### M10.02 - complete required router function set

Using `M10-router-api.json`, execute vectors for all Section-10.2 functions: enforce unique six-character names; name->address and address->name mapping; physical/virtual port->station mapping; distinguish unknown from known-but-unavailable; direct routing; router-local services; queue incoming messages for online stations not currently receiving; notify queued destinations; validate packet length and CRC; prevent source impersonation; and return status/error responses. Every vector must pass through the same production router core used by virtual and physical paths.

**Evidence output:** `certification/results/micear/M10.02.json`

### M10.03 - administrative boundary and Phase-5 minimum operations

BLOCKED at the applicable point until D28 is frozen. Prove the initial administrative path is local-only (for example local CLI/loopback IPC) and is outside the Spectrum BASIC front end. If any network-reachable web/management path exists, require explicit authentication/authorization and prove it is not an unauthenticated wildcard listener. Using `M10-admin-vectors.json`, exercise all Phase-5 minimum operations: port enable/disable; station-name assignment; address assignment; conflict resolution; queue inspection; log inspection; protocol-version control; timing-hardware firmware update; and link diagnostics. Unauthorized or Spectrum-side attempts must not acquire administrative authority.

**Evidence output:** `certification/results/micear/M10.03.json`

### M10.04 - persistence and atomic visibility

BLOCKED for the concrete schema until D38 and for the applicable restart semantics until D24. Using `zx48-router-test.sqlite` and `M10-persistence-crash.json`, prove the baseline persistent store represents, as applicable, stations, committed messages, object-staging metadata, router configuration, audit-log metadata, and protocol metadata. Whether payload is a database blob or file referenced by metadata, inject crashes/short writes between every metadata/payload commit boundary and prove metadata never exposes a committed message/object whose required payload is missing or partial. Restart must follow the phase-frozen D24 recovery policy rather than inventing ad-hoc repair semantics.

**Evidence output:** `certification/results/micear/M10.04.json`

**Pass rule:** PASS only when M10.01-M10.04 prove the full daemon responsibility set, every required logical function, the administration trust boundary/operation set, and atomic persistent visibility. Missing required router function, unauthenticated network administration, or committed metadata pointing at incomplete payload is FAIL. D24/D28/D35/D38/D50-dependent details remain BLOCKED_GATE until frozen.

## M11. Architecture section 11: Station Identity and Addressing

**Architecture authority:** `M` source lines 931-1078, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Station-name/address grammar, explicit registry mapping, reserved endpoints, authenticated source identity, and registration/rebinding gates.

**Required artifacts:**

- `certification/gates/micear/D10.md`
- `certification/gates/micear/D11.md`
- `certification/manifests/micear/D12-closed-contract.json`
- `certification/gates/micear/D47.md`
- `tests/fixtures/micear/M11-address-registration.json`
- `tests/fixtures/micear/M05-trust-boundary-vectors.json`

**Artifact/source authority:** Project registration/spoofing vectors. D12 is closed to lowercase->uppercase normalization before validation. D10 freezes remaining reservations. D11 freezes production reassignment/allocation policy; D47 additionally governs pending-data safety when station identity is administratively replaced/rebound.

**Tests:**

### M11.01 - names, addresses, and explicit mapping

Validate station names as exactly six A-Z/0-9 characters, unique in the router naming domain, canonically stored uppercase, and never `ROUTER`. Using the closed D12 decision, lowercase input must normalize to uppercase **before** validation and compare/storage remain uppercase. Validate area and node as independent bytes 0..255 displayed in decimal `area.node`. Prove the registry stores an explicit bidirectional mapping such as `GAME02 <-> 1.3`; no algorithm may mathematically derive the name from the address or vice versa.

**Evidence output:** `certification/results/micear/M11.01.json`

### M11.02 - router endpoint and remaining reservation gate

Prove `"ROUTER"` and `"0.0"` designate the same local router service and neither may be assigned to an ordinary station. Remaining x.0/x.1-254/x.255/255.x/255.255 behavior is BLOCKED until D10 freezes the reservation plan; test exactly D10 afterward. Regardless of the reservation choice, initial broadcast **delivery** remains excluded unless the architecture is explicitly revised with complete broadcast command/routing/queue/error semantics.

**Evidence output:** `certification/results/micear/M11.02.json`

### M11.03 - physical/virtual-port source authentication

Feed valid and spoofed uplinks from distinct bound ports. The router must know the port on which each uplink arrived and must not trust a Spectrum-supplied source name/address. For the initial protocol, prove the uplink routed-packet header omits the source address and the router inserts/derives source identity from the authenticated port binding. A packet carrying a client-selected source field as though a future protocol version were active must be rejected as unsupported/malformed under the initial format; no future asserted-source behavior is certified by this test.

**Evidence output:** `certification/results/micear/M11.03.json`

### M11.04 - FORMAT registration and production-policy gate

Exercise `FORMAT "n";N` with the architecture examples `"MASTER"` and `"1.1"`. For the Phase-1..3 baseline, preconfigure each physical/virtual port with its permitted name/address and prove FORMAT only confirms/activates that binding; it cannot claim another port's identity. Ordinary network operations before successful local identity establishment must fail under the later error-precedence contract, while FORMAT itself remains exempt from the Station-not-set precondition. Any dynamic allocation, administrative reassignment, station move, or rebinding test is BLOCKED until D11; any such operation involving pending messages/objects is additionally BLOCKED until D47.

**Evidence output:** `certification/results/micear/M11.04.json`

**Pass rule:** PASS only when M11.01-M11.04 prove exact lexical/storage/address behavior, explicit mapping, router reservation, port-authenticated source identity, and the baseline registration semantics. D10/D11/D47-dependent production behavior must remain BLOCKED_GATE until frozen and may not be inferred from the prototype baseline.

## M12. Architecture section 12: Link Operation

**Architecture authority:** `M` source lines 1079-1170, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Link direction, Spectrum-initiated receive, turnaround, queue/offline behavior, and busy/liveness semantics.

**Required artifacts:**

- `certification/gates/micear/D33.md`
- `certification/gates/micear/D39.md`
- `certification/gates/micear/D48.md`
- `tests/fixtures/micear/M12-link-liveness.json`
- `tests/fixtures/micear/M12-queue-busy.json`
- `tests/fixtures/micear/M28-virtual-two-node.json`
- `certification/hardware/micear/M12-turnaround.csv`

**Artifact/source authority:** Project virtual link/queue/liveness vectors plus real Issue-2 turnaround captures. D33 freezes the request/response guard; D48 freezes station liveness/unavailable policy; D39 is required only for the queue/staging-resource refusal case named as one possible busy cause.

**Tests:**

### M12.01 - direction and Spectrum-initiated full downlink

Using `M28-virtual-two-node.json`, prove uplink is Spectrum MIC -> router RX and downlink is router TX -> Spectrum EAR. Queue a complete response while the destination Spectrum is executing unrelated code and prove the router does **not** transmit the full packet until the Spectrum has entered a known receive operation and its timing-sensitive EAR polling loop is active. Attention signalling is not treated as full-payload delivery.

**Evidence output:** `certification/results/micear/M12.01.json`

### M12.02 - literal turnaround sequence

BLOCKED for the exact guard interval until D33 is frozen. Correlate virtual traces and `M12-turnaround.csv` against the exact eight-step order: (1) Spectrum request via MIC; (2) stop transmitting; (3) establish receive state; (4) enter EAR polling loop; (5) router waits frozen guard; (6) router response via EAR; (7) Spectrum validates response; (8) Spectrum acknowledges when required. No response edge may precede the frozen receive-ready/guard condition.

**Evidence output:** `certification/results/micear/M12.02.json`

### M12.03 - queued online destination versus offline destination

For a known **online** destination not currently receiving, commit a completed character-message stream and prove the router accepts it, queues it, signals pending traffic, and lets the destination retrieve it later. For the same registered station marked offline/disconnected under the applicable lifecycle policy, prove commit returns the architecture's `Station not available` semantic and is not silently accepted for offline delivery. Preserve the queue when the online destination merely lacks a current receive loop; that condition alone must not become `busy`.

**Evidence output:** `certification/results/micear/M12.03.json`

### M12.04 - busy and liveness semantics

BLOCKED until D48 freezes the physical/virtual evidence used for online/unavailable state. Exercise cable/activity evidence, successful registration, attention acknowledgment, valid traffic, explicit disable, and liveness timeout according to D48 and prove one missed/failed receive window does not by itself make a station unavailable. Prove application idleness is not `busy`. Exercise each Section-12.5 busy class: incompatible active transfer; downlink already committed to an immediate operation; operation requiring a live receive window that is unavailable; and, once D39 freezes the applicable queue/staging bound, refusal caused by that frozen resource policy. Only nonqueueable requested operations return `Station busy`; an ordinary queued character-message commit to an online but non-receiving station must follow M12.03 instead of waiting indefinitely or returning busy.

**Evidence output:** `certification/results/micear/M12.04.json`

**Pass rule:** PASS only when M12.01-M12.04 preserve the exact direction, receive-window, turnaround, queue/offline, and busy/liveness distinctions. No retry policy is inferred from Section 12; retries/timeouts are certified under their own later contracts. Unfrozen D33/D39/D48-dependent behavior remains BLOCKED_GATE.

## M13. Architecture section 13: Physical Modulation

**Architecture authority:** `M` source lines 1171-1248, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Physical modulation.

**Required artifacts:**

- `certification/gates/micear/D05.md`
- `certification/gates/micear/P0-protocol-negotiation.md`
- `tests/fixtures/micear/M13-modulation-vectors.json`
- `certification/hardware/micear/M13-pulse-measurements.csv`

**Artifact/source authority:** Project-generated vectors and hardware captures; no historical tape is an oracle for provisional network timing.

**Tests:**

### M13.01

Before D05 freezes the applicable physical profile, treat the architecture's 2168/667/735/855/1710-T-state compatibility timings, 64-half-pulse direct-link pilot, and >=3 ms inter-frame idle only as proposed test values. Independently of those numeric values, prove the baseline modulation class is self-clocking edge-timed pulse-width coding rather than raw asynchronous UART and that each data bit consists of **two equal-duration half-pulses**: short+short for 0 and long+long for 1. After D05, generate/decode the frozen pilot, sync, zero, one, direct-link pilot, and inter-frame idle values and measure tolerance/margin on multiple Issue-2 machines.

For multi-byte vector fields, prove network byte order is most-significant **byte** first and bit serialization inside each byte is most-significant **bit** first. A later faster profile is BLOCKED until D05 and `P0-protocol-negotiation.md` define the supported profile/negotiation behavior; then prove a station is never sent a profile it does not support and that no implementation-local convenience can silently change pulse widths, byte order, or bit order without explicit version negotiation.

**Evidence output:** `certification/results/micear/M13.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## M14. Architecture section 14: Packet Framing

**Architecture authority:** `M` source lines 1249-1402, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Layer separation, exact packet codec/type set, stop-and-wait transfer control, CRC/end-to-end integrity, and sequence/transaction idempotence.

**Required artifacts:**

- `certification/gates/micear/D06.md`
- `certification/gates/micear/D07.md`
- `certification/gates/micear/D08.md`
- `certification/gates/micear/D09.md`
- `certification/gates/micear/D36.md`
- `certification/gates/micear/D43.md`
- `certification/gates/micear/D45.md`
- `certification/gates/micear/D46.md`
- `tests/fixtures/micear/M14-packet-vectors.json`
- `tests/fixtures/micear/M14-transfer-window-vectors.json`
- `tests/fixtures/micear/M14-end-to-end-integrity.json`

**Artifact/source authority:** Project binary packet/window/integrity vectors generated independently from the frozen wire-format tables. D06/D07 freeze link sync/CRC; D08 payload size; D09 retry counts/timing; D36 routed-header/type layout; D43 whole-message/object integrity algorithms; D45 packet-sequence wrap/duplicate window; D46 logical transaction identifiers/idempotent keys.

**Tests:**

### M14.01 - layering, packet format, and packet-type codec

After D06/D07/D36 freeze, prove the implementation maintains distinct physical pulse stream -> link frame -> routed packet -> stream/object service layers. For the initial protocol, one physical/link frame must contain **exactly one** routed packet and the routed packet's single D07 packet CRC covers the frozen header+payload and is the integrity value the timing controller validates; no second hidden link CRC is allowed. Encode/decode the exact D36 uplink and downlink fields byte-for-byte, proving uplink source is absent/derived from port binding and downlink source is present as frozen. Exercise every frozen control, identity, stream/message, object-transfer, and notification packet type and reject unknown/invalid type/layout values according to the frozen protocol.

**Evidence output:** `certification/results/micear/M14.01.json`

### M14.02 - payload and stop-and-wait transfer control

BLOCKED until D08 and D09 freeze the applicable payload/retry values. Exercise zero/minimum, boundary, maximum, and over-limit payloads. Prove the initial transfer-control semantics are stop-and-wait, at most one data packet outstanding, an acknowledgment required for every data packet, and a bounded retry count/timing exactly as frozen by D09. Inject lost ACK, duplicate data, delayed ACK, NAK, and retry exhaustion; no implementation may silently pipeline multiple data packets or wait/retry without the frozen bound.

**Evidence output:** `certification/results/micear/M14.02.json`

### M14.03 - packet CRC versus end-to-end integrity

Corrupt every routed-header, payload, and CRC bit in `M14-packet-vectors.json` and prove packet CRC validation catches the applicable errors before the frame is reported valid. Separately, after D43, create multi-packet message/object cases whose **individual packet CRCs are all valid** but whose overall payload/order/completion data violates the frozen end-to-end integrity value; the completed message/object must fail end-to-end validation. Packet CRC may not be treated as the whole-transfer integrity oracle.

**Evidence output:** `certification/results/micear/M14.03.json`

### M14.04 - sequence, wrap, duplicate recovery, and logical transaction identity

After D45/D46, exercise packet sequence wrap, duplicate recognition window, retry recognition, ordered chunk delivery, and lost-ack recovery. Prove packet sequence alone is never used as whole stream/message/object identity: the frozen `stream_id`/`transfer_id` or equivalent logical transaction key must distinguish transactions and make repeated close/commit/object-completion requests idempotent within the frozen retry/recovery window.

**Evidence output:** `certification/results/micear/M14.04.json`

**Pass rule:** PASS only when M14.01-M14.04 prove the exact layering, one-frame/one-packet/one-packet-CRC rule, frozen packet types/layout, bounded stop-and-wait behavior, independent end-to-end integrity, and sequence/transaction semantics. Unfrozen D06/D07/D08/D09/D36/D43/D45/D46-dependent values remain BLOCKED_GATE.

## M15. Architecture section 15: Router Routing and Queueing

**Architecture authority:** `M` source lines 1403-1546, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Resolution/routing pipeline, durable message records/FIFO, immutable identity, object staging, and offline behavior.

**Required artifacts:**

- `certification/gates/micear/D11.md`
- `certification/gates/micear/D21.md`
- `certification/gates/micear/D22.md`
- `certification/gates/micear/D24.md`
- `certification/gates/micear/D39.md`
- `certification/gates/micear/D43.md`
- `certification/gates/micear/D47.md`
- `certification/gates/micear/D48.md`
- `tests/fixtures/micear/M15-routing-vectors.json`
- `tests/fixtures/micear/M15-queue-durability.json`
- `tests/fixtures/micear/M15-object-staging.json`
- `tests/fixtures/micear/zx48-router-test.sqlite`

**Artifact/source authority:** Project routing/queue/object/crash vectors and test database. D11/D47 govern reassignment/rebinding, D21/D22 object staging/size, D24 restart recovery, D39 resource bounds, D43 end-to-end integrity, and D48 authoritative online/unavailable lifecycle state.

**Tests:**

### M15.01 - name resolution and direct routing pipeline

Using `M15-routing-vectors.json`, resolve both name and address targets. For a name, prove the exact order: validate six-character syntax -> registry resolution -> Unknown if no mapping -> Unavailable if mapping exists but D48 says station is down. For each direct packet, prove the exact routing pipeline: authenticate source from input port -> validate destination -> validate packet type/length -> locate destination state -> route/queue/reject -> return explicit status. Spoofed source fields, malformed destination/type/length, unknown, unavailable, and queueable/nonqueueable states must enter the correct branch without bypassing prior validation.

**Evidence output:** `certification/results/micear/M15.01.json`

### M15.02 - committed message record and durability

Commit a stream through `CLOSE #` to an online registered destination. Require exactly one queued message record and validate every mandatory logical field: message identifier; monotonic commit sequence; source internal station ID; source address/name snapshots; destination internal station ID; destination address/name snapshots; router-assigned creation time present only as diagnostic metadata; payload length; payload; frozen D43 end-to-end integrity value; delivery state; and retry state. `0 OK` may be returned only after complete metadata+payload are durably committed. Under the applicable D24 restart policy, restart after `0 OK` and prove the record survives. `0 OK` must not be interpreted as recipient-read acknowledgment.

**Evidence output:** `certification/results/micear/M15.02.json`

### M15.03 - FIFO, retry position, delivery marking, retention, and offline commit

Create interleaved messages from multiple sources and exercise both exact reserved router-endpoint forms `OPEN #...;"ROUTER"` and `OPEN #...;"0.0"`. Prove both select the same local router endpoint and offer pending messages FIFO by monotonic **commit sequence** for the immutable destination identity, never creation wall-clock. Separately exercise source-specific retrieval: resolve the current source designator to internal identity and use FIFO only within that source subset. In every case the returned source name is the commit-time snapshot, and partial-delivery retries retain their position ahead of newer eligible messages. Transmission start alone must not delete/mark delivered. Final delivery/deletion requires complete Spectrum validation plus successful close/ack exchange. Successfully committed messages have no automatic wall-clock TTL and remain pending until delivery, admin removal, or an explicitly defined recovery action, subject only to frozen D39 bounds for **new** acceptance. A new commit to a D48-offline/disconnected known station returns Station not available with no grace period; unknown returns Station not found. If the destination disconnects only **after** a successful commit, the existing message remains bound/retained for that same internal identity.

**Evidence output:** `certification/results/micear/M15.03.json`

### M15.04 - immutable identity, renaming/readdressing, replacement, and resource bounds

Prove committed queue ownership is the immutable non-reused router-internal station identity, not mutable display name/address. Basic rename/readdress behavior is BLOCKED until D11 where production reassignment policy is required; any replacement/rebinding that can affect pending data is additionally BLOCKED until D47. After those gates, rename/readdress a station and prove existing committed records do not retarget; replace/rebind and verify pending messages/objects follow the explicit D47 policy. Saturate queue/staging resources only after D39 freezes bounds and prove new acceptance fails explicitly rather than retargeting, truncating, silently dropping, or corrupting existing committed records.

**Evidence output:** `certification/results/micear/M15.04.json`

### M15.05 - object staging visibility

BLOCKED until D21/D22/D43 freeze the applicable staging strategy, maximum object size, and end-to-end object integrity. Stage zero/minimum, typical, maximum, and over-limit objects in `M15-object-staging.json`. The receiver must not be able to observe/accept a staged object until every required chunk is present, object metadata validates, and final object integrity passes. Inject missing chunk, corrupted chunk with valid packet CRC, invalid metadata, failed final integrity, and interrupted staging; none may become a visible completed object.

**Evidence output:** `certification/results/micear/M15.05.json`

**Pass rule:** PASS only when M15.01-M15.05 prove resolution/routing order, complete message-record/durability semantics, stable FIFO/delivery/retention, immutable identity, bounded-resource behavior, and atomic object visibility. Any unfrozen D11/D21/D22/D24/D39/D43/D47/D48-dependent behavior remains BLOCKED_GATE.

## M16. Architecture section 16: Interface 1-style BASIC Front End

**Architecture authority:** `M` source lines 1547-1640, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Exact visible BASIC vocabulary/device/station forms, immediate/stored-program support, and absence of router-internal leakage.

**Required artifacts:**

- `tests/fixtures/micear/zx48-micear-basic-immediate.tap`
- `tests/fixtures/micear/zx48-micear-basic-stored.tap`
- `tests/fixtures/micear/M16-router-leakage.json`
- `tests/fixtures/micear/M04-lexical-conventions.json`
- `certification/gates/micear/D14.md`
- `certification/gates/micear/D15.md`

**Artifact/source authority:** Project-built BASIC TAPs from resident-stack sources, lexical vectors, and router-internal leakage vectors; legally supplied/pinned standard 48K ROM. D14 governs the ROM error-hook/recovery contract and D15 the complete stored-BASIC tokenizer/list/execution strategy.

**Tests:**

### M16.01 - visible vocabulary, device selector, and station/router designators

Using `zx48-micear-basic-immediate.tap` and `M04-lexical-conventions.json`, prove the Architecture-3-visible network vocabulary is exactly `FORMAT`, `SAVE *`, `LOAD *`, `VERIFY *`, `MERGE *`, `OPEN #`, `PRINT #`, `INPUT #`, `INKEY$ #`, `CLOSE #`, and `MOVE`. The network device selector is exactly `"n"`. Every ordinary station-targeted operation that names a station accepts only the frozen ordinary N forms `"x.y"` or `"AAAAAA"` subject to reservation rules. `OPEN #` additionally accepts `"ROUTER"` and `"0.0"` only as the equivalent reserved local-router endpoint. Explicitly verify `NET`, `MSG`, `WHO`, `FILES`, `GET`, `SEND`, and `SHARE` are not introduced as Architecture-3 BASIC commands.

**Evidence output:** `certification/results/micear/M16.01.json`

### M16.02 - immediate and stored BASIC behavior

Immediate command support is mandatory. After the minimum applicable D14 hook/recovery gate, exercise every supported immediate form and verify unrelated BASIC/ROM errors preserve their normal path. Stored-program support for normal Interface-1-style forms remains BLOCKED until D15 and the full applicable D14 strategy are frozen; then load/list/save/reload/execute `zx48-micear-basic-stored.tap` and prove tokenization/listing/execution round-trip without converting the network syntax into a different modern vocabulary.

**Evidence output:** `certification/results/micear/M16.02.json`

### M16.03 - no modern router-internal leakage

Using `M16-router-leakage.json`, force operations with nontrivial queue/database/port/packet state and inspect every Spectrum-visible success/error/data path. Queue IDs, database IDs, router-internal service names, physical port numbers, Pi process names, and transport packet-type names must not leak into the Spectrum UI/programming abstraction. The visible abstraction remains only Spectrum objects, BASIC streams, stations, and normal completion or Interface-1-like reports.

**Evidence output:** `certification/results/micear/M16.03.json`

**Pass rule:** PASS only when M16.01-M16.03 prove the complete visible abstraction and both immediate/stored requirements at their gates. A hidden/private router implementation may exist behind the boundary, but exposing a new command or router-internal identifier through the Spectrum front end is FAIL. D14/D15-dependent stored/hook behavior remains BLOCKED_GATE until frozen.

## M17. Architecture section 17: Command Semantics

**Architecture authority:** `M` source lines 1641-1912, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** One-for-one command syntax, data-flow, blocking/commit behavior, and command-specific outcomes.

**Required artifacts:**

- `tests/fixtures/micear/M17-command-semantics.json`
- `tests/fixtures/micear/M17-stream-visibility.json`
- `tests/fixtures/micear/M18-errors.json`
- `tests/fixtures/micear/zx48-micear-object-transfer.tap`
- `certification/gates/micear/D13.md`
- `certification/gates/micear/D40.md`

**Artifact/source authority:** Project command/error/stream vectors and object-transfer TAP. D40 freezes channel direction and mixed PRINT/INPUT/INKEY semantics; D13 freezes the exact Interface-1 compatibility baseline, accepted MOVE grammar, and EOF behavior.

**Tests:**

### M17.01 - literal command-semantics ledger

| Command | Exact test obligations |
|---|---|
| FORMAT | Parse `FORMAT "n";N` including `MASTER` and `1.1`; establish/confirm identity, validate local name/address, activate binding, and store returned local identity. Exercise `0 OK`, Invalid station, Missing station, Station name in use, valid-preconfigured-but-missing Station not found, Network not present, Network timeout, and Network version error. |
| SAVE * | Parse all six forms: plain BASIC, `LINE`, numeric `DATA`, character `DATA`, `CODE address,length`, `SCREEN$`; transfer the matching Spectrum object; return `0 OK` only after complete validated router acceptance; exercise Station not set/found/available/busy, Network not present/timeout/data error, and Transfer rejected. |
| LOAD * | Parse plain, numeric/character DATA, CODE/no-address, CODE/address, CODE/address,length, and SCREEN$ forms; explicitly enter receive mode before full downlink; receive/load object from named source; exercise the listed SAVE-like error set. |
| VERIFY * | Parse plain, numeric/character DATA, CODE address,length, SCREEN$; receive and compare without replacing mismatched memory semantics; inject mismatch and require `Verification has failed` in addition to applicable LOAD errors. |
| MERGE * | Parse `MERGE *"n";N`; receive BASIC and merge using normal Spectrum MERGE concept; applicable outcomes mirror LOAD plus genuine BASIC merge ROM errors. |
| OPEN # | Parse station and ROUTER_ENDPOINT forms, including GAME02/1.3/ROUTER/0.0 examples; create channel only after resolution/availability checks. Half-duplex direction/mixed-use behavior is BLOCKED until D40, then follows D40 exactly. Exercise all listed OPEN outcomes. |
| PRINT # | Parse `PRINT #stream;expression`; add formatted character data to output stream. Use `M17-stream-visibility.json` to force local-buffer and transport-flush boundaries and prove provisional router-side bytes are never visible as a committed destination message before CLOSE. |
| INPUT # | Parse variable and `LINE string-variable` forms; receive formatted input from the opened station/router endpoint; prove the operation is blocking while awaiting the explicitly requested payload. |
| INKEY$ # | Parse `LET A$=INKEY$ #stream`; when no character is available return `""` immediately, when available return exactly one character; surface established-peer Station not available and previously detected Network data error; never create a new network timeout merely because no character is ready. |
| CLOSE # | Output channel: flush final buffer, commit, wait for router acceptance, then `0 OK`. Queued-message input channel: acknowledge only after full validated receive; interrupted/unconsumed/failed delivery stays preserved/requeued; release local channel state only after close/ack or defined failure path. Generic non-message stream close remains BLOCKED until the Phase-6/D40 compatibility rule is frozen. |
| MOVE | Syntax class is `MOVE source TO destination`; network channels may be on either side. Exact accepted source/destination grammar and EOF behavior remain BLOCKED until D13, then are copied from the chosen Interface-1 compatibility baseline and mapped to the router without inventing new MOVE semantics. |

Execute every row using `M17-command-semantics.json`; use `zx48-micear-object-transfer.tap` for SAVE/LOAD/VERIFY/MERGE object cases and `M18-errors.json` for the command-specific report states. No row may be marked PASS merely because a neighboring command shares an implementation path.

**Evidence output:** `certification/results/micear/M17.01.json`

### M17.02 - cross-command stream direction and commit invariants

After D40, open channels and exercise first-operation direction selection/mixed-operation sequences exactly as frozen. Prove half-duplex behavior is enforced by channel semantics rather than encoded into the legacy OPEN syntax; PRINT-side provisional storage remains uncommitted until CLOSE; blocking INPUT and nonblocking INKEY$ retain their distinct wait/error behavior; and CLOSE is the semantic commit/ack boundary. No implementation may expose a partial message simply because a transport buffer flushed.

**Evidence output:** `certification/results/micear/M17.02.json`

**Pass rule:** PASS only when every one of the eleven command rows passes its exact syntax and semantics and D40/D13-dependent cases remain BLOCKED_GATE until frozen. A broad end-to-end transfer success cannot substitute for a missing command form, error state, blocking rule, or commit boundary.

## M18. Architecture section 18: Success Reports and Error Reports

**Architecture authority:** `M` source lines 1913-2033, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Exact Spectrum-style success/error presentation, one-for-one semantic report distinctions, and normative precedence.

**Required artifacts:**

- `tests/fixtures/micear/M18-errors.json`
- `tests/fixtures/micear/M18-precedence.json`
- `tests/fixtures/micear/zx48-micear-basic-immediate.tap`
- `certification/gates/micear/D25.md`

**Artifact/source authority:** Project-generated error/precedence state matrices executed through the resident BASIC front end. Section 18 freezes semantic text/distinctions and line:statement presentation; D25 alone freezes the final report letters/numeric identifiers.

**Tests:**

### M18.01 - success and report presentation

Execute successful immediate operations through `zx48-micear-basic-immediate.tap` and require normal Spectrum completion presentation exactly in the form `0 OK, line:statement`, including the architecture example `0 OK, 0:1` when that location applies. No modern prose success banner may be added. For every network failure, require Interface-1-like presentation containing a report identifier, exact semantic report text, line number, and statement number. Semantic text/line/statement can be certified before D25; the concrete report letter/numeric identifier remains BLOCKED until D25.

**Evidence output:** `certification/results/micear/M18.01.json`

### M18.02 - literal semantic error catalog

Using `M18-errors.json`, independently force and verify each semantic distinction exactly as defined:

| Report text | Condition that must produce it |
|---|---|
| `Invalid station` | station operand malformed or outside permitted ranges |
| `Missing station` | required station operand absent |
| `Station name in use` | requested six-character local name assigned elsewhere |
| `Station not set` | local Spectrum has not completed valid registration |
| `Station not found` | no registered name/address matches N |
| `Station not available` | identity exists but station is offline/disconnected/disabled/down |
| `Station busy` | station exists and is online but cannot accept the requested operation |
| `Network timeout` | expected exchange does not complete within its permitted interval |
| `Network not present` | resident stack cannot communicate with local router |
| `Network data error` | framing/sequence/checksum/repeated-link corruption prevents completion |
| `Network version error` | stack/router have no compatible protocol version |
| `Transfer rejected` | destination/router explicitly refuses transfer |
| `Verification has failed` | VERIFY received data successfully but it differs from memory |

Explicitly pair `Station not found` against `Station not available` in otherwise equivalent vectors and prove they can never collapse into one generic failure.

**Evidence output:** `certification/results/micear/M18.02.json`

### M18.03 - normative error precedence and FORMAT exemption

Using `M18-precedence.json`, construct multi-fault cases and prove ordinary already-registered operations surface the earliest authoritative failure in exactly this order: (1) syntax/missing operand; (2) local station not set; (3) local router not present; (4) protocol version mismatch; (5) station not found; (6) station not available; (7) station busy; (8) transfer rejected; (9) timeout; (10) data error; (11) verification failure. For every adjacent pair and representative multi-fault combination, later failures must remain hidden until earlier ones are removed. Separately prove `FORMAT "n";N` is exempt from `Station not set` and uses its Section-17.1 command-specific outcomes, allowing transition from NOT_SET to registered. No individual handler may choose a different precedence without an architecture/protocol revision.

**Evidence output:** `certification/results/micear/M18.03.json`

**Pass rule:** PASS only when M18.01-M18.03 prove exact Spectrum-style presentation, every semantic report distinction, and the complete precedence rule. Any substitution of `Station not found` with a generic/renamed condition, precedence drift, or modern success banner is FAIL. Only final report identifiers remain BLOCKED_GATE on D25.

## M19. Architecture section 19: Asynchronous Incoming Messages

**Architecture authority:** `M` source lines 2034-2201, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Router-side asynchronous queueing, non-payload attention, interrupt detector limits, pending-flag reconciliation, exact retrieval forms, delivery acknowledgment, and safe presentation.

**Required artifacts:**

- `certification/gates/micear/D17.md`
- `certification/gates/micear/D18.md`
- `certification/gates/micear/D40.md`
- `certification/gates/micear/D43.md`
- `tests/fixtures/micear/zx48-micear-stack.tap`
- `tests/fixtures/micear/M19-attention.json`
- `tests/fixtures/micear/M19-pending-reconciliation.json`
- `tests/fixtures/micear/M19-retrieval-vectors.json`
- `tests/fixtures/micear/M28-async-message-vectors.json`
- `certification/hardware/micear/M19-attention-capture.csv`

**Artifact/source authority:** Project resident-stack TAP and async/pending/retrieval fixtures plus real Issue-2 attention captures. D17 freezes the exact attention waveform/false-positive threshold; D18 freezes notification presentation; D40 freezes Phase-6 channel-byte/direction compatibility; D43 freezes end-to-end message integrity.

**Tests:**

### M19.01 - asynchronous queue commit and attention payload separation

While the destination Spectrum is not executing INPUT or another receive operation, send the architecture example stream (`OPEN #4;"n";"GAME02"`, `PRINT #4;"READY TO START"`, `CLOSE #4`). Prove the router accepts the completed stream, creates one queued message, and returns sender success only after queue commit. On destination pending-count transition 0->nonzero, begin periodic EAR attention signalling. The attention waveform must contain **no message payload** and must never become an unsolicited complete-packet receive.

**Evidence output:** `certification/results/micear/M19.01.json`

### M19.02 - interrupt-time detector contract

Load `zx48-micear-stack.tap`. After D17 freezes the final attention pattern, correlate `M19-attention-capture.csv` with the 50-Hz resident interrupt path. The detector is available only while its hook is installed, reserved RAM remains intact, and interrupts are serviced compatibly. It may sample EAR, advance only the tiny attention state machine, set the pending flag after a complete valid pattern, preserve required machine state, and return quickly. It must never receive full payload, call arbitrary BASIC, or write message text into the editor. If the hook is absent/corrupted/interrupts masked incompatibly, notification may be missed but the router queue remains authoritative.

**Evidence output:** `certification/results/micear/M19.02.json`

### M19.03 - router attention lifecycle and suppression

Using `M19-attention.json`, prove periodic attention starts when pending count changes from zero to nonzero; stops during active packet RX, active packet TX, and router-generated tape/bootstrap; resumes after return to NETWORK_IDLE if pending messages remain; and stops after Spectrum pending acknowledgment or queue-empty transition. Before D17, the proposed 4-frame active / 2-frame idle / 4-frame active (~200 ms at 50 Hz) sequence is test-only and cannot be certified as final. After D17, measure the frozen pattern for false-positive resistance and noninterference with normal packet waveforms.

**Evidence output:** `certification/results/micear/M19.03.json`

### M19.04 - NET_PENDING advisory state and reconciliation

Using `M19-pending-reconciliation.json`, prove at minimum `NET_PENDING=0` means no attention pattern has been accepted and `NET_PENDING=1` means the router indicates one or more queued messages. The flag is advisory, never queue authority. Every foreground network entry point allowed by the architecture may reconcile it with `PENDING_STATUS`; after a period in which the resident interrupt hook was unavailable, software must poll/reconcile the router before concluding there is no pending traffic. Exercise missed-attention, stale-1-after-queue-empty, and message-arrival-during-hook-outage cases.

**Evidence output:** `certification/results/micear/M19.04.json`

### M19.05 - exact queued-message retrieval semantics

Using `M19-retrieval-vectors.json`, prove no new BASIC command is introduced. Router-wide retrieval accepts `OPEN #4;"n";"ROUTER"` and equivalent `"0.0"`, then exposes exactly two logical input fields in order: canonical six-character **source name**, then complete character message payload. Source-specific retrieval such as `OPEN #4;"n";"MASTER"` exposes only the message payload. Execute the documented INPUT/CLOSE sequence. The exact channel-byte encoding is BLOCKED until D40 freezes Phase-6 compatibility, but whatever encoding is chosen must preserve these BASIC-observable field semantics exactly.

**Evidence output:** `certification/results/micear/M19.05.json`

### M19.06 - delivery acknowledgment and integrity

After D43, inject complete, corrupted, truncated, interrupted, and close/ACK-failure retrievals. A queued message may be marked delivered/deleted only when the complete record has been received, frozen end-to-end integrity validation passes, and the Spectrum completes CLOSE or the explicit acknowledgment path. Any failure before all three conditions preserves/requeues the message under the queue contract.

**Evidence output:** `certification/results/micear/M19.06.json`

### M19.07 - safe notification presentation

BLOCKED until D18 freezes the presentation choice. Then exercise the selected short sound, border indication, combination, or other non-destructive indication and prove it never alters the current edit line, injects text, modifies BASIC program data, or corrupts compatible application state. Presentation is advisory only; it must not replace `NET_PENDING`/router queue semantics.

**Evidence output:** `certification/results/micear/M19.07.json`

**Pass rule:** PASS only when M19.01-M19.07 prove asynchronous queue authority, attention-only notification, interrupt safety, lifecycle suppression/resume, advisory pending reconciliation, exact BASIC retrieval fields, delivery acknowledgment, and non-destructive presentation. D17/D18/D40/D43-dependent details remain BLOCKED_GATE until frozen.

## M20. Architecture section 20: Resident High-Memory Spectrum Stack

**Architecture authority:** `M` source lines 2202-2343, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Tape-delivered residency/memory protection, complete resident-component inventory, BASIC error-hook flow/safety, stored-program compatibility, and cooperative blocking operation.

**Required artifacts:**

- `certification/gates/micear/D14.md`
- `certification/gates/micear/D15.md`
- `certification/gates/micear/D16.md`
- `tests/fixtures/micear/zx48-micear-stack.tap`
- `tests/fixtures/micear/zx48-micear-protected-memory.tap`
- `tests/fixtures/micear/zx48-micear-basic-immediate.tap`
- `tests/fixtures/micear/zx48-micear-basic-stored.tap`
- `tests/fixtures/micear/M20-component-map.json`
- `tests/fixtures/micear/M20-hook-safety.json`

**Artifact/source authority:** Project-built resident-stack/protection/BASIC TAPs plus linked symbol/component map and hook-state vectors; standard 48K ROM legally supplied and hash-pinned. D14 freezes the immediate/full hook recovery behavior, D15 stored BASIC strategy, and D16 the phase-applicable high-memory range/footprint.

**Tests:**

### M20.01 - delivery, residency, and BASIC memory-boundary protection

Load `zx48-micear-stack.tap` through ordinary tape/bootstrap into an unmodified/certified 48K Issue-2 environment. After the applicable D16 range is frozen, prove the installer places the stack in reserved high RAM and lowers/protects the normal BASIC usable-memory boundary so ordinary BASIC allocation cannot overwrite it. Demonstrate the resident extension is invoked only through hooks/explicit machine-code calls and is not modeled as a DOS-style TSR. Deliberately overwrite the reserved region, replace the required interrupt hook without chaining, and disable interrupts indefinitely in isolated destructive tests; networking notification/API availability may cease as documented, but the implementation must not falsely claim transparent coexistence or auto-repair through hidden emulator mechanisms.

**Evidence output:** `certification/results/micear/M20.01.json`

### M20.02 - complete resident component inventory

Using `M20-component-map.json` produced from the linked resident-stack image, require an implemented/testable mapping for every Section-20.3 component: installer/uninstaller; BASIC error-path hook; Interface-1 syntax parser; channel manager; object-transfer manager; packet encoder/decoder; MIC transmitter; EAR receiver; attention detector; message-pending flag; router registration/resolution client; checksums/retries; diagnostics; and buffers/state. Every component must map to resident symbols/modules and at least one lower-level test; an omitted component cannot be hidden by a generic “network stack” label.

**Evidence output:** `certification/results/micear/M20.02.json`

### M20.03 - BASIC error-path hook and Interface-1 exclusion

After the minimum applicable D14 hook contract, run `zx48-micear-basic-immediate.tap` with Interface 1 absent. Prove unsupported Interface-1-style syntax reaches the normal 48K ROM error machinery, which invokes the RAM-resident hook; recognized network forms parse/execute/return correctly; unrecognized forms preserve the original ROM error path. In WZSN require `EAR_MIC`; in physical tests keep Interface 1 absent. No Interface-1 shadow-ROM parser chaining, dual `"n"` ownership, networking-ROM paging, PC patch, or ROM-loader shortcut may satisfy this test.

**Evidence output:** `certification/results/micear/M20.03.json`

### M20.04 - atomic hook installation, duplicate detection, and clean restoration

Using `M20-hook-safety.json`, record every pointer/stack destination and BASIC memory-boundary value before install. Fault-inject each installation step and prove the hook is installed atomically or prior state remains intact. Verify reserved RAM range before activation; a second install is detected rather than stacking duplicate hooks. On safe uninstall/reset, restore every replaced hook/pointer and saved memory-boundary state exactly. Unrelated BASIC errors must pass to the original handler and retain correct line/statement reporting before, during, and after install/remove.

**Evidence output:** `certification/results/micear/M20.04.json`

### M20.05 - stored BASIC tokenization/listing/execution gate

Immediate interception alone cannot certify stored programs. Stored-program testing is BLOCKED until D15 and the full applicable D14 mechanism are frozen. Then execute `zx48-micear-basic-stored.tap` and prove normal Interface-1-style network forms in numbered lines preserve tokenization, syntax checking, listing, execution, error position, and ordinary 48K BASIC compatibility. The implementation may not claim that the immediate-mode error hook automatically solves these cases without the D15 evidence.

**Evidence output:** `certification/results/micear/M20.05.json`

### M20.06 - protected receive destinations and cooperative blocking operation

Using `zx48-micear-protected-memory.tap`, attempt CODE, BASIC, array, MERGE, and workspace receives that touch the reserved stack/workspace, saved hook state, or lowered active BASIC boundary. Reject unsafe destinations **before destructive writes** and map rejection through the frozen transfer/error policy. Explicit CODE ranges must be range-checked; BASIC/array/MERGE must honor normal allocation/error behavior under the lowered boundary. Prove full transmit/receive operations are blocking and non-reentrant; compatible BASIC/machine-code callers must use safe call points and preserve resident/API state; the attention detector is the only background-like operation and it sets only the pending flag.

**Evidence output:** `certification/results/micear/M20.06.json`

**Pass rule:** PASS only when M20.01-M20.06 prove tape delivery, protected residency, every resident component, authentic error-hook flow, atomic install/remove safety, stored-program behavior at its gates, and cooperative blocking semantics. D14/D15/D16-dependent values/behavior remain BLOCKED_GATE until frozen.

## M21. Architecture section 21: Tape Distribution and Installation

**Architecture authority:** `M` source lines 2344-2438, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Release-tape structure, address-aware bootstrap, conditional digital packaging, exact router-generated load flow, cold-start boundary, and versioned image header.

**Required artifacts:**

- `certification/gates/micear/D16.md`
- `certification/gates/micear/D37.md`
- `tests/fixtures/micear/zx48-micear-stack.tap`
- `tests/fixtures/micear/zx48-micear-stack.tzx`
- `tests/fixtures/micear/zx48-micear-stack.wav`
- `tests/fixtures/micear/zx48-micear-diagnostic.tap`
- `tests/fixtures/micear/M21-tape-block-map.json`
- `tests/fixtures/micear/M21-bootstrap-flow.json`
- `tests/fixtures/micear/M21-image-header.json`

**Artifact/source authority:** Project build artifacts generated from the same resident-stack binary; no downloaded media. D16 freezes the phase-applicable load range/footprint and therefore final CLEAR/load/entry addresses. D37 freezes the binary image-header/tape packaging format. TZX is a conditional artifact: it is certification-required only when the frozen release requires timing features that ordinary TAP cannot represent.

**Tests:**

### M21.01 - release tape structure and address-aware bootstrap

Using `M21-tape-block-map.json`, verify the release tape can encode the baseline block roles in order as applicable: BASIC bootstrap; high-memory core code; optional BASIC-front-end code; optional diagnostics/help/demo. The architecture's sample `CLEAR 49151 / LOAD "" CODE 49152 / RANDOMIZE USR 49152` is **example-only** and may not become a certified constant. The final CLEAR value, load address, and entry address are BLOCKED until the applicable D16 binary/map is frozen; afterward parse the actual bootstrap and prove its values agree exactly with that binary/map.

**Evidence output:** `certification/results/micear/M21.01.json`

### M21.02 - digital distribution equivalence without over-requiring TZX

Produce `zx48-micear-stack.tap` and `zx48-micear-stack.wav` from the same frozen resident binary and prove both load identical bytes and produce the same installed stack. Produce/test `zx48-micear-stack.tzx` **only when** the frozen release uses timing features that exceed ordinary TAP representation; otherwise mark that conditional TZX case N/A rather than FAIL. When TZX is required, prove its timing/data reconstruct the same binary. No format may contain emulator-only RAM injection semantics.

**Evidence output:** `certification/results/micear/M21.02.json`

### M21.03 - literal router-generated bootstrap flow and port-mode exclusivity

Using `M21-bootstrap-flow.json`, execute the exact startup order: (1) port enters `BOOTSTRAP_TAPE` and attention/network downlink are suppressed; (2) user executes `LOAD ""`; (3) router emits normal Spectrum tape waveform through EAR; (4) bootstrap+stack load into RAM; (5) stack installs hooks; (6) bootstrap completes and port returns to `NETWORK_IDLE`; (7) stack contacts router through normal MIC/EAR network framing; (8) router returns station identity and status. At no point may attention or routed-packet waveform interleave with bootstrap. In WZSN `EAR_MIC`, ordinary Tape transport still may not concurrently own cassette EAR/MIC; only the explicit router `BOOTSTRAP_TAPE` path supplies this in-mode waveform.

**Evidence output:** `certification/results/micear/M21.03.json`

### M21.04 - future network-update bootstrap and cold-start boundary

Prove a later small stable bootstrap that fetches a newer full stack is **not** an initial-release acceptance requirement and cannot be used to waive cold-start media. Every initial cold start must begin from at least one architecture-permitted source: real tape, digital tape playback, or router-generated tape waveform. A build that requires a pre-existing installed network stack to obtain the network stack fails the cold-start contract.

**Evidence output:** `certification/results/micear/M21.04.json`

### M21.05 - image header/packaging contract

BLOCKED until D37. Then generate `M21-image-header.json` independently from the linker/package metadata and parse the actual distributed image. Require the frozen header equivalents of: signature; image-format version; network-protocol version; software version; load address; entry address; required RAM; feature flags; binary length; and binary checksum. Corrupt each field/length/checksum and require controlled rejection before executing/installing an invalid image.

**Evidence output:** `certification/results/micear/M21.05.json`

**Pass rule:** PASS only when M21.01-M21.05 preserve the example-versus-frozen-address distinction, conditional TZX rule, exact pulse-level bootstrap sequence/exclusivity, cold-start requirement, and frozen image header. D16/D37-dependent values remain BLOCKED_GATE until frozen.

## M22. Architecture section 22: Internal Spectrum Stack API

**Architecture authority:** `M` source lines 2439-2498, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Internal Spectrum stack API and calling convention.

**Required artifacts:**

- `certification/gates/micear/D29.md`
- `certification/gates/micear/D34.md`
- `tests/fixtures/micear/M22-stack-api.json`
- `tests/fixtures/micear/zx48-micear-api-harness.tap`
- `build/spectrum/zx48-micear-stack.map`

**Artifact/source authority:** Project stack symbol/map file plus machine-code API harness.

**Tests:**

### M22.01

After D29/D34, compare `build/spectrum/zx48-micear-stack.map` against the frozen D34 export list and require every baseline entry point exactly once: `NET_INIT`, `NET_REMOVE`, `NET_REGISTER`, `NET_GET_IDENTITY`, `NET_RESOLVE`, `NET_OPEN_STREAM`, `NET_STREAM_WRITE`, `NET_STREAM_READ`, `NET_STREAM_POLL`, `NET_CLOSE_STREAM`, `NET_SEND_OBJECT`, `NET_RECEIVE_OBJECT`, `NET_VERIFY_OBJECT`, `NET_POLL_ROUTER`, `NET_GET_PENDING`, `NET_GET_STATUS`, and `NET_ABORT`. Using `zx48-micear-api-harness.tap`, call every entry from machine code; route equivalent BASIC-front-end operations through the same networking core and prove existing exported semantics do not drift between caller types. Verify the D29 Z80 register/error convention, buffer ownership, non-reentrancy rules, and error returns for every entry. The attention handler may touch only dedicated interrupt-safe state; while full link functions own EAR they must disable or coordinate attention processing so the non-reentrant stack cannot execute overlapping full-link state machines.

**Evidence output:** `certification/results/micear/M22.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## M23. Architecture section 23: Router Data Model

**Architecture authority:** `M` source lines 2499-2599, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** One-for-one verification of the baseline station, message, object-transfer, and port-status data model.

**Required artifacts:**

- `certification/gates/micear/D38.md`
- `tests/fixtures/micear/M23-data-model.json`
- `tests/fixtures/micear/M23-schema-introspection.json`
- `tests/fixtures/micear/zx48-router-test.sqlite`

**Artifact/source authority:** Project schema migration/test database and independently generated schema-introspection record. D38 freezes the concrete persistent/in-memory mapping at the applicable phase; the architecture field names below are logical requirements and need not force identical SQL column spelling if D38 documents a lossless mapping.

**Tests:**

### M23.01 - stations model and identity constraints

After D38 freezes the registry mapping, introspect `zx48-router-test.sqlite`/router state and prove lossless storage for: `station_id_internal`, name, area, node, physical_port, enabled, online, protocol_version, stack_version, last_seen, created_at, updated_at. Enforce uniqueness of name, uniqueness of `(area,node)`, and uniqueness of the active physical-port binding. Delete/replace a station and reuse its visible name/address; the replacement must receive a different immutable internal station ID and no internal ID may ever be reused for a different station.

**Evidence output:** `certification/results/micear/M23.01.json`

### M23.02 - queued-message model

Create committed/retried/delivered message cases and prove the data model losslessly represents: message_id; commit_sequence; source internal station ID; source area/node/name snapshots; destination internal station ID; destination area/node/name snapshots; payload_length; payload location or blob; payload_checksum; created_at; delivery_state; delivery_attempts; last_attempt_at. Verify source/destination snapshots remain the commit-time values even when visible station metadata changes later.

**Evidence output:** `certification/results/micear/M23.02.json`

### M23.03 - object-transfer staging model

Create active/completed/aborted object-transfer cases and prove lossless representation of: transfer_id; source internal station ID; source address/name snapshot; destination internal station ID; destination address/name snapshot; object type; object metadata; total length; received length; object checksum; staging location; state; created_at; updated_at. The test proves field presence/meaning under D38; it does not invent transaction-ID width or staging semantics that are frozen by other gates.

**Evidence output:** `certification/results/micear/M23.03.json`

### M23.04 - per-port status model

For two independently active ports, prove the table/in-memory state losslessly represents per port: port number; configured station; cable/activity state; uplink state; downlink state; last edge time; last valid frame time; framing errors; CRC errors; retries; queue depth; controller health. Update each field on one port and prove the other port's status is not aliased or overwritten.

**Evidence output:** `certification/results/micear/M23.04.json`

**Pass rule:** PASS only when all four logical models and every listed field/constraint have an explicit D38 mapping and executable evidence. A database implementation may choose different physical names/layouts, but missing or lossy logical state is FAIL. Behavior belonging to sequence-window, queue-retention, or transfer-state gates is not silently inferred from this schema-only section.

## M24. Architecture section 24: State Machines

**Architecture authority:** `M` source lines 2600-2765, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Exact transition/illegal-transition proof for all seven Section-24 state machines and cassette/attention ownership.

**Required artifacts:**

- `certification/gates/micear/D09.md`
- `certification/gates/micear/D11.md`
- `certification/gates/micear/D17.md`
- `certification/gates/micear/D21.md`
- `certification/gates/micear/D43.md`
- `certification/gates/micear/D46.md`
- `certification/gates/micear/D48.md`
- `tests/fixtures/micear/M24-state-machines.json`
- `tests/fixtures/micear/M24-port-mode.json`

**Artifact/source authority:** Project state-transition vectors/model-checker specification. Decision gates are required only for the transitions whose concrete conditions/values they freeze; the topology of the diagrams themselves is tested exactly as written.

**Tests:**

### M24.01 - station lifecycle

Model-check `UNCONFIGURED -> CONFIGURED -> ONLINE -> UNAVAILABLE -> ONLINE`. Administrative assignment of port/name/address creates CONFIGURED; physical activity plus valid registration creates ONLINE; disconnect/timeout/disable creates UNAVAILABLE; reconnect+register restores ONLINE. Concrete liveness/timeout evidence is BLOCKED until D48. Any production reassignment semantics beyond initial assignment remain subject to D11. Reject all transitions not represented by the frozen lifecycle/policy.

**Evidence output:** `certification/results/micear/M24.01.json`

### M24.02 - Spectrum local registration

Model-check `NOT_SET -> REGISTERING` only through `FORMAT "n";N`. REGISTERING may resolve to SUCCESS->SET or NAME_CONFLICT, NETWORK_NOT_PRESENT, VERSION_ERROR, or TIMEOUT exactly as shown. Production identity allocation/reassignment behavior is BLOCKED on D11, but FORMAT must remain the only normal transition from NOT_SET into registration.

**Evidence output:** `certification/results/micear/M24.02.json`

### M24.03 - output-stream commit and COMMIT_UNKNOWN

Model-check `CLOSED --OPEN#--> OPEN_OUTPUT --PRINT#--> BUFFERING --CLOSE#--> COMMITTING`. ROUTER_ACCEPTED returns CLOSED/0 OK; REJECTED returns CLOSED/error. A timeout enters `COMMIT_UNKNOWN`, not an invented success/failure. After D09/D46, retry **the same logical commit transaction** within the bounded idempotent retry policy: ACK -> CLOSED/0 OK; retries exhausted -> CLOSED/Network timeout. Prove a retry cannot create a duplicate queued message.

**Evidence output:** `certification/results/micear/M24.03.json`

### M24.04 - queued-message delivery state machine

Drive `RECEIVING_FROM_SENDER -> QUEUED` only after complete data plus frozen D43 end-to-end integrity succeeds; notification moves to PENDING_DELIVERY; destination input open moves to DELIVERING; validated ACK moves to DELIVERED; interruption returns the same record to QUEUED. Inject interruption at each delivery offset and prove no duplicate/new record identity is created.

**Evidence output:** `certification/results/micear/M24.04.json`

### M24.05 - object-upload staging state machine

After D21/D43, model-check `OFFER` -> REJECTED or ACCEPTED -> RECEIVING_CHUNKS -> ABORTED/RETRY/or VERIFYING_OBJECT -> CHECKSUM_ERROR/or STAGED_COMPLETE. Missing/corrupt chunks and failed whole-object integrity cannot reach STAGED_COMPLETE; retry resumes the same frozen transfer transaction rather than silently starting another completed object.

**Evidence output:** `certification/results/micear/M24.05.json`

### M24.06 - attention detector state machine

After D17 freezes the actual durations, model-check `IDLE -> ACTIVE_1 -> IDLE_GAP -> ACTIVE_2 -> PENDING_FLAG_SET` only on the expected active/idle/active sequence. Inject every wrong duration/order/extra edge and require reset to IDLE exactly as specified. No invalid partial pattern may set the pending flag.

**Evidence output:** `certification/results/micear/M24.06.json`

### M24.07 - router-port mode and cassette ownership

Using `M24-port-mode.json`, model-check only `NETWORK_IDLE -> BOOTSTRAP_TAPE -> NETWORK_IDLE` for router-generated cold/bootstrap waveform and `NETWORK_IDLE -> NETWORK_TRANSFER -> NETWORK_IDLE` for routed transfer, including complete/abort/timeout exits. Attention is permitted only in NETWORK_IDLE. Controller/transport loss forces physical output safe idle and logical unavailable/fault until resynchronized. In WZSN `EAR_MIC`, prove this state machine is the sole cassette-socket ownership arbiter: ordinary Tape cannot simultaneously drive EAR or consume MIC; `BOOTSTRAP_TAPE` is the only in-mode tape-waveform ownership path while Ear+Mic remains attached.

**Evidence output:** `certification/results/micear/M24.07.json`

**Pass rule:** PASS only when all seven diagrams are model-checked with every legal transition and representative illegal transition, and gated conditions use the named frozen decisions rather than guessed constants. Any extra transition that can alter delivery, identity, attention, or cassette ownership is FAIL.

## M25. Architecture section 25: Reliability and Recovery

**Architecture authority:** `M` source lines 2766-2861, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Required reliability mechanisms, stop-and-wait, duplicate/idempotent retry semantics, power/link-loss recovery, and operation-specific timeouts.

**Required artifacts:**

- `certification/gates/micear/D07.md`
- `certification/gates/micear/D09.md`
- `certification/gates/micear/D24.md`
- `certification/gates/micear/D43.md`
- `certification/gates/micear/D45.md`
- `certification/gates/micear/D46.md`
- `tests/fixtures/micear/M25-fault-injection.json`
- `tests/fixtures/micear/M25-recovery-matrix.json`
- `tests/fixtures/micear/M25-timeout-vectors.json`

**Artifact/source authority:** Project deterministic loss/corruption/duplicate/power-loss/timeout harness. D07 freezes packet CRC, D09 retry/timeout values, D24 router restart/sudden-power-loss behavior at the applicable phases, D43 message/object end-to-end integrity, D45 sequence wrap/duplicate window, and D46 logical transaction identifiers/idempotent keys.

**Tests:**

### M25.01 - required mechanisms and stop-and-wait baseline

After the applicable gates, exercise every Section-25.1 mechanism explicitly: packet CRC; whole-object checksum/integrity; packet sequence numbers; duplicate suppression; bounded retries; deterministic timeouts; explicit ACK/NAK; resumable router-side queued-message delivery; safe abort; and queue persistence where promised. For first-release file transfer, prove **one outstanding chunk at a time**: a sender may not advance to a second data chunk until the current chunk reaches its frozen ACK/NAK/retry outcome. Measure the resulting bounded RAM/state and deterministic traceability rather than silently pipelining.

**Evidence output:** `certification/results/micear/M25.01.json`

### M25.02 - duplicate packet/commit and unrecoverable acknowledgment loss

After D45/D46, drop an acknowledgment and resend the same packet and the same logical commit transaction. Within the frozen duplicate/retry window, the receiver/router must recognize the duplicate sequence/transaction ID, must not apply payload or create a committed queue record twice, and must resend the previous ACK/result while it remains authoritative. A successful commit is idempotent under retries. If all D09 retries expire after the response was lost, the sender returns `Network timeout` and remote completion may remain uncertain: the protocol must **not** claim exactly-once application semantics across that unrecoverable failure.

**Evidence output:** `certification/results/micear/M25.02.json`

### M25.03 - power/link-loss recovery matrix

Using `M25-recovery-matrix.json`, interrupt at every meaningful offset in four operations. During **message upload**, incomplete data is discarded or retained explicitly incomplete and never delivered. During **message delivery**, the record returns to QUEUED unless acknowledgment completed; after an unrecoverable ACK loss, delivery is at-least-once and the recipient/application cannot assume exactly-once. During **object upload**, staged data stays incomplete and invisible. During **object download**, the Spectrum reports failure; resume support is optional and must not be required for initial certification. Apply D24 where daemon restart/sudden-power-loss policy is phase-gated.

**Evidence output:** `certification/results/micear/M25.03.json`

### M25.04 - operation-specific deterministic timeout policy

BLOCKED until D09 freezes the applicable values. Independently force timeout of: registration; name resolution; stream open; stream close/commit; object offer; object chunk; object completion; message retrieval; and attention acknowledgment. Verify each uses its own frozen deterministic operation policy, produces the appropriate error/recovery state, and does not inherit an unrelated wall-clock/network-library timeout. Runtime host scheduling may delay wall-clock completion but may not change the protocol's deterministic timeout outcome in virtual fidelity tests.

**Evidence output:** `certification/results/micear/M25.04.json`

**Pass rule:** PASS only when M25.01-M25.04 prove every required reliability mechanism, strict first-release stop-and-wait, duplicate/idempotent behavior with the explicit at-least-once caveat, all power/link-loss states, and every timeout class. Unfrozen D07/D09/D24/D43/D45/D46-dependent behavior remains BLOCKED_GATE.

## M26. Architecture section 26: Security and Trust Boundaries

**Architecture authority:** `M` source lines 2862-2919, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Physical-port source identity, complete untrusted-input validation, bounded per-station denial-of-service controls, and the explicit absence of Spectrum-link confidentiality.

**Required artifacts:**

- `certification/gates/micear/D11.md`
- `certification/gates/micear/D28.md`
- `certification/gates/micear/D39.md`
- `tests/fixtures/micear/M26-security.json`
- `tests/fixtures/micear/M26-cable-swap.json`

**Artifact/source authority:** Project security/spoofing/resource-exhaustion vectors. D11 is needed only if production rebinding is exercised; D28 freezes the administrative mechanism/reachability policy; D39 freezes per-station queue/staging/rate/retry/channel bounds.

**Tests:**

### M26.01 - physical-port source identification, not cryptographic authentication

Prove every Spectrum-link source identity is derived from the configured physical/virtual router-port binding and cannot be overridden by packet-carried source name/address. With `M26-cable-swap.json`/bench setup, replace the device/cable on a configured physical port and prove the new physical device inherits that port's configured identity until an administrator disables or changes the binding. The result must explicitly label this **physical-port identification**, not cryptographic authentication. If a production rebind/change sequence is tested, that portion is BLOCKED until D11; simple administrative disable may be tested through the frozen admin path.

**Evidence output:** `certification/results/micear/M26.01.json`

### M26.02 - complete input-validation surface

Fuzz every field explicitly listed by Section 26.2: protocol version; packet type; length; destination; station-name syntax; address range; sequence number; CRC; object metadata; and memory ranges requested by Spectrum operations. Each invalid input must be rejected before unsafe routing, persistence, object exposure, or Spectrum-memory writes. Validating one parser layer does not waive validation at the trust boundary where the field becomes authoritative.

**Evidence output:** `certification/results/micear/M26.02.json`

### M26.03 - per-station denial-of-service controls

BLOCKED until D39 freezes the applicable release bounds. Independently exhaust message queue, object staging, packet rate, retry count, and open-channel limits; exercise administrative disable. Each limit must fail explicitly and remain scoped so one station cannot corrupt another station/port's state or create unbounded storage/work. The exact numeric bounds come only from D39.

**Evidence output:** `certification/results/micear/M26.03.json`

### M26.04 - no Spectrum-link confidentiality claim; separate admin trust boundary

Capture a normal MIC/EAR exchange and verify the architecture supplies **no encryption and no confidentiality/protection claim against a person with physical access to router or cables**. Documentation/result metadata must state that limitation rather than implying secrecy from CRC/framing. Separately, if a network-reachable administrative interface exists, test it under D28/Section 10.3 for the frozen explicit authentication/authorization requirement and ensure it is not an unauthenticated wildcard management listener. Section 26 does not itself require TLS or another particular protected transport; if D28 chooses one, test that separate frozen requirement without attributing it to the Spectrum-link confidentiality model.

**Evidence output:** `certification/results/micear/M26.04.json`

**Pass rule:** PASS only when M26.01-M26.04 prove the exact physical-port trust model, all listed validation fields, every frozen bounded DoS control, and the explicit no-confidentiality claim without promoting unrelated firmware-update or transport-security requirements into Section 26. D11/D28/D39-dependent details remain BLOCKED_GATE until frozen.

## M27. Architecture section 27: Diagnostics and Observability

**Architecture authority:** `M` source lines 2920-2995, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Release diagnostic utility, exact router log fields/privacy default, controller/router timing trace fields, and Phase-5 administrative status view.

**Required artifacts:**

- `certification/gates/micear/D28.md`
- `tests/fixtures/micear/zx48-micear-diagnostic.tap`
- `certification/results/micear/M27-diagnostic-transcript.txt`
- `certification/hardware/micear/M27-loopback.csv`
- `certification/results/micear/M27-router-log.jsonl`
- `certification/hardware/micear/M27-timing-trace.csv`

**Artifact/source authority:** Project-built diagnostic TAP, real/virtual router/controller logs and traces, and administration status capture. D28 is required only for the concrete Phase-5 administration/status mechanism.

**Tests:**

### M27.01 - release Spectrum diagnostic utility

The release tape distribution must include `zx48-micear-diagnostic.tap`. Execute every required capability and capture its Spectrum-visible transcript: emit known MIC pulse patterns; detect known EAR patterns; display router presence; display assigned station name and address; measure/report successful frames; report CRC errors; report timeout errors; test attention notification; perform router loopback; send a small test stream; and transfer a small CODE block. Correlate pulse/loopback cases with `M27-loopback.csv`; a host-only diagnostic that bypasses the Spectrum stack does not satisfy this requirement.

**Evidence output:** `certification/results/micear/M27.01.json`

### M27.02 - router operational log schema and privacy default

Drive success, error, retry, controller-fault, stream, and object cases and inspect `M27-router-log.jsonl`. Every operational record must expose at least: timestamp; physical port; authenticated station; operation; source; destination; result; error code; retry count; byte count; controller fault. Verify source identity matches authenticated port binding. Place recognizable sensitive message text in a test payload and prove message contents are **not logged by default**; any explicit diagnostic mode that records payload must be separately enabled and cannot alter the default.

**Evidence output:** `certification/results/micear/M27.02.json`

### M27.03 - timing trace content and correlation

Capture `M27-timing-trace.csv` during receive, transmit, CRC failure, and deliberate buffer over/underrun. The controller/router diagnostic trace must provide, for each applicable event: port; edge timestamp; measured pulse width; decoded bit; frame boundary; CRC result; transmit schedule; underrun or overrun. Correlate the trace with the diagnostic TAP and oscilloscope/controller evidence so fields are measured/derived from the real timing path rather than fabricated after the fact by high-level router state.

**Evidence output:** `certification/results/micear/M27.03.json`

### M27.04 - administrative status view

The concrete status presentation is BLOCKED until the applicable D28 Phase-5 administrative mechanism is frozen. Then capture `M27-admin-status.json` while varying state and prove it shows at least: port; station name; address; online state; last seen; queued-message count; active transfer; errors; protocol version. Change each underlying value and prove the view updates from authoritative router/controller state rather than stale duplicated UI state.

**Evidence outputs:** `certification/results/micear/M27-admin-status.json`; `certification/results/micear/M27.04.json`

**Pass rule:** PASS only when M27.01-M27.04 prove every diagnostic utility capability, every minimum log/trace/status field, default payload privacy, and physical/virtual timing correlation. D28-dependent presentation remains BLOCKED_GATE until frozen.

## M28. Architecture section 28: Testing Strategy

**Architecture authority:** `M` source lines 2996-3136, inclusive. This block is a literal one-for-one certification mapping of every test level and every test item in Architecture #3 Section 28. No Section-28 item may be satisfied only by a neighboring aggregate test.

**Forensic test intent:** Prove the complete Architecture-#3 testing strategy, in source order, with concrete project-owned or measured artifacts for every item.

**Required artifacts:**

- `certification/manifests/micear-section28-tests.json`
- `tests/fixtures/micear/M28-spectrum-unit-vectors.json`
- `tests/fixtures/micear/M14-packet-vectors.json`
- `tests/fixtures/micear/M18-errors.json`
- `tests/fixtures/micear/M15-queue-durability.json`
- `tests/fixtures/micear/M25-fault-injection.json`
- `tests/fixtures/micear/zx48-micear-diagnostic.tap`
- `tests/fixtures/micear/zx48-micear-stack.tap`
- `tests/fixtures/micear/M28-router-vectors.json`
- `tests/fixtures/micear/zx48-router-test.sqlite`
- `tests/fixtures/micear/M28-async-message-vectors.json`
- `tests/fixtures/micear/zx48-micear-basic-immediate.tap`
- `tests/fixtures/micear/zx48-micear-basic-stored.tap`
- `tests/fixtures/micear/zx48-micear-object-transfer.tap`
- `tests/fixtures/micear/M28-transfer-vectors.json`
- `certification/gates/micear/D22.md`
- `tests/fixtures/micear/M28-virtual-two-node.json`
- `tests/fixtures/micear/M28-fault-injection.json`
- `tests/fixtures/micear/M28-endurance-plan.json`
- `certification/hardware/micear/M28-physical-link-matrix.csv`
- `certification/hardware/micear/M28-physical-link-captures-manifest.json`

**Artifact/source authority:** All TAP/JSON/SQLite fixtures are project-owned and generated from the Architecture-#3 resident-stack/router sources. Hardware matrix/captures are project measurements from identified real ZX Spectrum 48K Issue-2 boards and the frozen router/controller prototype. No ambiguous historical media filename is required by Section 28.

**Physical-capture identity rule:** `M28-physical-link-captures-manifest.json` must list every physical capture admitted to certification by exact relative filename, SHA-256, diagnostic case, Spectrum board/ULA/CPU identity, router-port/controller build identity, instrument/capture configuration, and repetition/run identifier. The capture directory alone is never certification evidence.

**Evidence-direction rule:** `certification/results/micear/M28-endurance-results.json` is generated by M28.09 from `M28-endurance-plan.json`; it is not a prerequisite input. Likewise every result path in the literal ledger is produced by its row and may not be pre-supplied as oracle evidence.

**Tests:**

### M28.01

Validate the Section-28 manifest itself: every literal test level/item at source lines 3003-3133 appears exactly once below, in source order, with a concrete fixture or measurement artifact and evidence path. Missing, duplicate, reordered, or invented entries fail.

**Evidence output:** `certification/results/micear/M28.01.json`

**Pass rule:** PASS only when the ledger is a one-for-one transcription of the architecture and all referenced artifacts exist or are correctly BLOCKED_GATE by an upstream deferred decision.

### M28 literal testing-strategy item ledger

| Subtest | Source line | Literal test item | Required input artifact | Evidence artifact |
|---|---:|---|---|---|
| M28.02 | 3003 | unit tests | `tests/fixtures/micear/M28-spectrum-unit-vectors.json` | `certification/results/micear/M28.02.json` |
| M28.03 | 3004 | protocol codec tests | `tests/fixtures/micear/M14-packet-vectors.json` | `certification/results/micear/M28.03.json` |
| M28.04 | 3005 | router service tests | `tests/fixtures/micear/M28-router-vectors.json`; `tests/fixtures/micear/zx48-router-test.sqlite` | `certification/results/micear/M28.04.json` |
| M28.05 | 3006 | timing-controller tests | `tests/fixtures/micear/M09-controller-stress.json` | `certification/results/micear/M28.05.json` |
| M28.06 | 3007 | analog bench tests | `certification/hardware/micear/M28-physical-link-matrix.csv` | `certification/results/micear/M28.06.json` |
| M28.07 | 3008 | real Spectrum integration tests | `tests/fixtures/micear/zx48-micear-diagnostic.tap`; identified Issue-2 boards in `certification/hardware/micear/M28-physical-link-matrix.csv` | `certification/results/micear/M28.07.json` |
| M28.08 | 3009 | simulator integration tests | `tests/fixtures/micear/M28-virtual-two-node.json`; `tests/fixtures/micear/zx48-micear-stack.tap` | `certification/results/micear/M28.08.json` |
| M28.09 | 3010 | endurance tests | `tests/fixtures/micear/M28-endurance-plan.json` | `certification/results/micear/M28-endurance-results.json` |
| M28.10 | 3011 | fault-injection tests | `tests/fixtures/micear/M28-fault-injection.json` | `certification/results/micear/M28.10.json` |
| M28.11 | 3018 | station parser | `tests/fixtures/micear/M28-spectrum-unit-vectors.json` | `certification/results/micear/M28.11.json` |
| M28.12 | 3019 | six-character validation | `tests/fixtures/micear/M28-spectrum-unit-vectors.json` | `certification/results/micear/M28.12.json` |
| M28.13 | 3020 | x.y validation | `tests/fixtures/micear/M28-spectrum-unit-vectors.json` | `certification/results/micear/M28.13.json` |
| M28.14 | 3021 | name normalization | `tests/fixtures/micear/M28-spectrum-unit-vectors.json` | `certification/results/micear/M28.14.json` |
| M28.15 | 3022 | packet encoding | `tests/fixtures/micear/M14-packet-vectors.json` | `certification/results/micear/M28.15.json` |
| M28.16 | 3023 | packet decoding | `tests/fixtures/micear/M14-packet-vectors.json` | `certification/results/micear/M28.16.json` |
| M28.17 | 3024 | CRC | `tests/fixtures/micear/M14-packet-vectors.json` | `certification/results/micear/M28.17.json` |
| M28.18 | 3025 | sequence handling | `tests/fixtures/micear/M14-packet-vectors.json` | `certification/results/micear/M28.18.json` |
| M28.19 | 3026 | error mapping | `tests/fixtures/micear/M18-errors.json` | `certification/results/micear/M28.19.json` |
| M28.20 | 3027 | object metadata | `tests/fixtures/micear/M28-transfer-vectors.json` | `certification/results/micear/M28.20.json` |
| M28.21 | 3028 | attention state machine | `tests/fixtures/micear/M28-async-message-vectors.json` | `certification/results/micear/M28.21.json` |
| M28.22 | 3029 | hook installation and removal | `tests/fixtures/micear/zx48-micear-stack.tap` | `certification/results/micear/M28.22.json` |
| M28.23 | 3036 | unique name enforcement | `tests/fixtures/micear/M28-router-vectors.json` | `certification/results/micear/M28.23.json` |
| M28.24 | 3037 | unique address enforcement | `tests/fixtures/micear/M28-router-vectors.json` | `certification/results/micear/M28.24.json` |
| M28.25 | 3038 | port source authentication | `tests/fixtures/micear/M28-router-vectors.json` | `certification/results/micear/M28.25.json` |
| M28.26 | 3039 | unknown versus unavailable distinction | `tests/fixtures/micear/M28-router-vectors.json` | `certification/results/micear/M28.26.json` |
| M28.27 | 3040 | busy handling | `tests/fixtures/micear/M28-router-vectors.json` | `certification/results/micear/M28.27.json` |
| M28.28 | 3041 | message commit semantics | `tests/fixtures/micear/M15-queue-durability.json`; `tests/fixtures/micear/zx48-router-test.sqlite` | `certification/results/micear/M28.28.json` |
| M28.29 | 3042 | message redelivery after failed acknowledgment | `tests/fixtures/micear/M15-queue-durability.json` | `certification/results/micear/M28.29.json` |
| M28.30 | 3043 | object staging integrity | `tests/fixtures/micear/M28-transfer-vectors.json` | `certification/results/micear/M28.30.json` |
| M28.31 | 3044 | queue bounds | `tests/fixtures/micear/M25-fault-injection.json` | `certification/results/micear/M28.31.json` |
| M28.32 | 3045 | protocol-version mismatch | `tests/fixtures/micear/M14-packet-vectors.json` | `certification/results/micear/M28.32.json` |
| M28.33 | 3046 | restart persistence | `tests/fixtures/micear/zx48-router-test.sqlite` | `certification/results/micear/M28.33.json` |
| M28.34 | 3047 | process termination immediately after successful queue commit | `tests/fixtures/micear/M15-queue-durability.json`; `tests/fixtures/micear/zx48-router-test.sqlite` | `certification/results/micear/M28.34.json` |
| M28.35 | 3048 | atomic metadata/payload visibility after recovery | `tests/fixtures/micear/M15-queue-durability.json`; `tests/fixtures/micear/zx48-router-test.sqlite` | `certification/results/micear/M28.35.json` |
| M28.36 | 3055 | multiple Issue 2 boards | `certification/hardware/micear/M28-physical-link-matrix.csv` | `certification/results/micear/M28.36.json` |
| M28.37 | 3056 | multiple power supplies | `certification/hardware/micear/M28-physical-link-matrix.csv` | `certification/results/micear/M28.37.json` |
| M28.38 | 3057 | multiple cable lengths | `certification/hardware/micear/M28-physical-link-matrix.csv` | `certification/results/micear/M28.38.json` |
| M28.39 | 3058 | repeated plug insertion | `certification/hardware/micear/M28-physical-link-matrix.csv` | `certification/results/micear/M28.39.json` |
| M28.40 | 3059 | amplitude variation | `certification/hardware/micear/M28-physical-link-captures-manifest.json` | `certification/results/micear/M28.40.json` |
| M28.41 | 3060 | noise injection | `certification/hardware/micear/M28-physical-link-captures-manifest.json` | `certification/results/micear/M28.41.json` |
| M28.42 | 3061 | intentional pulse-width distortion | `certification/hardware/micear/M28-physical-link-captures-manifest.json` | `certification/results/micear/M28.42.json` |
| M28.43 | 3062 | lost edges | `tests/fixtures/micear/M28-fault-injection.json`; `certification/hardware/micear/M28-physical-link-captures-manifest.json` | `certification/results/micear/M28.43.json` |
| M28.44 | 3063 | duplicate frames | `tests/fixtures/micear/M28-fault-injection.json` | `certification/results/micear/M28.44.json` |
| M28.45 | 3064 | corrupted CRC | `tests/fixtures/micear/M28-fault-injection.json` | `certification/results/micear/M28.45.json` |
| M28.46 | 3065 | delayed acknowledgments | `tests/fixtures/micear/M28-fault-injection.json` | `certification/results/micear/M28.46.json` |
| M28.47 | 3066 | controller/Pi transport loss during receive and transmit | `tests/fixtures/micear/M28-fault-injection.json` | `certification/results/micear/M28.47.json` |
| M28.48 | 3067 | controller reset/watchdog safe-idle behavior | `tests/fixtures/micear/M28-fault-injection.json` | `certification/results/micear/M28.48.json` |
| M28.49 | 3074 | message arrives while BASIC prompt is idle | `tests/fixtures/micear/M28-async-message-vectors.json`; `tests/fixtures/micear/zx48-micear-basic-immediate.tap` | `certification/results/micear/M28.49.json` |
| M28.50 | 3075 | message arrives while a line is being edited | `tests/fixtures/micear/M28-async-message-vectors.json`; `tests/fixtures/micear/zx48-micear-basic-immediate.tap` | `certification/results/micear/M28.50.json` |
| M28.51 | 3076 | message arrives while BASIC program runs | `tests/fixtures/micear/M28-async-message-vectors.json`; `tests/fixtures/micear/zx48-micear-basic-stored.tap` | `certification/results/micear/M28.51.json` |
| M28.52 | 3077 | message is queued while arbitrary game code runs | `tests/fixtures/micear/M28-async-message-vectors.json`; one compatible developer-selected file directly in `WZSN-PRIVATE-TEST-MEDIA/` (the architecture intentionally mandates no fixed private filename) | `certification/results/micear/M28.52.json` |
| M28.53 | 3078-3079 | attention detected while compatible game/demo code runs with resident RAM and interrupt hook preserved | `tests/fixtures/micear/M28-async-message-vectors.json`; `tests/fixtures/micear/zx48-micear-stack.tap`; one compatible developer-selected file directly in `WZSN-PRIVATE-TEST-MEDIA/` (no fixed private filename mandated) | `certification/results/micear/M28.53.json` |
| M28.54 | 3080 | attention pattern during no traffic | `tests/fixtures/micear/M28-async-message-vectors.json` | `certification/results/micear/M28.54.json` |
| M28.55 | 3081 | attention suppressed during packet transfer | `tests/fixtures/micear/M28-async-message-vectors.json` | `certification/results/micear/M28.55.json` |
| M28.56 | 3082 | false-pattern rejection | `tests/fixtures/micear/M28-async-message-vectors.json` | `certification/results/micear/M28.56.json` |
| M28.57 | 3083 | repeated attention until acknowledgment | `tests/fixtures/micear/M28-async-message-vectors.json` | `certification/results/micear/M28.57.json` |
| M28.58 | 3084 | message retained after failed retrieval | `tests/fixtures/micear/M15-queue-durability.json` | `certification/results/micear/M28.58.json` |
| M28.59 | 3085 | message removed only after successful close/acknowledgment | `tests/fixtures/micear/M15-queue-durability.json` | `certification/results/micear/M28.59.json` |
| M28.60 | 3086 | message remains queued when resident interrupt notification is unavailable | `tests/fixtures/micear/M28-async-message-vectors.json` | `certification/results/micear/M28.60.json` |
| M28.61 | 3087 | foreground poll reconciles pending state after interrupt-hook restoration | `tests/fixtures/micear/M28-async-message-vectors.json`; `tests/fixtures/micear/zx48-micear-stack.tap` | `certification/results/micear/M28.61.json` |
| M28.62 | 3094 | immediate mode | `tests/fixtures/micear/zx48-micear-basic-immediate.tap` | `certification/results/micear/M28.62.json` |
| M28.63 | 3095 | numbered BASIC program where supported | `tests/fixtures/micear/zx48-micear-basic-stored.tap` | `certification/results/micear/M28.63.json` |
| M28.64 | 3096 | success path | `tests/fixtures/micear/zx48-micear-basic-immediate.tap`; `tests/fixtures/micear/zx48-micear-basic-stored.tap` | `certification/results/micear/M28.64.json` |
| M28.65 | 3097 | each defined error path | `tests/fixtures/micear/M18-errors.json`; `tests/fixtures/micear/zx48-micear-basic-immediate.tap`; `tests/fixtures/micear/zx48-micear-basic-stored.tap` | `certification/results/micear/M28.65.json` |
| M28.66 | 3098 | LIST behavior | `tests/fixtures/micear/zx48-micear-basic-stored.tap` | `certification/results/micear/M28.66.json` |
| M28.67 | 3099 | syntax error location | `tests/fixtures/micear/M18-errors.json` | `certification/results/micear/M28.67.json` |
| M28.68 | 3100 | line and statement report | `tests/fixtures/micear/M18-errors.json` | `certification/results/micear/M28.68.json` |
| M28.69 | 3101 | unrelated normal BASIC errors | `tests/fixtures/micear/zx48-micear-basic-immediate.tap` | `certification/results/micear/M28.69.json` |
| M28.70 | 3108 | zero or minimum legal size | `tests/fixtures/micear/zx48-micear-object-transfer.tap`; `tests/fixtures/micear/M28-transfer-vectors.json` | `certification/results/micear/M28.70.json` |
| M28.71 | 3109 | typical size | `tests/fixtures/micear/zx48-micear-object-transfer.tap`; `tests/fixtures/micear/M28-transfer-vectors.json` | `certification/results/micear/M28.71.json` |
| M28.72 | 3110 | maximum supported size | `tests/fixtures/micear/zx48-micear-object-transfer.tap`; `tests/fixtures/micear/M28-transfer-vectors.json`; `certification/gates/micear/D22.md` | `certification/results/micear/M28.72.json` |
| M28.73 | 3111 | exact checksum match | `tests/fixtures/micear/M28-transfer-vectors.json` | `certification/results/micear/M28.73.json` |
| M28.74 | 3112 | one corrupted chunk | `tests/fixtures/micear/M28-transfer-vectors.json` | `certification/results/micear/M28.74.json` |
| M28.75 | 3113 | lost acknowledgment | `tests/fixtures/micear/M28-fault-injection.json` | `certification/results/micear/M28.75.json` |
| M28.76 | 3114 | duplicate chunk | `tests/fixtures/micear/M28-transfer-vectors.json` | `certification/results/micear/M28.76.json` |
| M28.77 | 3115 | receiver cancellation | `tests/fixtures/micear/M28-transfer-vectors.json` | `certification/results/micear/M28.77.json` |
| M28.78 | 3116 | router restart during staging | `tests/fixtures/micear/zx48-router-test.sqlite`; `tests/fixtures/micear/M28-transfer-vectors.json` | `certification/results/micear/M28.78.json` |
| M28.79 | 3123 | repeated short messages | `tests/fixtures/micear/M28-endurance-plan.json` | `certification/results/micear/M28.79.json` |
| M28.80 | 3124 | repeated screen transfers | `tests/fixtures/micear/M28-endurance-plan.json`; `tests/fixtures/micear/zx48-micear-object-transfer.tap` | `certification/results/micear/M28.80.json` |
| M28.81 | 3125 | multi-port concurrent transfers | `tests/fixtures/micear/M28-endurance-plan.json` | `certification/results/micear/M28.81.json` |
| M28.82 | 3126 | attention signalling over many hours | `tests/fixtures/micear/M28-endurance-plan.json` | `certification/results/micear/M28.82.json` |
| M28.83 | 3127 | queue fill and drain cycles | `tests/fixtures/micear/M28-endurance-plan.json`; `tests/fixtures/micear/zx48-router-test.sqlite` | `certification/results/micear/M28.83.json` |
| M28.84 | 3128 | router/controller reconnect cycles | `tests/fixtures/micear/M28-endurance-plan.json` | `certification/results/micear/M28.84.json` |
| M28.85 | 3129 | repeated daemon restarts after acknowledged message commits | `tests/fixtures/micear/M28-endurance-plan.json`; `tests/fixtures/micear/zx48-router-test.sqlite` | `certification/results/micear/M28.85.json` |
| M28.86 | 3130 | deterministic WZSN multi-machine virtual runs with master-tick trace comparison | `tests/fixtures/micear/M28-virtual-two-node.json`; `tests/fixtures/micear/zx48-micear-stack.tap` | `certification/results/micear/M28.86.json` |
| M28.87 | 3131 | WZSN NONE / INTERFACE1 / EAR_MIC mutual-exclusion transitions | `tests/fixtures/networking/wzsn-networking-modes-v1.json` | `certification/results/micear/M28.87.json` |
| M28.88 | 3132 | EAR_MIC fidelity run after loading the distributed RAM stack | `tests/fixtures/micear/zx48-micear-stack.tap`; `tests/fixtures/micear/M28-virtual-two-node.json` | `certification/results/micear/M28.88.json` |
| M28.89 | 3133 | proof that EAR_MIC mode cannot expose Interface 1/Microdrive/original ZX Net | `tests/fixtures/networking/wzsn-networking-modes-v1.json` | `certification/results/micear/M28.89.json` |

**Ledger pass rule:** Every row must execute or be explicitly BLOCKED_GATE by the named architecture decision. Any missing input artifact, unexplained skip, private-media filename mandate, or mismatch between source-line semantics and the row is FAIL.

## M29. Architecture section 29: Simulator and Emulator Integration

**Architecture authority:** `M` source lines 3137-3277, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** WZSN simulator/emulator integration fidelity.

**Required artifacts:**

- `warajevo-zx-spectrum-next-architecture.md`
- `warajevo-zx-spectrum-next-ui-architecture.md`
- `tests/fixtures/timing/zx48-pal-timing-v1.json`
- `tests/fixtures/networking/wzsn-networking-modes-v1.json`
- `tests/fixtures/ui/U23-networking-transition.json`
- `certification/results/core/C24.01.json`
- `certification/results/core/C24.04.json`
- `tests/fixtures/micear/M29-two-node-virtual.json`
- `tests/fixtures/micear/M29-speed-invariance.json`
- `tests/fixtures/micear/M29-boundary-separation.json`
- `tests/fixtures/micear/zx48-micear-stack.tap`
- `certification/gates/micear/P0-wzsn-48k-issue2-profile.md`
- `certification/gates/micear/D30.md`

**Artifact/source authority:** The attached/canonical Core and UI architecture documents are upstream authorities for master-tick timing, networking-mode/cold-reconfiguration semantics, and application orchestration. The named Core/UI fixtures/results prove those upstream contracts before Architecture #3 consumes them. `P0-wzsn-48k-issue2-profile.md` freezes the exact WZSN 48K Issue-2 profile/ROM/hardware assumptions used by the virtual proof. Project virtual coordinator fixtures and the exact project-built stack TAP prove the Architecture-3-specific integration.

**Tests:**

### M29.01

Using the upstream Core timing contract and `zx48-pal-timing-v1.json`, use only `OnMicLevelChanged(level, master_tick)` and `GetEarLevel(master_tick)` as the fidelity boundary. Convert all Spectrum-side T-state engineering values through the exact frozen Core profile ratio; no second emulated clock exists. Run >=2 independent machine contexts under one deterministic virtual-time coordinator per D30; use the same router core/protocol with an injected time source; repeated runs are identical; no direct SendByte shortcut can satisfy fidelity.

**Evidence output:** `certification/results/micear/M29.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### M29.02

Using `wzsn-networking-modes-v1.json`, `U23-networking-transition.json`, and the upstream C24 results, exercise the exact WZSN mode set `NONE`, `INTERFACE1`, `EAR_MIC`: `NONE` and `INTERFACE1` disconnect the Architecture-3 virtual router; `EAR_MIC` is available only for the Phase-0-frozen certified 48K Issue-2 profile, excludes IF1/MDR/original ZX Net, and owns the cassette peer. Compare 25/50/100/200/400/800/Unlimited host pacing and require identical virtual-protocol master-tick event order. Prove no networking ROM paging or auto-install, load the same distributed stack through BOOTSTRAP_TAPE, and verify upstream cold mode reconfiguration prevents hook leakage while preserving application pause state. Multiple independent GUI processes/Control Ports are not automatically a virtual network.

**Evidence output:** `certification/results/micear/M29.02.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### M29.03

Using `tests/fixtures/micear/M29-boundary-separation.json`, prove the virtual port replaces analog conditioning/physical timing hardware while preserving the same MIC/EAR pulse stream, routed-packet framing, and router service behavior, but does **not** claim to model cable, comparator, amplitude, grounding, or other Issue-2 analog properties. Certification metadata must route those properties only to the real-hardware/bench evidence tests. If an accelerated/direct-byte simulator mode exists, prove it is explicitly separate from fidelity mode and that its result cannot mark any pulse-level/physical-layer fidelity criterion PASS.

**Evidence output:** `certification/results/micear/M29.03.json`

**Pass rule:** PASS only when the virtual/physical authority boundary and accelerated-mode exclusion are structural and machine-checkable; any analog-fidelity claim from the virtual port or any byte-shortcut fidelity PASS is FAIL.

## M30. Architecture section 30: Implementation Phases

**Architecture authority:** `M` source lines 3278-3496, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Implementation phases and prerequisite gates.

**Required artifacts:**

- `certification/manifests/micear-phase-gates.json`
- `certification/manifests/micear-controller-build.json`
- `tests/fixtures/micear/M30-controller-firmware-update.json`

**Artifact/source authority:** Project phase-gate manifest generated from Sections30 and32.

**Tests:**

### M30.01

Before each Phase0..7 entry/exit, verify Architecture #1/#2 prerequisites and every Section32 decision required by that phase are closed; execute exact phase exit tests; block dependent implementation/certification when a gate artifact is absent or stale.

**Evidence output:** `certification/results/micear/M30.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### M30 literal phase prerequisite/action ledger

Every Section-30 freeze or implementation bullet is separately evidenced below. A row whose decision is intentionally deferred is `BLOCKED_GATE` until its exact named artifact exists; no implementation test may silently choose the missing value.

| ID | Source line | Phase | Literal requirement | Required gate/evidence artifact |
|---|---:|---|---|---|
| M30.P001 | 3290 | 0 freeze | exact upstream WZSN 48K Issue-2 profile/variant identity, ROM hash, hardware assumptions | `certification/gates/micear/P0-wzsn-48k-issue2-profile.md` |
| M30.P002 | 3292 | 0 freeze | remaining ordinary/reserved address ranges | `certification/gates/micear/D10.md` |
| M30.P003 | 3293 | 0 freeze | registration policy beyond preconfigured binding | `certification/gates/micear/D11.md` |
| M30.P004 | 3294 | 0 freeze | packet header and packet-type numeric assignments | `certification/gates/micear/D36.md` |
| M30.P005 | 3295 | 0 freeze | Phase-3 channel direction/mixed-I/O semantics | `certification/gates/micear/D40.md` |
| M30.P006 | 3296 | 0 freeze | physical/link frame preamble and sync | `certification/gates/micear/D06.md` |
| M30.P007 | 3297 | 0 freeze | within-version protocol negotiation rules | `certification/gates/micear/P0-protocol-negotiation.md` |
| M30.P008 | 3298 | 0 freeze | CRC polynomial/init/reflection/byte order | `certification/gates/micear/D07.md` |
| M30.P009 | 3299 | 0 freeze | virtual-proof payload-size candidate | `certification/gates/micear/D08.md` |
| M30.P010 | 3300 | 0 freeze | Phase-1 retry counts/timeouts | `certification/gates/micear/D09.md` |
| M30.P011 | 3301 | 0 freeze | stream commit, immutable binding, queue order, retention | `certification/gates/micear/P0-stream-commit-semantics.md`; `tests/fixtures/micear/M15-queue-durability.json` |
| M30.P012 | 3303 | 0 freeze | transaction IDs and packet sequence wrap/duplicate window | `certification/gates/micear/D45.md`; `certification/gates/micear/D46.md` |
| M30.P013 | 3305 | 0 freeze | broadcast exclusion or complete semantics | `certification/manifests/micear/D20-closed-contract.json`; `certification/gates/micear/P0-broadcast-policy.md` |
| M30.P014 | 3306 | 0 freeze | object metadata format | `certification/gates/micear/P0-object-metadata.md` |
| M30.P015 | 3307 | 0 freeze | maximum initial message size | `certification/gates/micear/D22.md` |
| M30.P016 | 3308 | 0 freeze | Pi/controller framing sufficient for simulation stubs | `certification/gates/micear/P0-controller-stub-framing.md` |
| M30.P017 | 3309 | 0 freeze | daemon/module boundary and repository placement | `certification/gates/micear/D31.md`; `certification/gates/micear/D35.md` |
| M30.P018 | 3310 | 0 freeze | router daemon and virtual-proof language/build system | `certification/gates/micear/D50.md` |
| M30.P019 | 3311 | 0 freeze | initial error-code namespace | `certification/gates/micear/P0-error-code-namespace.md` |
| M30.P020 | 3312 | 0 freeze | stack/daemon/controller versioning policy | `certification/gates/micear/P0-versioning-policy.md` |
| M30.P021 | 3321 | 1 implement | packet codec | `certification/results/micear/M14.01.json` |
| M30.P022 | 3322 | 1 implement | router registry | `certification/results/micear/M11.01.json`; `certification/results/micear/M15.01.json` |
| M30.P023 | 3323 | 1 implement | virtual ports | `tests/fixtures/micear/M29-boundary-separation.json`; `certification/results/micear/M29.03.json` |
| M30.P024 | 3324 | 1 implement | deterministic shared-master-tick coordinator | `tests/fixtures/micear/M29-two-node-virtual.json`; `certification/results/micear/M29.01.json` |
| M30.P025 | 3325 | 1 implement | two WZSN 48K contexts in EAR_MIC | `tests/fixtures/micear/M29-two-node-virtual.json` |
| M30.P026 | 3326 | 1 implement | load same distributed resident stack, no emulator-only ROM | `tests/fixtures/micear/zx48-micear-stack.tap`; `certification/results/micear/M21.01.json` |
| M30.P027 | 3328 | 1 implement | direct message stream | `tests/fixtures/micear/M17-command-semantics.json`; `certification/results/micear/M17.01.json` |
| M30.P028 | 3329 | 1 implement | persistent queue commit/restart store | `tests/fixtures/micear/zx48-router-test.sqlite`; `tests/fixtures/micear/M15-queue-durability.json` |
| M30.P029 | 3330 | 1 implement | success/error mapping | `tests/fixtures/micear/M18-errors.json`; `certification/results/micear/M18.01.json` |
| M30.P030 | 3341 | 2 freeze | diagnostic hardware port count | `certification/gates/micear/P2-diagnostic-port-count.md` |
| M30.P031 | 3342 | 2 freeze | assembler/build toolchain and calling convention | `certification/gates/micear/D49.md`; `certification/gates/micear/D29.md` |
| M30.P032 | 3343 | 2 freeze | provisional diagnostic high-memory load range | `certification/gates/micear/D16.md` |
| M30.P033 | 3344 | 2 freeze | minimum immediate-mode BASIC error-hook mechanism | `certification/gates/micear/D14.md` |
| M30.P034 | 3345 | 2 freeze | conservative bench pulse profile | `certification/gates/micear/D05.md` |
| M30.P035 | 3346 | 2 freeze | turnaround guard test procedure | `certification/gates/micear/D33.md` |
| M30.P036 | 3347 | 2 freeze | prototype timing-controller technology | `certification/gates/micear/D41.md` |
| M30.P037 | 3348 | 2 freeze | prototype Pi/controller transport/framing | `certification/gates/micear/D42.md` |
| M30.P038 | 3349 | 2 freeze | analog prototype schematic/protection limits | `certification/gates/micear/D01.md` |
| M30.P039 | 3354 | 2 implement | MIC pattern transmitter | `tests/fixtures/micear/zx48-micear-diagnostic.tap`; `certification/hardware/micear/M27-loopback.csv` |
| M30.P040 | 3355 | 2 implement | EAR pattern detector | `tests/fixtures/micear/zx48-micear-diagnostic.tap`; `certification/hardware/micear/M27-loopback.csv` |
| M30.P041 | 3356 | 2 implement | one-port analog prototype | `certification/hardware/micear/M06-port-wiring-record.json` |
| M30.P042 | 3357 | 2 implement | timing controller | `tests/fixtures/micear/M09-controller-stress.json`; `certification/manifests/micear-controller-build.json` |
| M30.P043 | 3358 | 2 implement | loopback tests | `certification/hardware/micear/M27-loopback.csv` |
| M30.P044 | 3359 | 2 implement | virtual/physical trace correlation | `certification/hardware/micear/M30-P2-trace-correlation.json` |
| M30.P045 | 3369 | 3 freeze | exact turnaround guard | `certification/gates/micear/D33.md` |
| M30.P046 | 3370 | 3 freeze | initial physical pulse timings | `certification/gates/micear/D05.md` |
| M30.P047 | 3371 | 3 freeze | initial release packet payload size | `certification/gates/micear/D08.md` |
| M30.P048 | 3372 | 3 freeze | integrated high-memory range | `certification/gates/micear/D16.md` |
| M30.P049 | 3373 | 3 freeze | immediate BASIC error-hook install/recovery | `certification/gates/micear/D14.md` |
| M30.P050 | 3374 | 3 freeze | station liveness/disconnect policy | `certification/gates/micear/D48.md` |
| M30.P051 | 3375 | 3 freeze | attention waveform/false-positive threshold | `certification/gates/micear/D17.md` |
| M30.P052 | 3376 | 3 freeze | retry/timeout values | `certification/gates/micear/D09.md` |
| M30.P053 | 3377 | 3 freeze | max queued message size + message integrity | `certification/gates/micear/D22.md`; `certification/gates/micear/D43.md` |
| M30.P054 | 3378 | 3 freeze | final message channel direction/mixed-I/O | `certification/gates/micear/D40.md` |
| M30.P055 | 3379 | 3 freeze | router crash recovery for committed messages | `certification/gates/micear/D24.md` |
| M30.P056 | 3380 | 3 freeze | station rebinding of queued data | `certification/gates/micear/D47.md` |
| M30.P057 | 3384 | 3 implement | two independent ports | `certification/hardware/micear/M06-cross-port-isolation.csv` |
| M30.P058 | 3385 | 3 implement | registration | `tests/fixtures/micear/M11-address-registration.json` |
| M30.P059 | 3386 | 3 implement | OPEN#/PRINT#/CLOSE# send | `tests/fixtures/micear/zx48-micear-basic-immediate.tap`; `tests/fixtures/micear/M17-command-semantics.json` |
| M30.P060 | 3387 | 3 implement | INPUT# retrieval | `tests/fixtures/micear/zx48-micear-basic-immediate.tap`; `tests/fixtures/micear/M17-command-semantics.json` |
| M30.P061 | 3388 | 3 implement | attention flag | `tests/fixtures/micear/M28-async-message-vectors.json` |
| M30.P062 | 3389 | 3 implement | FIFO message selection | `tests/fixtures/micear/M15-queue-durability.json` |
| M30.P063 | 3390 | 3 implement | unknown/unavailable errors | `tests/fixtures/micear/M18-errors.json` |
| M30.P064 | 3391 | 3 implement | durable commit + process restart recovery | `tests/fixtures/micear/zx48-router-test.sqlite`; `tests/fixtures/micear/M15-queue-durability.json` |
| M30.P065 | 3400 | 4 freeze | object staging strategy | `certification/gates/micear/D21.md` |
| M30.P066 | 3401 | 4 freeze | maximum object size | `certification/gates/micear/D22.md` |
| M30.P067 | 3402 | 4 freeze | object checksum/order/metadata | `certification/gates/micear/D43.md`; `certification/gates/micear/P0-object-metadata.md` |
| M30.P068 | 3403 | 4 freeze | Phase-4 retry/timeouts | `certification/gates/micear/D09.md` |
| M30.P069 | 3407 | 4 implement | CODE | `tests/fixtures/micear/zx48-micear-object-transfer.tap`; `tests/fixtures/micear/M28-transfer-vectors.json` |
| M30.P070 | 3408 | 4 implement | SCREEN$ | `tests/fixtures/micear/zx48-micear-object-transfer.tap`; `tests/fixtures/micear/M28-transfer-vectors.json` |
| M30.P071 | 3409 | 4 implement | BASIC program | `tests/fixtures/micear/zx48-micear-object-transfer.tap`; `tests/fixtures/micear/M28-transfer-vectors.json` |
| M30.P072 | 3410 | 4 implement | numeric + character arrays | `tests/fixtures/micear/zx48-micear-object-transfer.tap`; `tests/fixtures/micear/M28-transfer-vectors.json` |
| M30.P073 | 3411 | 4 implement | VERIFY | `tests/fixtures/micear/zx48-micear-object-transfer.tap`; `tests/fixtures/micear/M28-transfer-vectors.json` |
| M30.P074 | 3412 | 4 implement | MERGE | `tests/fixtures/micear/zx48-micear-object-transfer.tap`; `tests/fixtures/micear/M28-transfer-vectors.json` |
| M30.P075 | 3422 | 5 freeze | multi-port reference count >=4 | `certification/gates/micear/D04.md` |
| M30.P076 | 3423 | 5 freeze | production-intent port topology | `certification/gates/micear/P5-reference-port-topology.md` |
| M30.P077 | 3424 | 5 freeze | Raspberry Pi deployment/OS/service baseline | `certification/gates/micear/D51.md` |
| M30.P078 | 3425 | 5 freeze | admin mechanism/access-control policy | `certification/gates/micear/D28.md` |
| M30.P079 | 3426 | 5 freeze | per-station resource limits | `certification/gates/micear/D39.md` |
| M30.P080 | 3427 | 5 freeze | router/controller watchdog/reconnect | `certification/gates/micear/P5-watchdog-reconnect.md` |
| M30.P081 | 3431 | 5 implement | four or more ports | `certification/hardware/micear/M30-P5-multiport.json` |
| M30.P082 | 3432 | 5 implement | concurrent independent links | `tests/fixtures/micear/M28-endurance-plan.json` |
| M30.P083 | 3433 | 5 implement | persistent registry | `tests/fixtures/micear/zx48-router-test.sqlite` |
| M30.P084 | 3434 | 5 implement | bounded queues/staging | `tests/fixtures/micear/M25-fault-injection.json` |
| M30.P085 | 3435 | 5 implement | local administration | `tests/fixtures/micear/M10-router-api.json` |
| M30.P086 | 3436 | 5 implement | diagnostics/status surfaces | `certification/results/micear/M27-diagnostic-transcript.txt` |
| M30.P087 | 3445 | 6 freeze | exact IF1 MOVE compatibility baseline | `certification/gates/micear/D13.md` |
| M30.P088 | 3446 | 6 freeze | final ROM error-hook behavior | `certification/gates/micear/D14.md` |
| M30.P089 | 3448 | 6 freeze | complete stored-BASIC strategy | `certification/gates/micear/D15.md` |
| M30.P090 | 3449 | 6 freeze | binary image header/tape packaging | `certification/gates/micear/D37.md` |
| M30.P091 | 3450 | 6 freeze | final Spectrum stack calling convention | `certification/gates/micear/D29.md` |
| M30.P092 | 3451 | 6 freeze | release-final high-memory map | `certification/gates/micear/D16.md` |
| M30.P093 | 3453 | 6 freeze | final report letters/numeric identifiers | `certification/gates/micear/D25.md` |
| M30.P094 | 3454 | 6 freeze | pending-message notification presentation | `certification/gates/micear/D18.md` |
| M30.P095 | 3458 | 6 implement/validate | immediate forms | `tests/fixtures/micear/zx48-micear-basic-immediate.tap` |
| M30.P096 | 3459 | 6 implement/validate | stored-program forms | `tests/fixtures/micear/zx48-micear-basic-stored.tap` |
| M30.P097 | 3460 | 6 implement/validate | channel behavior | `tests/fixtures/micear/M17-command-semantics.json` |
| M30.P098 | 3461 | 6 implement/validate | MOVE | `tests/fixtures/micear/M17-command-semantics.json`; `certification/gates/micear/D13.md` |
| M30.P099 | 3462 | 6 implement/validate | line/statement reports | `tests/fixtures/micear/M18-errors.json` |
| M30.P100 | 3463 | 6 implement/validate | complete tape installer | `tests/fixtures/micear/zx48-micear-stack.tap` |
| M30.P101 | 3464 | 6 implement/validate | router-endpoint two-field message retrieval | `tests/fixtures/micear/M17-command-semantics.json` |
| M30.P102 | 3474 | 7 freeze | final isolation strategy/ratings | `certification/gates/micear/D02.md` |
| M30.P103 | 3475 | 7 freeze | connector/cable specification | `certification/gates/micear/D03.md` |
| M30.P104 | 3476 | 7 freeze | production PCB port count/layout | `certification/gates/micear/D04.md` |
| M30.P105 | 3477 | 7 freeze | final electrical components/protection | `certification/gates/micear/P7-final-electrical-values.md` |
| M30.P106 | 3478 | 7 freeze | final conservative + optional faster profile | `certification/gates/micear/D05.md` |
| M30.P107 | 3479 | 7 freeze | controller build/update toolchain + security/recovery | `certification/gates/micear/D32.md`; `certification/gates/micear/P7-controller-build-toolchain.md` |
| M30.P108 | 3480 | 7 freeze | release-profile buffer/flow-control margins | `certification/gates/micear/D44.md` |
| M30.P109 | 3481 | 7 freeze | sudden-power-loss expectations/recovery | `certification/gates/micear/D24.md` |
| M30.P110 | 3485 | 7 implement | production isolation | `certification/hardware/micear/M08-fault-isolation.csv` |
| M30.P111 | 3486 | 7 implement | production PCB | `certification/hardware/micear/M30-P7-production-pcb.json` |
| M30.P112 | 3487 | 7 implement | watchdogs | `tests/fixtures/micear/M28-fault-injection.json` |
| M30.P113 | 3488 | 7 implement | controller firmware update | `certification/gates/micear/D32.md`; `tests/fixtures/micear/M30-controller-firmware-update.json` |
| M30.P114 | 3489 | 7 implement | endurance/fault injection | `tests/fixtures/micear/M28-endurance-plan.json`; `tests/fixtures/micear/M28-fault-injection.json` |
| M30.P115 | 3490 | 7 implement | complete documentation/manufacturing/test records | `certification/manifests/micear-manufacturing-test-records.json` |

**Ledger pass rule:** All 115 rows are required in source order. Each freeze row must point to an existing, review-approved gate artifact before dependent work can PASS; each implementation row must point to executed evidence. Missing/stale artifact, implicit design choice, or evidence from a different phase is FAIL.

### M30 literal phase-exit ledger

| Subtest | Source line | Phase | Literal exit gate | Primary proof test(s) | Required proof artifact(s) | Evidence artifact |
|---|---:|---|---|---|---|---|
| M30.G01 | 3314 | Phase 0 - Specification freeze | the packet codec, router registry, and virtual Spectrum stack can be ticketed without an implementer choosing a wire-format behavior locally. | M30.P001-M30.P020; M14; M15; M22; M29 | `certification/gates/micear/D06.md`; `certification/gates/micear/D07.md`; `certification/gates/micear/D08.md`; `certification/gates/micear/D09.md`; `certification/gates/micear/D10.md`; `certification/gates/micear/D11.md`; `certification/gates/micear/D31.md`; `certification/gates/micear/D35.md`; `certification/gates/micear/D36.md`; `certification/gates/micear/D38.md`; `certification/gates/micear/D45.md`; `certification/gates/micear/D46.md`; `certification/gates/micear/D50.md`; `certification/gates/micear/P0-protocol-negotiation.md`; `certification/gates/micear/P0-object-metadata.md`; `certification/manifests/micear/D20-closed-contract.json`; `certification/gates/micear/P0-broadcast-policy.md` | `certification/results/micear/M30.G01.json` |
| M30.G02 | 3332 | Phase 1 - Virtual proof | two simulated Spectrums in `EAR_MIC` mode exchange a pulse-level message through the virtual router deterministically in repeated runs, with no direct byte shortcut, no Ear+Mic ROM paging, and no simultaneous Interface 1 state in fidelity mode. | M29.01-M29.03; M20; M21; M28 virtual proof | `tests/fixtures/micear/M29-two-node-virtual.json`; `tests/fixtures/micear/M29-boundary-separation.json`; `tests/fixtures/micear/zx48-micear-stack.tap`; `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/micear/M28-virtual-two-node.json` | `certification/results/micear/M30.G02.json` |
| M30.G03 | 3361 | Phase 2 - Spectrum physical diagnostic | one unmodified Issue 2 machine can transmit and receive the known diagnostic patterns repeatedly without unsafe electrical behavior or unexplained timing divergence. | M06-M09; M27; M30.P031-M30.P044 | `tests/fixtures/micear/zx48-micear-diagnostic.tap`; `certification/hardware/micear/M06-port-wiring-record.json`; `certification/hardware/micear/M27-loopback.csv`; `certification/hardware/micear/M08-fault-isolation.csv`; `certification/gates/micear/D01.md`; `certification/gates/micear/D05.md`; `certification/gates/micear/D33.md`; `certification/gates/micear/D41.md`; `certification/gates/micear/D42.md` | `certification/results/micear/M30.G03.json` |
| M30.G04 | 3393 | Phase 3 - Two-station physical network | two real Issue 2 machines exchange queued messages bidirectionally with correct attention, failure, retry, and durability behavior. | M11-M19; M25; M30.P045-M30.P064 | `tests/fixtures/micear/zx48-micear-basic-immediate.tap`; `tests/fixtures/micear/M15-queue-durability.json`; `tests/fixtures/micear/M17-command-semantics.json`; `tests/fixtures/micear/M18-errors.json`; `tests/fixtures/micear/M28-async-message-vectors.json`; `tests/fixtures/micear/zx48-router-test.sqlite`; `certification/gates/micear/D09.md`; `certification/gates/micear/D17.md`; `certification/gates/micear/D24.md`; `certification/gates/micear/D33.md`; `certification/gates/micear/D43.md`; `certification/gates/micear/D47.md`; `certification/gates/micear/D48.md` | `certification/results/micear/M30.G04.json` |
| M30.G05 | 3414 | Phase 4 - Object transfer | every required object class passes minimum/typical/maximum, corruption, retry, abort, and end-to-end checksum tests on simulator and real hardware. | M15 object staging; M17 object forms; M28 transfer tests; M30.P065-M30.P074 | `tests/fixtures/micear/zx48-micear-object-transfer.tap`; `tests/fixtures/micear/M28-transfer-vectors.json`; `tests/fixtures/micear/M15-object-staging.json`; `certification/gates/micear/D21.md`; `certification/gates/micear/D22.md`; `certification/gates/micear/D43.md` | `certification/results/micear/M30.G05.json` |
| M30.G06 | 3438 | Phase 5 - Multi-port router | multi-port concurrency and isolation tests pass without cross-port corruption, source-identity confusion, or unbounded resource use. | M06; M08; M10; M26; M28 multi-port/fault tests; M30.P075-M30.P086 | `certification/hardware/micear/M30-P5-multiport.json`; `certification/hardware/micear/M06-cross-port-isolation.csv`; `certification/hardware/micear/M08-fault-isolation.csv`; `tests/fixtures/micear/M28-endurance-plan.json`; `tests/fixtures/micear/M28-fault-injection.json`; `tests/fixtures/micear/M10-router-api.json`; `certification/gates/micear/D04.md`; `certification/gates/micear/D28.md`; `certification/gates/micear/D39.md`; `certification/gates/micear/D51.md` | `certification/results/micear/M30.G06.json` |
| M30.G07 | 3466 | Phase 6 - Full Interface 1-style integration | every supported Interface 1-style form passes immediate and stored-program compatibility tests without changing unrelated 48K BASIC error behavior. | M16-M22; M30.P087-M30.P101 | `tests/fixtures/micear/zx48-micear-basic-immediate.tap`; `tests/fixtures/micear/zx48-micear-basic-stored.tap`; `tests/fixtures/micear/M17-command-semantics.json`; `tests/fixtures/micear/M18-errors.json`; `tests/fixtures/micear/M20-hook-safety.json`; `certification/gates/micear/D13.md`; `certification/gates/micear/D14.md`; `certification/gates/micear/D15.md`; `certification/gates/micear/D16.md`; `certification/gates/micear/D25.md`; `certification/gates/micear/D29.md`; `certification/gates/micear/D37.md` | `certification/results/micear/M30.G07.json` |
| M30.G08 | 3492 | Phase 7 - Hardening | all Section-31 acceptance criteria pass on the release hardware, and no Section-32 item remains open beyond a deliberately deferred future feature. | M31.AC01-M31.AC65; M30.P102-M30.P115 | `certification/manifests/micear-section31-acceptance.json`; `certification/manifests/micear-section32-decisions.json`; `certification/results/micear/M32.01.json`; `certification/hardware/micear/M08-fault-isolation.csv`; `certification/hardware/micear/M30-P7-production-pcb.json`; `tests/fixtures/micear/M28-endurance-plan.json`; `tests/fixtures/micear/M28-fault-injection.json`; `certification/manifests/micear-manufacturing-test-records.json` | `certification/results/micear/M30.G08.json` |
## M31. Architecture section 31: Acceptance Criteria

**Architecture authority:** `M` source lines 3497-3635, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Acceptance criteria 1-65.

**Required artifacts:**

- `certification/manifests/micear-section31-acceptance.json`

**Artifact/source authority:** Project acceptance manifest; exact criteria embedded later in this proof.

**Tests:**

### M31.01

Execute one explicit acceptance record for each of the 65 numbered Section31 criteria; each row below names the concrete lower-level input fixture, measurement, gate, or cross-architecture artifact required to produce its result, not merely the result JSON; criteria 1..65 appear exactly once; no criterion PASS if an applicable Section32 gate is unresolved.

**Evidence output:** `certification/results/micear/M31.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### M31 acceptance-item forensic ledger

| Test | Source line | Literal acceptance criterion | Required proof artifact(s) | Evidence artifact |
|---|---:|---|---|---|
| M31.AC01 | 3502 | An unmodified Issue 2 48K Spectrum can load the resident stack from a standard tape image or router-generated tape waveform. | `tests/fixtures/micear/zx48-micear-stack.tap`; `tests/fixtures/micear/M21-bootstrap-flow.json`; `certification/gates/micear/D16.md`; `certification/gates/micear/D37.md` | `certification/results/micear/M31.AC01.json` |
| M31.AC02 | 3504 | After installation, unrelated normal 48K BASIC errors still follow their normal behavior and report path. | `tests/fixtures/micear/zx48-micear-basic-immediate.tap`; `tests/fixtures/micear/M20-hook-safety.json`; `certification/gates/micear/D14.md` | `certification/results/micear/M31.AC02.json` |
| M31.AC03 | 3506 | `FORMAT "n";N` establishes/confirms a valid local identity or returns a defined deterministic error. | `tests/fixtures/micear/M17-command-semantics.json`; `tests/fixtures/micear/M11-address-registration.json`; `certification/gates/micear/D11.md` | `certification/results/micear/M31.AC03.json` |
| M31.AC04 | 3508 | Exactly six-character alphanumeric station names are enforced uniquely; canonical storage is uppercase. | `tests/fixtures/micear/M11-address-registration.json`; `certification/manifests/micear/D12-closed-contract.json` | `certification/results/micear/M31.AC04.json` |
| M31.AC05 | 3510 | Reserved service name `ROUTER` cannot be assigned to an ordinary station. | `tests/fixtures/micear/M11-address-registration.json`; `tests/fixtures/micear/M04-lexical-conventions.json` | `certification/results/micear/M31.AC05.json` |
| M31.AC06 | 3511 | Reserved address `0.0` cannot be assigned to an ordinary station and selects the same local router endpoint as `ROUTER` where defined. | `tests/fixtures/micear/M11-address-registration.json`; `tests/fixtures/micear/M04-lexical-conventions.json`; `certification/gates/micear/D10.md` | `certification/results/micear/M31.AC06.json` |
| M31.AC07 | 3513 | Both ordinary `"x.y"` and `"AAAAAA"` station designators work. | `tests/fixtures/micear/M04-lexical-conventions.json`; `tests/fixtures/micear/M17-command-semantics.json` | `certification/results/micear/M31.AC07.json` |
| M31.AC08 | 3514 | Unknown and known-but-unavailable stations produce different reports. | `tests/fixtures/micear/M18-errors.json`; `tests/fixtures/micear/M12-link-liveness.json`; `certification/gates/micear/D48.md` | `certification/results/micear/M31.AC08.json` |
| M31.AC09 | 3515 | Source identity is derived from the authenticated physical/virtual port, not trusted from Spectrum payload content. | `tests/fixtures/micear/M05-trust-boundary-vectors.json`; `tests/fixtures/micear/M11-address-registration.json` | `certification/results/micear/M31.AC09.json` |
| M31.AC10 | 3517 | `OPEN #`, `PRINT #`, and `CLOSE #` can send a queued character message through the router. | `tests/fixtures/micear/zx48-micear-basic-immediate.tap`; `tests/fixtures/micear/M17-command-semantics.json`; `tests/fixtures/micear/M15-queue-durability.json` | `certification/results/micear/M31.AC10.json` |
| M31.AC11 | 3519 | `CLOSE #` returns `0 OK` only after the complete message and required metadata are persistently committed under Section 15.4. | `tests/fixtures/micear/M15-queue-durability.json`; `tests/fixtures/micear/zx48-router-test.sqlite` | `certification/results/micear/M31.AC11.json` |
| M31.AC12 | 3521 | A successful queued message survives a normal router-daemon restart before recipient retrieval. | `tests/fixtures/micear/M15-queue-durability.json`; `tests/fixtures/micear/zx48-router-test.sqlite`; `certification/gates/micear/D24.md` | `certification/results/micear/M31.AC12.json` |
| M31.AC13 | 3523 | The initial release rejects queue commits to stations that are offline at the authoritative commit check; no grace-period mailbox behavior occurs. | `tests/fixtures/micear/M12-queue-busy.json`; `certification/gates/micear/D48.md` | `certification/results/micear/M31.AC13.json` |
| M31.AC14 | 3525 | Router-endpoint retrieval is stable FIFO by commit sequence, while source-specific retrieval is FIFO within that source subset. | `tests/fixtures/micear/M15-queue-durability.json` | `certification/results/micear/M31.AC14.json` |
| M31.AC15 | 3527 | A message can arrive while the destination Spectrum is not receiving. | `tests/fixtures/micear/M28-async-message-vectors.json`; `tests/fixtures/micear/M15-queue-durability.json` | `certification/results/micear/M31.AC15.json` |
| M31.AC16 | 3528 | The destination detects an attention indication without receiving payload in the interrupt handler. | `tests/fixtures/micear/M19-attention.json`; `tests/fixtures/micear/zx48-micear-stack.tap` | `certification/results/micear/M31.AC16.json` |
| M31.AC17 | 3530 | The attention handler does not inject text, call arbitrary BASIC, or perform full packet reception. | `tests/fixtures/micear/M19-attention.json`; `tests/fixtures/micear/M20-hook-safety.json`; `tests/fixtures/micear/zx48-micear-stack.tap` | `certification/results/micear/M31.AC17.json` |
| M31.AC18 | 3532 | A queued message remains queued after failed or incomplete retrieval. | `tests/fixtures/micear/M15-queue-durability.json`; `tests/fixtures/micear/M19-retrieval-vectors.json` | `certification/results/micear/M31.AC18.json` |
| M31.AC19 | 3533 | A message is marked delivered only after complete validated retrieval and the required close/acknowledgment succeeds. | `tests/fixtures/micear/M19-retrieval-vectors.json`; `tests/fixtures/micear/M15-queue-durability.json`; `certification/gates/micear/D43.md` | `certification/results/micear/M31.AC19.json` |
| M31.AC20 | 3535 | `SAVE *` and `LOAD *` transfer every required Spectrum object class with end-to-end integrity. | `tests/fixtures/micear/zx48-micear-object-transfer.tap`; `tests/fixtures/micear/M28-transfer-vectors.json`; `certification/gates/micear/D22.md`; `certification/gates/micear/D43.md` | `certification/results/micear/M31.AC20.json` |
| M31.AC21 | 3537 | `VERIFY *` reports `Verification has failed` for mismatched data. | `tests/fixtures/micear/zx48-micear-object-transfer.tap`; `tests/fixtures/micear/M17-command-semantics.json`; `tests/fixtures/micear/M18-errors.json` | `certification/results/micear/M31.AC21.json` |
| M31.AC22 | 3538 | `MERGE *` preserves normal Spectrum BASIC merge semantics for the supported compatibility baseline. | `tests/fixtures/micear/zx48-micear-object-transfer.tap`; `tests/fixtures/micear/M17-command-semantics.json` | `certification/results/micear/M31.AC22.json` |
| M31.AC23 | 3540 | No Spectrum MIC outputs are electrically tied together; each station has an independent router RX/TX path. | `certification/hardware/micear/M06-port-wiring-record.json`; `certification/hardware/micear/M06-cross-port-isolation.csv` | `certification/results/micear/M31.AC23.json` |
| M31.AC24 | 3542 | No Raspberry Pi Linux user-space scheduler is relied upon to generate or measure active cassette pulse timing. | `tests/fixtures/micear/M09-controller-stress.json`; `certification/hardware/micear/M09-linux-jitter-stress.csv` | `certification/results/micear/M31.AC24.json` |
| M31.AC25 | 3544 | Every physical router port meets the release protection/isolation contract. | `certification/hardware/micear/M08-fault-isolation.csv`; `certification/gates/micear/D02.md`; `certification/gates/micear/D03.md` | `certification/results/micear/M31.AC25.json` |
| M31.AC26 | 3545 | A WZSN virtual harness exercises the same pulse-level protocol using canonical master-tick timestamps. | `tests/fixtures/micear/M29-two-node-virtual.json`; `tests/fixtures/micear/M29-boundary-separation.json` | `certification/results/micear/M31.AC26.json` |
| M31.AC27 | 3547 | The WZSN fidelity path contains no direct byte shortcut that bypasses port-FE MIC/EAR behavior. | `tests/fixtures/micear/M29-boundary-separation.json` | `certification/results/micear/M31.AC27.json` |
| M31.AC28 | 3549 | Repeated identical virtual tests produce identical event order and results. | `tests/fixtures/micear/M29-two-node-virtual.json`; `certification/gates/micear/D30.md` | `certification/results/micear/M31.AC28.json` |
| M31.AC29 | 3550 | Virtual and physical timing traces for the diagnostic corpus have no unexplained protocol-significant divergence. | `tests/fixtures/micear/zx48-micear-diagnostic.tap`; `certification/hardware/micear/M27-timing-trace.csv`; `certification/hardware/micear/M28-physical-link-captures-manifest.json` | `certification/results/micear/M31.AC29.json` |
| M31.AC30 | 3552 | The visible Spectrum command vocabulary contains no new `NET`, `MSG`, `WHO`, `FILES`, `GET`, `SEND`, or `SHARE` command family. | `tests/fixtures/micear/M01-basic-forms.json`; `tests/fixtures/micear/M16-router-leakage.json` | `certification/results/micear/M31.AC30.json` |
| M31.AC31 | 3554 | Immediate-mode and supported stored-BASIC forms preserve listing, syntax, execution, and line/statement reporting requirements. | `tests/fixtures/micear/zx48-micear-basic-immediate.tap`; `tests/fixtures/micear/zx48-micear-basic-stored.tap`; `certification/gates/micear/D15.md`; `tests/fixtures/micear/M18-errors.json` | `certification/results/micear/M31.AC31.json` |
| M31.AC32 | 3556 | The resident stack is non-reentrant; interrupt attention state cannot race with full EAR receive ownership. | `tests/fixtures/micear/M20-hook-safety.json`; `tests/fixtures/micear/M19-attention.json`; `tests/fixtures/micear/zx48-micear-stack.tap` | `certification/results/micear/M31.AC32.json` |
| M31.AC33 | 3558 | Duplicate packets do not apply payload twice and produce the required repeat acknowledgment behavior. | `tests/fixtures/micear/M25-fault-injection.json`; `certification/gates/micear/D45.md` | `certification/results/micear/M31.AC33.json` |
| M31.AC34 | 3560 | Packet corruption, lost acknowledgments, timeouts, cancellation, and router restart follow the Section-25 recovery contract. | `tests/fixtures/micear/M25-fault-injection.json`; `tests/fixtures/micear/M25-recovery-matrix.json`; `tests/fixtures/micear/M25-timeout-vectors.json`; `certification/gates/micear/D09.md`; `certification/gates/micear/D24.md` | `certification/results/micear/M31.AC34.json` |
| M31.AC35 | 3562 | Per-station queues, open channels, retries, and staging are bounded by frozen release limits. | `tests/fixtures/micear/M25-fault-injection.json`; `certification/gates/micear/D39.md` | `certification/results/micear/M31.AC35.json` |
| M31.AC36 | 3564 | Initial administration is local-only; any network-reachable management surface is authenticated and authorized. | `tests/fixtures/micear/M10-admin-vectors.json`; `certification/gates/micear/D28.md` | `certification/results/micear/M31.AC36.json` |
| M31.AC37 | 3566 | Router logs omit message contents by default while retaining required operational diagnostics. | `certification/results/micear/M27-router-log.jsonl` | `certification/results/micear/M31.AC37.json` |
| M31.AC38 | 3568 | At least four ports operate concurrently in the multi-port reference build without cross-port identity, waveform, queue, or fault contamination. | `certification/hardware/micear/M08-port-count-builds.json`; `certification/hardware/micear/M30-P5-multiport.json`; `certification/gates/micear/D04.md` | `certification/results/micear/M31.AC38.json` |
| M31.AC39 | 3570 | Tape-distributed diagnostics can exercise MIC output, EAR input, attention, loopback, a small stream, and a small CODE transfer. | `tests/fixtures/micear/zx48-micear-diagnostic.tap`; `certification/results/micear/M27-diagnostic-transcript.txt`; `certification/hardware/micear/M27-loopback.csv` | `certification/results/micear/M31.AC39.json` |
| M31.AC40 | 3572 | `FORMAT` can execute from local `NOT_SET` state; ordinary `Station not set` precedence never blocks registration. | `tests/fixtures/micear/M17-command-semantics.json`; `tests/fixtures/micear/M18-precedence.json` | `certification/results/micear/M31.AC40.json` |
| M31.AC41 | 3574 | `INKEY$ #` returns immediately and does not generate a timeout solely because no character is available. | `tests/fixtures/micear/M17-command-semantics.json` | `certification/results/micear/M31.AC41.json` |
| M31.AC42 | 3576 | Queued-message transport retries are idempotent and do not create duplicate committed records from a repeated commit transaction. | `tests/fixtures/micear/M25-fault-injection.json`; `certification/gates/micear/D46.md` | `certification/results/micear/M31.AC42.json` |
| M31.AC43 | 3578 | The protocol documentation makes no exactly-once guarantee after an unrecoverable acknowledgment timeout; retained/redelivered messages follow the documented at-least-once recovery rule. | `tests/fixtures/micear/M25-recovery-matrix.json`; `certification/gates/micear/D09.md`; `certification/gates/micear/D46.md` | `certification/results/micear/M31.AC43.json` |
| M31.AC44 | 3581 | Router-generated bootstrap tape, attention signalling, and routed packet delivery are mutually exclusive port modes and never interleave waveforms. | `tests/fixtures/micear/M24-port-mode.json`; `tests/fixtures/micear/M21-bootstrap-flow.json`; `certification/gates/micear/D17.md` | `certification/results/micear/M31.AC44.json` |
| M31.AC45 | 3583 | Loss/reset of the Pi/controller link forces the physical downlink driver to the frozen safe-idle state. | `tests/fixtures/micear/M09-controller-fault-injection.json`; `tests/fixtures/micear/M25-recovery-matrix.json` | `certification/results/micear/M31.AC45.json` |
| M31.AC46 | 3585 | An installed resident stack rejects network receives that would overwrite its reserved RAM/hook state. | `tests/fixtures/micear/zx48-micear-protected-memory.tap`; `tests/fixtures/micear/M20-hook-safety.json` | `certification/results/micear/M31.AC46.json` |
| M31.AC47 | 3587 | Network notification/API availability is not claimed for arbitrary software that destroys the resident RAM or interrupt contract; queued router data remains authoritative and can be reconciled after restoration. | `tests/fixtures/micear/M19-pending-reconciliation.json`; `tests/fixtures/micear/zx48-micear-stack.tap` | `certification/results/micear/M31.AC47.json` |
| M31.AC48 | 3590 | One physical/link frame carries one routed packet in the initial protocol and uses the single frozen packet CRC definition; no undocumented second link CRC exists. | `tests/fixtures/micear/M14-packet-vectors.json`; `certification/gates/micear/D07.md`; `certification/gates/micear/D36.md` | `certification/results/micear/M31.AC48.json` |
| M31.AC49 | 3593 | Queued messages and staged objects bind to immutable internal station identities; later display-name/address changes cannot silently retarget them. | `tests/fixtures/micear/M15-routing-vectors.json`; `tests/fixtures/micear/M15-queue-durability.json`; `certification/gates/micear/D47.md` | `certification/results/micear/M31.AC49.json` |
| M31.AC50 | 3595 | A destination that disconnects after a successful commit retains that already committed message for its later return, while new commits to an offline destination remain rejected. | `tests/fixtures/micear/M15-queue-durability.json`; `tests/fixtures/micear/M12-link-liveness.json`; `certification/gates/micear/D48.md` | `certification/results/micear/M31.AC50.json` |
| M31.AC51 | 3598 | Initial committed messages do not silently expire by wall-clock age; bounded queue limits govern acceptance of new data. | `tests/fixtures/micear/M15-queue-durability.json`; `certification/gates/micear/D39.md` | `certification/results/micear/M31.AC51.json` |
| M31.AC52 | 3600 | Stream/object control has a frozen logical transaction identifier and packet sequence-wrap/duplicate-window rule sufficient for idempotent retries. | `tests/fixtures/micear/M14-packet-vectors.json`; `certification/gates/micear/D45.md`; `certification/gates/micear/D46.md` | `certification/results/micear/M31.AC52.json` |
| M31.AC53 | 3602 | The virtual router uses the same router protocol/core behavior as the physical system and an injected deterministic time source. | `tests/fixtures/micear/M29-two-node-virtual.json`; `tests/fixtures/micear/M29-boundary-separation.json`; `certification/gates/micear/D30.md` | `certification/results/micear/M31.AC53.json` |
| M31.AC54 | 3604 | WZSN emulation-speed changes do not change virtual Spectrum-time protocol timing or event ordering. | `tests/fixtures/micear/M29-speed-invariance.json` | `certification/results/micear/M31.AC54.json` |
| M31.AC55 | 3606 | Router liveness rules distinguish an idle online station, a busy station, and an unavailable/disconnected station deterministically under the frozen Phase-3 policy. | `tests/fixtures/micear/M12-link-liveness.json`; `certification/gates/micear/D48.md` | `certification/results/micear/M31.AC55.json` |
| M31.AC56 | 3609 | `station_id_internal` is immutable/non-reused, so deleting and recreating a visible name/address cannot inherit old queued data accidentally. | `tests/fixtures/micear/M23-data-model.json`; `tests/fixtures/micear/M23-schema-introspection.json`; `certification/gates/micear/D38.md` | `certification/results/micear/M31.AC56.json` |
| M31.AC57 | 3611 | Phase-3 integrated BASIC commands use a frozen immediate-mode hook and memory reservation; Phase 6 does not retroactively invent those foundations. | `certification/gates/micear/D14.md`; `certification/gates/micear/D16.md`; `build/spectrum/zx48-micear-stack.map` | `certification/results/micear/M31.AC57.json` |
| M31.AC58 | 3613 | The selected Spectrum/router/controller build and deployment toolchains are recorded before their dependent implementation/release tickets. | `certification/gates/micear/D49.md`; `certification/gates/micear/D50.md`; `certification/gates/micear/D51.md`; `certification/manifests/micear-controller-build.json` | `certification/results/micear/M31.AC58.json` |
| M31.AC59 | 3615 | All Phase-0 through Phase-7 exit gates applicable to the release are closed before that release is declared architecture-complete. | `certification/manifests/micear-phase-gates.json` | `certification/results/micear/M31.AC59.json` |
| M31.AC60 | 3617 | Physical Architecture-#3 acceptance is performed with Interface 1 absent or inactive while the Ear+Mic resident BASIC front end is installed. | `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/micear/zx48-micear-stack.tap`; `certification/hardware/micear/M28-physical-link-matrix.csv` | `certification/results/micear/M31.AC60.json` |
| M31.AC61 | 3619 | WZSN Architecture-#3 fidelity tests run only in `EAR_MIC` networking mode, which structurally excludes Interface 1, Microdrive, and original ZX Net. | `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/micear/M29-two-node-virtual.json` | `certification/results/micear/M31.AC61.json` |
| M31.AC62 | 3621 | `EAR_MIC` mode does not page a networking ROM and does not auto-install or patch the resident Spectrum stack; fidelity tests load the distributed stack through the documented tape/bootstrap path. | `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/micear/zx48-micear-stack.tap`; `tests/fixtures/micear/M29-boundary-separation.json` | `certification/results/micear/M31.AC62.json` |
| M31.AC63 | 3624 | WZSN networking-mode changes use the upstream cold machine-reconfiguration transition, so old resident RAM hooks/device state cannot survive the mode change and simultaneous `INTERFACE1`/`EAR_MIC` state cannot occur. | `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/networking/wzsn-networking-transition-edge-v1.json`; `tests/fixtures/mdr/wzsn-mdr-dirty-transition-v1.json`; `certification/results/core/C24.01.json`; `certification/results/core/C24.02.json`; `certification/results/core/C24.04.json` | `certification/results/micear/M31.AC63.json` |
| M31.AC64 | 3627 | In WZSN `EAR_MIC` mode the routed virtual port exclusively owns cassette EAR/MIC semantics; ordinary tape transport cannot drive/consume the same signals concurrently, and stack installation uses `BOOTSTRAP_TAPE`. | `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/micear/M21-bootstrap-flow.json`; `tests/fixtures/micear/zx48-micear-stack.tap` | `certification/results/micear/M31.AC64.json` |
| M31.AC65 | 3630 | WZSN Ear+Mic fidelity uses an explicitly certified ZX Spectrum 48K Issue-2 profile/variant; 128K and uncertified 48K variants are rejected without machine mutation, and distinct Telnet Control Ports do not imply an unsynchronized cross-process virtual network. | `certification/gates/micear/P0-wzsn-48k-issue2-profile.md`; `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/micear/M29-two-node-virtual.json`; `tests/fixtures/telnet/wzsn-control-port-race-v1.json` | `certification/results/micear/M31.AC65.json` |
## M32. Architecture section 32: Deferred Engineering Decisions and Gates

**Architecture authority:** `M` source lines 3636-3699, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Deferred decisions D01-D51.

**Required artifacts:**

- `certification/manifests/micear-section32-decisions.json`
- `certification/manifests/micear/D12-closed-contract.json`
- `certification/manifests/micear/D19-closed-contract.json`
- `certification/manifests/micear/D20-closed-contract.json`
- `certification/manifests/micear/D26-closed-contract.json`
- `certification/manifests/micear/D27-closed-contract.json`

**Artifact/source authority:** Project gate manifest plus architecture-derived closed-contract manifests for D12/D19/D20/D26/D27. Each closed-contract manifest reproduces the exact frozen Architecture-#3 decision text/source line and architecture SHA-256; it indexes architecture authority rather than pretending to be an independently produced behavioral result. Exact D table embedded later in this proof.

**Tests:**

### M32.01

For each D01..D51 require exact gate status/evidence filename and required phase. Closed D12/D19/D26/D27 are asserted directly against the frozen architecture text. D20 defaults to the frozen initial-release exclusion, while `P0-broadcast-policy.md` must explicitly record either EXCLUDED (referencing the closed D20 contract) or, only if Phase 0 elects inclusion, complete broadcast semantics and all dependent proof updates. Every other decision is BLOCKED until its named gate artifact exists and passes its own review/measurement tests; no provisional Appendix-E/F value can substitute.

**Evidence output:** `certification/results/micear/M32.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### M32 exact deferred-decision/gate ledger

| Decision | Source line | Decision | Required gate | Exact gate/result artifact | Certification state rule |
|---|---:|---|---|---|---|
| D01 | 3644 | Final analog input/output schematics for supervised prototype | Phase 2 entry | `certification/gates/micear/D01.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D02 | 3645 | Final production isolation strategy and ratings | Phase 7 entry | `certification/gates/micear/D02.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D03 | 3646 | Final connector and cable specification | Phase 7 entry | `certification/gates/micear/D03.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D04 | 3647 | Production/reference PCB port count beyond Phase-5 minimum four | Phase 5/7 entry as applicable | `certification/gates/micear/D04.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D05 | 3648 | Conservative pulse timings and any later turbo profile | prototype profile: Phase 2 entry; initial release profile: Phase 3 entry; turbo: Phase 7 or later | `certification/gates/micear/D05.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D06 | 3649 | Final physical/link frame preamble and sync | Phase 0 exit | `certification/gates/micear/D06.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D07 | 3650 | CRC polynomial, initialization, reflection, and representation | Phase 0 exit | `certification/gates/micear/D07.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D08 | 3651 | Packet payload size | virtual-proof candidate: Phase 0 exit; initial physical release value: Phase 3 entry | `certification/gates/micear/D08.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D09 | 3652 | Retry counts and operation-specific timeout values | before each dependent phase; initial set at Phase 0 exit | `certification/gates/micear/D09.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D10 | 3653 | Remaining reserved/broadcast address ranges beyond frozen `0.0` | Phase 0 exit | `certification/gates/micear/D10.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D11 | 3654 | Production registration/reassignment policy beyond preconfigured port binding | Phase 0 exit; pending-data reassignment safety also requires D47 | `certification/gates/micear/D11.md`; `certification/gates/micear/D47.md` | BLOCKED_GATE until both applicable artifacts are reviewed/frozen |
| D12 | 3655 | Lowercase station-name handling | **closed:** normalize to uppercase before validation | `certification/manifests/micear/D12-closed-contract.json` | FROZEN_ARCHITECTURE - verify closed behavior |
| D13 | 3656 | Exact Interface 1 compatibility baseline for `MOVE` | Phase 6 entry | `certification/gates/micear/D13.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D14 | 3657 | BASIC ROM error-hook implementation/recovery | minimum immediate-mode contract: Phase 2/3 entry; full stored-program compatibility: Phase 6 entry | `certification/gates/micear/D14.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D15 | 3658 | Complete stored-BASIC tokenizer/list/execution strategy | Phase 6 entry | `certification/gates/micear/D15.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D16 | 3659 | High-memory load address and RAM footprint | diagnostic provisional range: Phase 2 entry; integrated Phase-3 map: Phase 3 entry; release-final confirmation/map: Phase 6 entry | `certification/gates/micear/D16.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D17 | 3660 | Exact attention waveform and false-positive threshold | Phase 3 entry | `certification/gates/micear/D17.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D18 | 3661 | Pending-message presentation: border, sound, or combination | Phase 6 entry | `certification/gates/micear/D18.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D19 | 3662 | Offline queue grace-period semantics | **closed for initial release:** destination must be online at commit check; no grace period | `certification/manifests/micear/D19-closed-contract.json` | FROZEN_ARCHITECTURE - verify closed behavior |
| D20 | 3663 | Broadcast in initial release | **closed:** excluded unless Phase 0 adds complete explicit semantics | `certification/manifests/micear/D20-closed-contract.json`; `certification/gates/micear/P0-broadcast-policy.md` | FROZEN initial exclusion; if Phase-0 policy elects inclusion, BLOCKED_GATE until complete explicit semantics and dependent tests are frozen |
| D21 | 3664 | Object-transfer staging strategy | Phase 4 entry | `certification/gates/micear/D21.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D22 | 3665 | Maximum message and object sizes | message: Phase 3 entry; object: Phase 4 entry | `certification/gates/micear/D22.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D23 | 3666 | Pi/controller physical transport selection | Phase 2 entry | `certification/gates/micear/D23.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D24 | 3667 | Router restart/database recovery and sudden-power-loss policy | daemon restart: Phase 3 entry; sudden power loss: Phase 7 entry | `certification/gates/micear/D24.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D25 | 3668 | Final report letters/numeric identifiers | Phase 6 entry | `certification/gates/micear/D25.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D26 | 3669 | Uplink source-field policy | **closed:** source omitted on initial uplink; router derives it from port binding | `certification/manifests/micear/D26-closed-contract.json` | FROZEN_ARCHITECTURE - verify closed behavior |
| D27 | 3670 | Bit order within a byte | **closed:** most-significant bit first | `certification/manifests/micear/D27-closed-contract.json` | FROZEN_ARCHITECTURE - verify closed behavior |
| D28 | 3671 | Initial administrative mechanism and network reachability | Phase 5 entry; network-reachable administration requires authentication | `certification/gates/micear/D28.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D29 | 3672 | Spectrum-stack Z80 calling convention and buffer ownership | Phase 2 entry | `certification/gates/micear/D29.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D30 | 3673 | Virtual multi-machine event-order/coordinator contract | Phase 1 entry | `certification/gates/micear/D30.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D31 | 3674 | Repository/source-tree placement for router daemon, timing firmware, Spectrum stack, and hardware design files | Phase 0 exit | `certification/gates/micear/D31.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D32 | 3675 | Controller firmware-update security/recovery mechanism | Phase 7 entry | `certification/gates/micear/D32.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D33 | 3676 | Exact request/response turnaround guard | Phase 3 entry | `certification/gates/micear/D33.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D34 | 3677 | Baseline exported Spectrum-stack API names/semantics | Phase 2 entry | `certification/gates/micear/D34.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D35 | 3678 | Router daemon/module decomposition beyond frozen logical responsibilities | Phase 0 exit | `certification/gates/micear/D35.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D36 | 3679 | Routed-packet header field layout and packet-type numeric assignments | Phase 0 exit | `certification/gates/micear/D36.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D37 | 3680 | Binary image-header/tape packaging format | Phase 6 entry | `certification/gates/micear/D37.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D38 | 3681 | Concrete persistent schema/storage mapping for baseline router data model | Phase 1 for registry; Phase 3/4 for messages/objects | `certification/gates/micear/D38.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D39 | 3682 | Per-station queue, staging, open-channel, packet-rate, and retry bounds | Phase 5 entry; smaller test bounds frozen earlier as needed | `certification/gates/micear/D39.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D40 | 3683 | Stream channel direction and mixed `PRINT #`/`INPUT #`/`INKEY$ #` semantics | Phase 3 entry for message subset; Phase 6 final compatibility | `certification/gates/micear/D40.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D41 | 3684 | Deterministic timing-controller technology for prototype | Phase 2 entry | `certification/gates/micear/D41.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D42 | 3685 | Pi/controller digital framing, integrity, flow control, and reconnect protocol | Phase 2 entry | `certification/gates/micear/D42.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D43 | 3686 | End-to-end message and object integrity algorithms | message: Phase 3 entry; object: Phase 4 entry | `certification/gates/micear/D43.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D44 | 3687 | Controller buffering/flow-control capacity at each supported physical profile | Phase 2 prototype; Phase 7 release profile | `certification/gates/micear/D44.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D45 | 3688 | Packet sequence wrap, duplicate-recognition window, and recovery lifetime | Phase 0 exit | `certification/gates/micear/D45.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D46 | 3689 | Stream/message transaction-ID representation and idempotent commit key | Phase 0 exit; object transfer ID finalized at Phase 4 entry | `certification/gates/micear/D46.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D47 | 3690 | Administrative station replacement/rebinding treatment of pending messages/objects | Phase 3 entry | `certification/gates/micear/D47.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D48 | 3691 | Station online/unavailable liveness, cable/activity evidence, and disconnect timeout policy | Phase 3 entry | `certification/gates/micear/D48.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D49 | 3692 | Spectrum Z80 assembler/build/tape-packaging toolchain for physical stack development | Phase 2 entry | `certification/gates/micear/D49.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D50 | 3693 | Router daemon and virtual-proof implementation language/build system | Phase 0 exit | `certification/gates/micear/D50.md` | BLOCKED_GATE until artifact reviewed/frozen |
| D51 | 3694 | Raspberry Pi deployment OS/service baseline and release packaging | Phase 5 entry | `certification/gates/micear/D51.md` | BLOCKED_GATE until artifact reviewed/frozen |

## M33. Architecture section 33: Requirements Traceability Matrix

**Architecture authority:** `M` source lines 3700-3769, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Requirements traceability R01-R64.

**Required artifacts:**

- `certification/manifests/micear-section33-rtm.json`

**Artifact/source authority:** Project RTM generated/reviewed against Section33. Each row below names both the primary lower-level proof test(s) and the concrete fixture/gate/measurement needed to execute those tests; the row result JSON is evidence, not a substitute for its proof input.

**Tests:**

### M33.01

For each R01..R64 map the architecture requirement and cited source sections to at least one already-audited lower-level test ID, its concrete required proof artifact(s), and a result evidence artifact. Fail certification for an unmapped requirement, a lower-level test without architecture authority, a missing/ambiguous input artifact, or an unresolved gate represented as PASS.

**Evidence output:** `certification/results/micear/M33.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

### M33 R01-R64 forensic traceability ledger

| Test | Source line | Requirement | Architecture sections | Primary proof test(s) | Required proof artifact(s) | Evidence artifact |
|---|---:|---|---|---|---|---|
| M33.R01 | 3704 | Issue 2 48K physical target only | 2, 3, 6 | M02.01; M06.01 | `tests/fixtures/micear/M02-scope-included.json`; `certification/hardware/micear/M06-port-wiring-record.json` | `certification/results/micear/M33.R01.json` |
| M33.R02 | 3705 | MIC uplink and EAR downlink | 1, 6, 12 | M06.01; M07.01 | `certification/hardware/micear/M06-port-wiring-record.json`; `certification/hardware/micear/M07-portfe-waveform-correlation.csv` | `certification/results/micear/M33.R02.json` |
| M33.R03 | 3706 | Signal and return on each mono socket | 1, 6 | M06.01 | `certification/hardware/micear/M06-port-wiring-record.json` | `certification/results/micear/M33.R03.json` |
| M33.R04 | 3707 | Central active router | 1, 5, 10 | M01.01; M05.01 | `tests/fixtures/micear/M01-system-topology.json`; `tests/fixtures/micear/M05-component-boundaries.json` | `certification/results/micear/M33.R04.json` |
| M33.R05 | 3708 | Raspberry Pi-class logical router | 1, 9, 10 | M10.01; M10.02 | `tests/fixtures/micear/M10-module-boundaries.json`; `tests/fixtures/micear/M10-router-api.json` | `certification/results/micear/M33.R05.json` |
| M33.R06 | 3709 | Deterministic pulse timing outside Linux user-space | 8, 9, 31 | M09.01; M09.02 | `certification/gates/micear/D41.md`; `tests/fixtures/micear/M09-controller-stress.json`; `certification/hardware/micear/M09-linux-jitter-stress.csv` | `certification/results/micear/M33.R06.json` |
| M33.R07 | 3710 | Independent active-star ports for many Spectrums | 6, 8 | M06.01; M08.04 | `certification/hardware/micear/M06-cross-port-isolation.csv`; `certification/hardware/micear/M08-port-count-builds.json` | `certification/results/micear/M33.R07.json` |
| M33.R08 | 3711 | No tied Spectrum MIC outputs | 2, 6, 31 | M06.01 | `certification/hardware/micear/M06-cross-port-isolation.csv` | `certification/results/micear/M33.R08.json` |
| M33.R09 | 3712 | Six-character unique alphanumeric station names | 4, 11, 31 | M11.01 | `tests/fixtures/micear/M11-address-registration.json`; `certification/manifests/micear/D12-closed-contract.json` | `certification/results/micear/M33.R09.json` |
| M33.R10 | 3713 | Two-byte area-plus-node addresses | 4, 11 | M11.01 | `tests/fixtures/micear/M11-address-registration.json` | `certification/results/micear/M33.R10.json` |
| M33.R11 | 3714 | Ordinary station designator is `"x.y"` or `"AAAAAA"` | 4, 11, 16 | M04.01; M11.01 | `tests/fixtures/micear/M04-lexical-conventions.json`; `tests/fixtures/micear/M11-address-registration.json` | `certification/results/micear/M33.R11.json` |
| M33.R12 | 3715 | Reserved router service is `ROUTER` / `0.0` | 11, 17, 19, Appendix A | M11.02; M17.01 | `tests/fixtures/micear/M11-address-registration.json`; `tests/fixtures/micear/M17-command-semantics.json`; `certification/gates/micear/D10.md` | `certification/results/micear/M33.R12.json` |
| M33.R13 | 3716 | Router maintains name/address mapping | 10, 11 | M11.01 | `tests/fixtures/micear/M11-address-registration.json` | `certification/results/micear/M33.R13.json` |
| M33.R14 | 3717 | Router authenticates source by physical/virtual port | 5, 11, 26 | M11.03 | `tests/fixtures/micear/M05-trust-boundary-vectors.json`; `tests/fixtures/micear/M11-address-registration.json` | `certification/results/micear/M33.R14.json` |
| M33.R15 | 3718 | Interface 1-style front end only | 16, 17, 31 | M16.01; M16.03 | `tests/fixtures/micear/zx48-micear-basic-immediate.tap`; `tests/fixtures/micear/M16-router-leakage.json` | `certification/results/micear/M33.R15.json` |
| M33.R16 | 3719 | FORMAT | 17 | M17.01 | `tests/fixtures/micear/M17-command-semantics.json`; `tests/fixtures/micear/zx48-micear-basic-immediate.tap` | `certification/results/micear/M33.R16.json` |
| M33.R17 | 3720 | SAVE *, LOAD *, VERIFY *, MERGE * | 17 | M17.01 | `tests/fixtures/micear/M17-command-semantics.json`; `tests/fixtures/micear/zx48-micear-object-transfer.tap` | `certification/results/micear/M33.R17.json` |
| M33.R18 | 3721 | OPEN #, PRINT #, INPUT #, INKEY$ #, CLOSE # | 17 | M17.02 | `tests/fixtures/micear/M17-stream-visibility.json`; `certification/gates/micear/D40.md` | `certification/results/micear/M33.R18.json` |
| M33.R19 | 3722 | MOVE | 17, 32 | M17.01 | `tests/fixtures/micear/M17-command-semantics.json`; `certification/gates/micear/D13.md` | `certification/results/micear/M33.R19.json` |
| M33.R20 | 3723 | Normal `0 OK` feedback | 18 | M18.01 | `tests/fixtures/micear/zx48-micear-basic-immediate.tap`; `tests/fixtures/micear/M18-errors.json` | `certification/results/micear/M33.R20.json` |
| M33.R21 | 3724 | Interface 1-like network errors | 18, Appendix C | M18.02; M37.01 | `tests/fixtures/micear/M18-errors.json`; `tests/fixtures/micear/M37-error-catalog.json`; `certification/gates/micear/D25.md` | `certification/results/micear/M33.R21.json` |
| M33.R22 | 3725 | Unknown versus unavailable distinction | 18, 31 | M18.02 | `tests/fixtures/micear/M18-errors.json`; `tests/fixtures/micear/M12-link-liveness.json` | `certification/results/micear/M33.R22.json` |
| M33.R23 | 3726 | High-memory resident stack | 20 | M20.01; M20.02 | `tests/fixtures/micear/zx48-micear-stack.tap`; `tests/fixtures/micear/M20-component-map.json`; `certification/gates/micear/D16.md` | `certification/results/micear/M33.R23.json` |
| M33.R24 | 3727 | Tape distribution | 21 | M21.01; M21.02 | `tests/fixtures/micear/zx48-micear-stack.tap`; `tests/fixtures/micear/M21-tape-block-map.json` | `certification/results/micear/M33.R24.json` |
| M33.R25 | 3728 | BASIC error-path hook preserves unrelated errors | 20, 31 | M20.03; M20.04 | `tests/fixtures/micear/M20-hook-safety.json`; `tests/fixtures/micear/zx48-micear-basic-immediate.tap`; `certification/gates/micear/D14.md` | `certification/results/micear/M33.R25.json` |
| M33.R26 | 3729 | No payload reception in interrupt handler | 19, 20, 31 | M19.02 | `tests/fixtures/micear/zx48-micear-stack.tap`; `tests/fixtures/micear/M19-attention.json` | `certification/results/micear/M33.R26.json` |
| M33.R27 | 3730 | Asynchronous router queueing | 15, 19 | M15.02; M19.01 | `tests/fixtures/micear/M15-queue-durability.json`; `tests/fixtures/micear/M28-async-message-vectors.json` | `certification/results/micear/M33.R27.json` |
| M33.R28 | 3731 | Slow EAR attention indication | 19 | M19.03 | `tests/fixtures/micear/M19-attention.json`; `certification/hardware/micear/M19-attention-capture.csv`; `certification/gates/micear/D17.md` | `certification/results/micear/M33.R28.json` |
| M33.R29 | 3732 | Explicit cooperative message retrieval | 19 | M19.05 | `tests/fixtures/micear/M19-retrieval-vectors.json` | `certification/results/micear/M33.R29.json` |
| M33.R30 | 3733 | Message retained until validated acknowledgment | 15, 19, 25 | M19.06; M25.02 | `tests/fixtures/micear/M19-retrieval-vectors.json`; `tests/fixtures/micear/M25-recovery-matrix.json`; `certification/gates/micear/D43.md` | `certification/results/micear/M33.R30.json` |
| M33.R31 | 3734 | Stable FIFO queue selection | 15, 31 | M15.03 | `tests/fixtures/micear/M15-queue-durability.json` | `certification/results/micear/M33.R31.json` |
| M33.R32 | 3735 | Durable queue commit before `0 OK` | 15, 25, 31 | M15.02 | `tests/fixtures/micear/M15-queue-durability.json`; `tests/fixtures/micear/zx48-router-test.sqlite` | `certification/results/micear/M33.R32.json` |
| M33.R33 | 3736 | Offline destination rejected in initial release | 12, 15, 31 | M12.03; M15.03 | `tests/fixtures/micear/M12-queue-busy.json`; `certification/gates/micear/D48.md` | `certification/results/micear/M33.R33.json` |
| M33.R34 | 3737 | Separate core API and BASIC front end | 5, 22 | M05.03; M22.01 | `tests/fixtures/micear/M05-component-boundaries.json`; `tests/fixtures/micear/M22-stack-api.json`; `certification/gates/micear/D34.md` | `certification/results/micear/M33.R34.json` |
| M33.R35 | 3738 | Pulse-level WZSN integration uses canonical master ticks | 29, 31 | M29.01 | `tests/fixtures/micear/M29-two-node-virtual.json`; `certification/gates/micear/D30.md` | `certification/results/micear/M33.R35.json` |
| M33.R36 | 3739 | No direct SendByte shortcut in fidelity mode | 29, 31 | M29.03 | `tests/fixtures/micear/M29-boundary-separation.json` | `certification/results/micear/M33.R36.json` |
| M33.R37 | 3740 | No modern NET/MSG/WHO/GET vocabulary | 16, 31 | M16.03 | `tests/fixtures/micear/M16-router-leakage.json`; `tests/fixtures/micear/M02-scope-excluded.json` | `certification/results/micear/M33.R37.json` |
| M33.R38 | 3741 | Local-only initial administration; authenticated remote management | 10, 26, 31 | M10.03; M26.04 | `tests/fixtures/micear/M10-admin-vectors.json`; `tests/fixtures/micear/M26-security.json`; `certification/gates/micear/D28.md` | `certification/results/micear/M33.R38.json` |
| M33.R39 | 3742 | Bounded resource use | 25, 26, 30, 31 | M26.03 | `tests/fixtures/micear/M26-security.json`; `certification/gates/micear/D39.md` | `certification/results/micear/M33.R39.json` |
| M33.R40 | 3743 | Explicit deferred-decision gates | 30, 32 | M30.01; M32.01 | `certification/manifests/micear-phase-gates.json`; `certification/manifests/micear-section32-decisions.json` | `certification/results/micear/M33.R40.json` |
| M33.R41 | 3744 | Architecture #3 starts only after WZSN core/UI prerequisite completion | preamble, 30 | M30.01 | `certification/manifests/micear-phase-gates.json` | `certification/results/micear/M33.R41.json` |
| M33.R42 | 3745 | Detailed testing and acceptance criteria | 28, 31 | M28.01; M31.01 | `certification/manifests/micear-section28-tests.json`; `certification/manifests/micear-section31-acceptance.json` | `certification/results/micear/M33.R42.json` |
| M33.R43 | 3746 | FORMAT registration exempt from `Station not set` precondition | 17, 18, 24, 31 | M11.04; M18.03 | `tests/fixtures/micear/M11-address-registration.json`; `tests/fixtures/micear/M18-precedence.json` | `certification/results/micear/M33.R43.json` |
| M33.R44 | 3747 | INKEY$ is genuinely nonblocking | 17, 31 | M17.02 | `tests/fixtures/micear/M17-stream-visibility.json`; `certification/gates/micear/D40.md` | `certification/results/micear/M33.R44.json` |
| M33.R45 | 3748 | Bootstrap tape, attention, and packet transmission are exclusive port modes | 9, 19, 21, 24, 31 | M09.03; M24.01 | `tests/fixtures/micear/M09-attention-mode-vectors.json`; `tests/fixtures/micear/M24-port-mode.json`; `tests/fixtures/micear/M21-bootstrap-flow.json` | `certification/results/micear/M33.R45.json` |
| M33.R46 | 3749 | Resident RAM/hook protection and cooperative-software limitation | 20, 31 | M20.06 | `tests/fixtures/micear/zx48-micear-protected-memory.tap`; `tests/fixtures/micear/M20-hook-safety.json` | `certification/results/micear/M33.R46.json` |
| M33.R47 | 3750 | Idempotent retry plus documented at-least-once failure semantics | 15, 24, 25, 31 | M25.02; M25.04 | `tests/fixtures/micear/M25-recovery-matrix.json`; `tests/fixtures/micear/M25-timeout-vectors.json`; `certification/gates/micear/D46.md` | `certification/results/micear/M33.R47.json` |
| M33.R48 | 3751 | One link frame carries one routed packet and one packet CRC definition | 14, Appendix B, 31 | M14.02; M36.01 | `tests/fixtures/micear/M14-packet-vectors.json`; `tests/fixtures/micear/M36-packet-wire-vectors.bin`; `certification/gates/micear/D07.md` | `certification/results/micear/M33.R48.json` |
| M33.R49 | 3752 | Controller host-loss/reset forces safe idle | 9, 24, 25, 31 | M09.04; M25.03 | `tests/fixtures/micear/M09-controller-fault-injection.json`; `tests/fixtures/micear/M25-recovery-matrix.json` | `certification/results/micear/M33.R49.json` |
| M33.R50 | 3753 | Queued/staged data binds to immutable internal station identity | 15, 23, 31 | M15.04 | `tests/fixtures/micear/M15-queue-durability.json`; `certification/gates/micear/D11.md`; `certification/gates/micear/D47.md` | `certification/results/micear/M33.R50.json` |
| M33.R51 | 3754 | Online-at-commit rule plus retention after later disconnect | 12, 15, 31 | M15.03; M12.04 | `tests/fixtures/micear/M15-queue-durability.json`; `tests/fixtures/micear/M12-link-liveness.json`; `certification/gates/micear/D48.md` | `certification/results/micear/M33.R51.json` |
| M33.R52 | 3755 | No automatic wall-clock expiry of initial committed messages | 15, 31 | M15.03 | `tests/fixtures/micear/M15-queue-durability.json`; `certification/gates/micear/D39.md` | `certification/results/micear/M33.R52.json` |
| M33.R53 | 3756 | Logical transaction IDs plus packet sequence/duplicate window | 14, 25, 30, 32 | M14.04; M25.02 | `tests/fixtures/micear/M14-packet-vectors.json`; `tests/fixtures/micear/M25-recovery-matrix.json`; `certification/gates/micear/D45.md`; `certification/gates/micear/D46.md` | `certification/results/micear/M33.R53.json` |
| M33.R54 | 3757 | Virtual router reuses router protocol/core with injected deterministic time | 29, 31 | M29.01 | `tests/fixtures/micear/M29-two-node-virtual.json`; `certification/gates/micear/D30.md` | `certification/results/micear/M33.R54.json` |
| M33.R55 | 3758 | WZSN speed changes host pacing, not virtual protocol timing | 29, 31 | M29.02 | `tests/fixtures/micear/M29-speed-invariance.json` | `certification/results/micear/M33.R55.json` |
| M33.R56 | 3759 | Deterministic station liveness/unavailable policy | 12, 24, 30, 32, 31 | M12.04; M24.02 | `tests/fixtures/micear/M12-link-liveness.json`; `tests/fixtures/micear/M24-state-machines.json`; `certification/gates/micear/D48.md` | `certification/results/micear/M33.R56.json` |
| M33.R57 | 3760 | Internal station IDs are immutable and non-reused | 15, 23, 31 | M23.01; M15.04 | `tests/fixtures/micear/M23-data-model.json`; `tests/fixtures/micear/M23-schema-introspection.json`; `certification/gates/micear/D38.md` | `certification/results/micear/M33.R57.json` |
| M33.R58 | 3761 | Immediate-mode hook/memory contracts freeze before Phase-3 BASIC network use | 20, 30, 32 | M20.03; M20.05 | `tests/fixtures/micear/M20-hook-safety.json`; `tests/fixtures/micear/zx48-micear-basic-stored.tap`; `certification/gates/micear/D14.md`; `certification/gates/micear/D15.md`; `certification/gates/micear/D16.md` | `certification/results/micear/M33.R58.json` |
| M33.R59 | 3762 | Implementation/deployment toolchains are explicit pre-ticket gates | 30, 32 | M30.01 | `certification/manifests/micear-controller-build.json`; `certification/gates/micear/D49.md`; `certification/gates/micear/D50.md`; `certification/gates/micear/D51.md` | `certification/results/micear/M33.R59.json` |
| M33.R60 | 3763 | Ear+Mic resident BASIC front end and active Interface 1 are mutually exclusive | 2, 20, 29, 31 | M02.04; M29.03 | `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/micear/zx48-micear-stack.tap`; `tests/fixtures/micear/M29-boundary-separation.json` | `certification/results/micear/M33.R60.json` |
| M33.R61 | 3764 | WZSN Architecture #3 uses the upstream NONE/INTERFACE1/EAR_MIC networking-mode arbiter | 2, 29, 31 | M29.03 | `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/micear/M29-two-node-virtual.json` | `certification/results/micear/M33.R61.json` |
| M33.R62 | 3765 | Ear+Mic uses distributed RAM stack; no networking-ROM paging or auto-install | 1, 2, 21, 29, 31 | M21.02; M29.03 | `tests/fixtures/micear/zx48-micear-stack.tap`; `tests/fixtures/micear/M29-boundary-separation.json` | `certification/results/micear/M33.R62.json` |
| M33.R63 | 3766 | WZSN Ear+Mic exclusively owns cassette EAR/MIC and uses BOOTSTRAP_TAPE for in-mode stack loading | 21, 24, 29, 31 | M21.03; M24.01 | `tests/fixtures/micear/M21-bootstrap-flow.json`; `tests/fixtures/micear/M24-port-mode.json`; `tests/fixtures/networking/wzsn-networking-modes-v1.json` | `certification/results/micear/M33.R63.json` |
| M33.R64 | 3767 | WZSN Ear+Mic requires certified 48K Issue-2 profile; Control Port multiplicity does not imply cross-process network timing | 2, 29, 31 | M02.04; M29.03 | `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/micear/M29-boundary-separation.json`; `tests/fixtures/micear/M29-two-node-virtual.json` | `certification/results/micear/M33.R64.json` |

## M34. Architecture section 34: Architecture Audit Record

**Architecture authority:** `M` source lines 3770-3812, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Reproduce all 18 mandated audit classes from the saved architecture bytes, prove the two-successive-cycle convergence rule, and prevent a clean architecture audit from being misrepresented as certification of still-unfrozen physical values.

**Required artifacts:**

- `design/warajevo-zx-spectrum-next-architecture.md`
- `design/warajevo-zx-spectrum-next-ui-architecture.md`
- `design/zx48-mic-ear-router-network-architecture.md`
- `certification/manifests/micear-section32-decisions.json`
- `certification/manifests/micear-section31-acceptance.json`
- `certification/manifests/micear-section33-rtm.json`
- `certification/manifests/micear-section-headings.json`
- `certification/manifests/micear-markdown-integrity.json`

**Artifact/source authority:** The three saved architecture files are the audit authority. The manifests/fixtures below are reproducible audit inputs or derived indexes; result JSONs are evidence outputs and may not substitute for the inputs they claim to audit.

**Tests:**

### M34.01 - execute all eighteen audit classes

Execute every Section-34 audit class from the saved disk architecture bytes. Each ledger row below names the concrete architecture/manifests/fixtures/gates that class must inspect. Audit may certify contract completeness only; unresolved physical/electrical/timing values remain governed by Section 32.

**Evidence output:** `certification/results/micear/M34.01.json`

### M34.02 - unchanged-byte convergence protocol

Run two independent audit methods: (a) structural/contract audit and (b) semantic/cross-architecture audit. Require both to report zero gaps, then re-run both methods from the beginning on the **identical saved architecture bytes** for a second successive cycle. Record the architecture SHA-256 before cycle 1 and after cycle 2 and require exact equality. Any correction resets the clean-cycle count to zero.

**Evidence outputs:** `certification/results/micear/M34-convergence-cycle1.json`; `certification/results/micear/M34-convergence-cycle2.json`; `certification/results/micear/M34-convergence-hash.txt`

### M34.03 - audit-scope limitation

Cross-check every unresolved Section-32 decision against the audit report. A clean M34 audit may state only that the uncertainty is explicit and correctly gated; it must not mark an unfrozen electrical value, physical timing value, or hardware behavior as measured/certified. Any such promotion is FAIL.

**Evidence output:** `certification/results/micear/M34.03.json`

**Pass rule:** PASS only when all 18 classes are zero-gap, both independent methods achieve two successive zero-gap cycles on identical saved bytes, and no unresolved physical value is falsely certified.

### M34 literal architecture-audit class ledger

| Subtest | Source line | Audit class | Required audit input(s) | Evidence artifact |
|---|---:|---|---|---|
| M34.A01 | 3777 | Markdown/section structure and heading continuity; | `design/zx48-mic-ear-router-network-architecture.md`; `certification/manifests/micear-section-headings.json` | `certification/results/micear/M34.A01.json` |
| M34.A02 | 3778 | ASCII-only encoding and malformed-fence detection; | `design/zx48-mic-ear-router-network-architecture.md`; `certification/manifests/micear-markdown-integrity.json` | `certification/results/micear/M34.A02.json` |
| M34.A03 | 3779 | frozen/baseline/proposed/open status consistency; | `design/zx48-mic-ear-router-network-architecture.md`; `certification/manifests/micear-section32-decisions.json` | `certification/results/micear/M34.A03.json` |
| M34.A04 | 3780 | every `[OPEN]` body item mapped to a Section-32 decision gate; | `design/zx48-mic-ear-router-network-architecture.md`; `certification/manifests/micear-section32-decisions.json` | `certification/results/micear/M34.A04.json` |
| M34.A05 | 3781 | command grammar versus examples and error semantics; | `tests/fixtures/micear/M35-command-grammar.json`; `tests/fixtures/micear/M37-error-catalog.json`; `tests/fixtures/micear/M38-example-sessions.txt` | `certification/results/micear/M34.A05.json` |
| M34.A06 | 3782 | station-name/address/service-endpoint consistency; | `tests/fixtures/micear/M04-lexical-conventions.json`; `tests/fixtures/micear/M11-address-registration.json`; `tests/fixtures/micear/M35-command-grammar.json` | `certification/results/micear/M34.A06.json` |
| M34.A07 | 3783 | message queue, durability, FIFO, acknowledgment, and offline semantics; | `tests/fixtures/micear/M15-queue-durability.json`; `tests/fixtures/micear/M19-retrieval-vectors.json`; `tests/fixtures/micear/M25-recovery-matrix.json` | `certification/results/micear/M34.A07.json` |
| M34.A08 | 3784 | object-transfer lifecycle consistency; | `tests/fixtures/micear/M15-object-staging.json`; `tests/fixtures/micear/M28-transfer-vectors.json`; `certification/gates/micear/D21.md`; `certification/gates/micear/D43.md` | `certification/results/micear/M34.A08.json` |
| M34.A09 | 3785 | Spectrum stack hook/reentrancy/memory-contract consistency; | `tests/fixtures/micear/zx48-micear-stack.tap`; `tests/fixtures/micear/M20-hook-safety.json`; `certification/gates/micear/D14.md`; `certification/gates/micear/D16.md` | `certification/results/micear/M34.A09.json` |
| M34.A10 | 3786 | electrical/timing-controller/Linux trust-boundary consistency; | `certification/gates/micear/D01.md`; `certification/gates/micear/D02.md`; `certification/gates/micear/D41.md`; `certification/gates/micear/D42.md`; `tests/fixtures/micear/M09-controller-stress.json`; `certification/hardware/micear/M08-fault-isolation.csv` | `certification/results/micear/M34.A10.json` |
| M34.A11 | 3787 | WZSN cross-architecture timing and integration consistency; | `design/warajevo-zx-spectrum-next-architecture.md`; `design/warajevo-zx-spectrum-next-ui-architecture.md`; `design/zx48-mic-ear-router-network-architecture.md`; `tests/fixtures/micear/M29-boundary-separation.json`; `tests/fixtures/micear/M29-speed-invariance.json` | `certification/results/micear/M34.A11.json` |
| M34.A12 | 3788 | Interface-1 versus Ear+Mic mutual exclusion and BASIC-hook ownership; | `design/warajevo-zx-spectrum-next-architecture.md`; `design/warajevo-zx-spectrum-next-ui-architecture.md`; `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/micear/zx48-micear-stack.tap` | `certification/results/micear/M34.A12.json` |
| M34.A13 | 3789 | WZSN 48K Issue-2 availability gating and no networking-ROM paging; | `design/warajevo-zx-spectrum-next-architecture.md`; `tests/fixtures/networking/wzsn-networking-modes-v1.json`; `tests/fixtures/micear/M29-boundary-separation.json` | `certification/results/micear/M34.A13.json` |
| M34.A14 | 3790 | cassette EAR/MIC ownership and `BOOTSTRAP_TAPE` exclusivity; | `design/warajevo-zx-spectrum-next-architecture.md`; `tests/fixtures/micear/M21-bootstrap-flow.json`; `tests/fixtures/micear/M24-port-mode.json` | `certification/results/micear/M34.A14.json` |
| M34.A15 | 3791 | ordinary multi-process Control Ports versus deterministic virtual-router timing separation; | `design/warajevo-zx-spectrum-next-architecture.md`; `design/warajevo-zx-spectrum-next-ui-architecture.md`; `tests/fixtures/micear/M29-boundary-separation.json`; `tests/fixtures/micear/M29-two-node-virtual.json` | `certification/results/micear/M34.A15.json` |
| M34.A16 | 3792 | implementation-phase dependency and exit-gate coverage; | `certification/manifests/micear-phase-gates.json`; `certification/manifests/micear-section32-decisions.json` | `certification/results/micear/M34.A16.json` |
| M34.A17 | 3793 | acceptance-criterion and traceability coverage; and | `certification/manifests/micear-section31-acceptance.json`; `certification/manifests/micear-section33-rtm.json` | `certification/results/micear/M34.A17.json` |
| M34.A18 | 3794 | contradiction/obsolete-wording scan. | `design/warajevo-zx-spectrum-next-architecture.md`; `design/warajevo-zx-spectrum-next-ui-architecture.md`; `design/zx48-mic-ear-router-network-architecture.md`; `certification/manifests/micear-section32-decisions.json` | `certification/results/micear/M34.A18.json` |

## M35. Architecture section 35: Command Grammar

**Architecture authority:** `M` source lines 3813-3893, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Appendix A command grammar.

**Required artifacts:**

- `tests/fixtures/micear/M35-command-grammar.json`
- `tests/fixtures/micear/zx48-micear-basic-immediate.tap`
- `certification/gates/micear/D10.md`
- `certification/gates/micear/D13.md`

**Artifact/source authority:** Project parser vector corpus generated from frozen grammar.

**Tests:**

### M35.01

Generate positive/negative parser cases from every Appendix-A production: DEVICE/NAME/AREA/NODE/ADDRESS/N/ROUTER_ENDPOINT and all FORMAT/SAVE/LOAD/VERIFY/MERGE/OPEN/PRINT/INPUT/INKEY$/CLOSE/MOVE forms; `"ROUTER"` and `"0.0"` identify the same local router service endpoint, are allowed only where endpoint grammar permits, are never assignable ordinary stations, and the literal registrations `FORMAT "n";"ROUTER"` and `FORMAT "n";"0.0"` must be rejected. AREA/NODE acceptance that depends on reserved ranges is BLOCKED_GATE until D10 is frozen, and MOVE compatibility/accepted-source-destination grammar is BLOCKED_GATE until D13 is frozen; the test may not infer either from examples or provisional implementation behavior.

**Evidence output:** `certification/results/micear/M35.01.json`

**Pass rule:** PASS only when the stated behavior is exact and reproducible. Any unexplained mismatch is FAIL. If the architecture intentionally gates a value not yet frozen, the result is BLOCKED_GATE until the named gate artifact exists; no guessed/default value may be certified.

## M36. Architecture section 36: Packet Definitions

**Architecture authority:** `M` source lines 3894-3966, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Prove the uplink/downlink packet headers, object metadata, logical transaction identity, and message envelope exactly as Appendix B defines them.

**Required artifacts:**

- `tests/fixtures/micear/M36-packet-wire-vectors.bin`
- `tests/fixtures/micear/M36-object-metadata-vectors.bin`
- `tests/fixtures/micear/M36-message-envelope-vectors.bin`
- `certification/gates/micear/D07.md`
- `certification/gates/micear/D08.md`
- `certification/gates/micear/D36.md`
- `certification/gates/micear/D43.md`
- `certification/gates/micear/D45.md`
- `certification/gates/micear/D46.md`

**Artifact/source authority:** Project-owned independent wire-vector generators derived from Appendix B after the applicable D-gates freeze numeric/layout/integrity details. The vectors are generated independently of the production encoder/decoder.

**Tests:**

### M36.01 - routed-packet headers and transaction identity

After relevant gates, compare encoder/decoder byte-for-byte to independently generated uplink/downlink/control vectors. Prove the initial uplink field order/offsets are protocol-version, packet-type, destination area, destination node, sequence, payload length, payload, CRC; prove source is absent and router-derived. Prove the downlink adds source area/node before destination area/node at the Appendix-B offsets. Enforce one frozen CRC only; payload boundaries/limits follow D08; packet sequence wrap/recovery follows D45; logical stream/transfer identity is separate from packet sequence and follows D46; packet type/header assignments follow D36; reject reserved/version/length/CRC violations.

**Evidence output:** `certification/results/micear/M36.01.json`

### M36.02 - object metadata

Using `M36-object-metadata-vectors.bin`, encode/decode every Appendix-B object-metadata field: object type; object length; load address when relevant; execution/autostart metadata when relevant; array metadata when relevant; complete-object checksum. Exercise each required Spectrum object class with relevant/not-relevant optional fields and reject missing, contradictory, truncated, or overlong metadata. The complete-object integrity algorithm is BLOCKED_GATE until D43 is frozen; no packet CRC may substitute for the object checksum.

**Evidence output:** `certification/results/micear/M36.02.json`

### M36.03 - message envelope

Using `M36-message-envelope-vectors.bin`, encode/decode every Appendix-B message-envelope field: message identifier, source address, source-name snapshot, payload length, payload, and payload checksum. Prove payload length exactly bounds the message payload and malformed/truncated/extra-byte envelopes are rejected. The payload-integrity algorithm is BLOCKED_GATE until D43 is frozen and remains logically distinct from the link packet CRC.

**Evidence output:** `certification/results/micear/M36.03.json`

**Pass rule:** PASS only when M36.01-M36.03 prove every Appendix-B field and layering distinction exactly. Any D07/D08/D36/D43/D45/D46-dependent value remains BLOCKED_GATE until frozen; no provisional implementation value may be certified.

## M37. Architecture section 37: Error Catalog

**Architecture authority:** `M` source lines 3967-4006, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Appendix C semantic error catalog and its required examples. Error precedence is owned/tested by Section 18 (M18), not invented by this appendix test.

**Required artifacts:**

- `tests/fixtures/micear/M37-error-catalog.json`
- `certification/gates/micear/D25.md`

**Artifact/source authority:** Project-owned error-state vectors generated from the literal Appendix-C catalog. D25 freezes report letters/numeric identifiers only; the semantic distinctions/text are baseline.

**Tests:**

### M37.01 - literal error catalog

Construct one exact failing state for each of the thirteen Appendix-C semantic reports: `Invalid station`, `Missing station`, `Station name in use`, `Station not set`, `Station not found`, `Station not available`, `Station busy`, `Network timeout`, `Network not present`, `Network data error`, `Network version error`, `Transfer rejected`, and `Verification has failed`. Prove each condition maps to its own semantic report and no pair is collapsed. Concrete report letter/numeric identifiers remain BLOCKED_GATE until D25 is frozen.

**Evidence output:** `certification/results/micear/M37.01.json`

### M37.02 - required Appendix-C examples

Replay the five required semantic examples exactly: `BAD-01` -> Invalid station; `NOEXST` -> Station not found; known disconnected `GAME02` -> Station not available; `FORMAT "n";"MASTER"` when MASTER belongs to another active station -> Station name in use; and rejected `SAVE *... CODE 32768,4096` -> Transfer rejected. The fixture must isolate the Appendix-C stated precondition for each example; Section-18 precedence is separately proved by M18 and is not redefined here.

**Evidence output:** `certification/results/micear/M37.02.json`

**Pass rule:** PASS only when the literal catalog meanings and all five required examples match Appendix C. D25-dependent report identifiers remain BLOCKED_GATE until frozen.

## M38. Architecture section 38: Example Sessions

**Architecture authority:** `M` source lines 4007-4090, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Replay each of Appendix D's ten sessions independently, proving the semantic state/queue/object behavior while treating illustrative report letters/numeric identifiers exactly as illustrative until D25 freezes them.

**Required artifacts:**

- `tests/fixtures/micear/M38-example-sessions.txt`
- `tests/fixtures/micear/M11-address-registration.json`
- `tests/fixtures/micear/M12-link-liveness.json`
- `tests/fixtures/micear/M15-queue-durability.json`
- `tests/fixtures/micear/M17-command-semantics.json`
- `tests/fixtures/micear/M18-errors.json`
- `tests/fixtures/micear/M19-retrieval-vectors.json`
- `tests/fixtures/micear/zx48-micear-object-transfer.tap`
- `certification/gates/micear/D10.md`
- `certification/gates/micear/D22.md`
- `certification/gates/micear/D25.md`
- `certification/gates/micear/D40.md`
- `certification/gates/micear/D43.md`
- `certification/gates/micear/D48.md`

**Artifact/source authority:** `M38-example-sessions.txt` is a project-owned literal transcript generated from Appendix D. Lower-level fixtures/gates provide the machine/router state needed for each session; D25 affects report identifiers only.

**Tests:**

### M38.01 - D.1 registration

Replay `FORMAT "n";"MASTER"` against the preconfigured permitted local binding. Prove registration/confirmation succeeds and local identity becomes MASTER. The semantic success may be certified before D25; the exact report letter/numeric identifier formatting is BLOCKED_GATE until D25.

**Evidence output:** `certification/results/micear/M38.01.json`

### M38.02 - D.2 unknown station

Replay `OPEN #4;"n";"NOEXST"` and prove the semantic result is `Station not found`. The illustrative report letter `s` must not be frozen by this appendix; exact report identifiers follow D25 only.

**Evidence output:** `certification/results/micear/M38.02.json`

### M38.03 - D.3 known but unavailable station

Using D48-frozen liveness state, register GAME02 and mark it unavailable/disconnected, then replay `OPEN #4;"n";"GAME02"`. Prove `Station not available`, distinct from not-found. The illustrative report letter `t` remains non-normative until D25.

**Evidence output:** `certification/results/micear/M38.03.json`

### M38.04 - D.4 send asynchronous message

After D40/D43 freeze the applicable stream and message-integrity details, replay the exact OPEN/PRINT/CLOSE sequence for `READY TO START`. Prove one complete message is committed for online GAME02 and sender success means router queue acceptance, **not recipient read**. Exact success report identifiers remain D25-gated.

**Evidence output:** `certification/results/micear/M38.04.json`

### M38.05 - D.5 receive next message from any sender

After D40/D43, open the router endpoint, execute `INPUT #4;S$,M$`, and close. Prove the next FIFO message yields source name `MASTER` and payload `READY TO START` in the Appendix-D field order, then follows the delivery acknowledgment contract.

**Evidence output:** `certification/results/micear/M38.05.json`

### M38.06 - D.6 receive from one source

After D40/D43, replay `OPEN #4;"n";"MASTER"`, `INPUT #4;M$`, `CLOSE #4`; prove source-specific FIFO selection and that the input field contains only the message payload, not the router-endpoint source-name field.

**Evidence output:** `certification/results/micear/M38.06.json`

### M38.07 - D.7 send CODE

After D22/D43 freeze applicable size/integrity rules, replay `SAVE *"n";"GAME02" CODE 32768,4096` using `zx48-micear-object-transfer.tap`; prove the exact 4096-byte CODE object, load address 32768, metadata, and end-to-end integrity are staged/committed under the object-transfer contract.

**Evidence output:** `certification/results/micear/M38.07.json`

### M38.08 - D.8 receive CODE

After D22/D43, replay `LOAD *"n";"MASTER" CODE 32768,4096`; prove the selected CODE object is validated and written exactly to the requested destination/range without damaging protected resident-stack memory.

**Evidence output:** `certification/results/micear/M38.08.json`

### M38.09 - D.9 verify screen

Replay `VERIFY *"n";"GAME02" SCREEN$` against matching and deliberately mismatching screen objects. Matching data succeeds without modification; mismatching data returns the Section-18/Appendix-C `Verification has failed` semantic report.

**Evidence output:** `certification/results/micear/M38.09.json`

### M38.10 - D.10 address form

After D10 freezes remaining address reservations, use an ordinary permitted station at `1.3` and replay the exact OPEN/PRINT/CLOSE `HELLO` session. Prove address-form routing resolves the intended station and commits the same character-message semantics as name-form routing; if D10 reserves `1.3`, replace the example only by an explicitly architecture-approved revision rather than silently changing the certified grammar.

**Evidence output:** `certification/results/micear/M38.10.json`

**Pass rule:** PASS only when all ten Appendix-D sessions reproduce their stated semantic behavior. D10/D22/D40/D43/D48-dependent behavior remains BLOCKED_GATE until frozen. D25 gates only the exact report letters/numeric identifiers that Appendix D itself marks or treats as illustrative; it does not block semantic state-transition testing.

## M39. Architecture section 39: Provisional Memory Map

**Architecture authority:** `M` source lines 4091-4116, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Ensure Appendix E remains illustrative only and that the D16-frozen release map is generated from the actual assembled resident-stack binary and checked against every memory consumer named by the architecture.

**Required artifacts:**

- `certification/gates/micear/D16.md`
- `build/spectrum/zx48-micear-stack.bin`
- `build/spectrum/zx48-micear-stack.map`
- `tests/fixtures/micear/zx48-micear-protected-memory.tap`
- `tests/fixtures/micear/M39-memory-boundaries.json`

**Artifact/source authority:** The actual assembled stack binary/map is the allocation authority after D16 freezes the applicable Phase-2/3/6 reservation. The project boundary fixture constructs deterministic 48K states for every Appendix-E protected consumer.

**Tests:**

### M39.01 - provisional-address exclusion and assembled-map authority

Do not certify `C000-CFFF`, `D000-D7FF`, `D800-DFFF`, `E000-EFFF`, `F000-F7FF`, or `F800-FFFF` merely because Appendix E illustrates them. At each D16 phase gate, require the final/respective map to be generated from `zx48-micear-stack.bin`/`.map`, and prove every code/data/buffer/hook symbol lies within the D16-frozen reservation. Any hard-coded dependency on the illustrative ranges that differs from the frozen assembled map is FAIL.

**Evidence output:** `certification/results/micear/M39.01.json`

### M39.02 - all Appendix-E memory-consumer overlap checks

Using `M39-memory-boundaries.json` and `zx48-micear-protected-memory.tap`, independently place each named consumer against the reservation boundary and prove install/receive/operation guards prevent destructive overlap with: **BASIC program space; BASIC variables; calculator stack; machine stack; UDGs; system variables; screen memory; and interrupt requirements/state**. Exercise boundary-minus-one, exact-boundary, boundary-plus-one, and largest-valid payload cases. A receive that would overwrite the resident stack/hook or any protected machine/BASIC region must be rejected before mutation.

**Evidence output:** `certification/results/micear/M39.02.json`

**Pass rule:** PASS only when the actual assembled map—not the illustrative Appendix-E addresses—is the tested allocation and all eight named memory consumers are protected exactly. D16-dependent addresses/footprint remain BLOCKED_GATE until frozen.

## M40. Architecture section 40: Provisional Timing Profile

**Architecture authority:** `M` source lines 4117-4138, inclusive. This test block covers every normative sentence, bullet, table row, code-block contract, REQUIRED/LATER/REPLACE/DROP classification, and acceptance/gate statement in that line range. Explanatory prose is not promoted beyond the architecture.

**Forensic test intent:** Keep every Appendix-F value provisional until its controlling gate and real Issue-2 validation are complete, and prove WZSN represents the frozen T-state timings only through the upstream canonical master-tick domain.

**Required artifacts:**

- `certification/gates/micear/D05.md`
- `certification/gates/micear/D06.md`
- `certification/gates/micear/D08.md`
- `tests/fixtures/micear/M40-provisional-value-matrix.json`
- `certification/hardware/micear/M40-timing-validation.csv`
- `certification/hardware/micear/M40-link-mode-window-validation.csv`
- `tests/fixtures/micear/M29-speed-invariance.json`

**Artifact/source authority:** D05 freezes physical pulse timing profiles, D06 freezes physical/link preamble and sync, and D08 freezes payload size. Hardware CSVs are repeated measurements/functional validation on identified real Issue-2 machines. Appendix F itself is never the normative value source.

**Tests:**

### M40.01 - literal provisional-value ledger and gate ownership

`M40-provisional-value-matrix.json` must contain exactly the Appendix-F starting entries and mark each as **PROVISIONAL**, never certified merely from the appendix: pilot half-pulse `2168 T-states`; sync-1 `667`; sync-2 `735`; zero half-pulse `855`; one half-pulse `1710`; pilot count `64 half-pulses`; inter-frame idle `3 ms minimum`; transfer mode `half-duplex`; chunk window `1 packet`; initial payload `32 or 64 bytes`. Map pulse widths/idle/profile behavior to D05, pilot/sync framing structure to D06 as applicable, and payload choice to D08. No implementation default may close these gates implicitly.

**Evidence output:** `certification/results/micear/M40.01.json`

### M40.02 - real Issue-2 validation of every final physical value

For every final release entry replacing or confirming an Appendix-F starting value, require repeated tests on identified real unmodified Issue-2 hardware. `M40-timing-validation.csv` records measured pilot/sync/data pulse widths, pilot behavior/count, idle/turnaround behavior relevant to the profile, tolerances, machine identity, repetition count, and pass/fail against the frozen D05/D06 values. `M40-link-mode-window-validation.csv` proves the frozen half-duplex/one-packet-window operating baseline works without protocol-significant overlap/loss on the physical link. Payload-size validation must include the D08-selected maximum and boundary cases. Any Appendix-F entry lacking real-hardware validation remains BLOCKED_GATE.

**Evidence output:** `certification/results/micear/M40.02.json`

### M40.03 - WZSN master-tick conversion and speed invariance

Convert each frozen T-state pulse value through the certified 48K Issue-2 WZSN machine profile into canonical master ticks using the Core/System architecture's master-tick mapping. Compare the resulting virtual waveform/event sequence at multiple WZSN runtime speeds using `M29-speed-invariance.json`; host pacing may change wall-clock duration only. Architecture #3 may not introduce an independent emulator-time base or use host wall-clock time to define virtual protocol edges.

**Evidence output:** `certification/results/micear/M40.03.json`

**Pass rule:** PASS only when every Appendix-F entry is either replaced/confirmed by its frozen gate plus real Issue-2 evidence or remains explicitly BLOCKED_GATE, and WZSN represents the resulting protocol solely on canonical master ticks.

# Part IV - Release certification execution

## Mandatory execution order

A release certification run executes tests in this exact order: `C01` through `C55`, then `U01` through `U53`, then `M01` through `M40`. Subtests, acceptance ledgers and gate ledgers execute at their position inside the parent section. Architecture #3 tests returning BLOCKED_GATE prevent an Architecture-#3 completeness claim but do not retroactively invalidate a separately scoped Architecture-#1/#2 release if the architecture allows that scope.

## Per-test result schema

Every JSON result contains at minimum:

```text
test_id
architecture_file_sha256
architecture_source_lines
wzsn_commit
build_id
machine_profile
required_artifacts [{path, sha256, provenance_class, source_authority, external_catalog_id_if_any, acquisition_filename_if_external, acquisition_source_if_external, archive_member_if_any, archive_member_sha256_if_any, status}]
per_test_artifact_manifest_record
gate_artifacts [{id, path, status}]
expected
observed
canonical_checkpoint_hashes
trace_file_if_used
first_divergence_if_any
result = PASS | FAIL | BLOCKED_GATE | N/A
reviewer
notes
```

## Zero-unexplained-failure rule

No release-level PASS is permitted while any applicable test has FAIL, unexplained divergence, missing mandatory artifact, stale architecture fingerprint, or incorrectly bypassed gate. External-oracle exceptions require stronger evidence, exact external test/revision identity, a project-owned regression, and rerun of every dependent test.

## Trace-on-failure rule

For deterministic/timing-sensitive failures, WZSN freezes the per-instance 16 MiB circular `TIMING_FULL` trace at the earliest identifiable mismatch. The evidence bundle retains the trace, expected/observed checkpoint, artifact hashes and build identity. Trace-on/off/wrap/failure must remain observationally equivalent as required by C44.

## Mutation proof of the certification system

At least periodically, deliberately mutate test builds (for example one-T-state contention shift, one undocumented flag error, one ULA fetch error, one interrupt edge delay, one tape pulse suppression, one snapshot bit restoration error, duplicate writable MDR ownership, Ear+Mic byte shortcut) and prove the expected gate fails. A certification suite that cannot detect its target fault is itself defective.

## Release evidence bundle

The release bundle contains project-generated evidence only; private copyrighted difficult media remain outside it unless separately authorized.

```text
certification/
  certificate.txt
  build-manifest.json
  architecture-fingerprints.json
  artifact-manifest.json
  manifests/per-test-artifacts.json
  gates/
  manifests/
  results/core/
  results/ui/
  results/micear/
  hardware/
  traces/
```

## Certification-specification reconciliation record

Before this specification is accepted as converged, its maintainer performs complete architecture-ordered reconciliation passes over the exact saved architecture bytes named in the source-fingerprint table. A pass begins at Core/System line 1, continues through its final line, then UI line 1 through its final line, then MIC/EAR line 1 through its final line. Every normative sentence, bullet, table row, code-block contract, disposition, decision/gate, acceptance item, appendix rule, and artifact dependency is checked against the corresponding `C`, `U`, or `M` test block. Artifact requirements are simultaneously checked against the exact per-test artifact manifest and, for external material, against this document's acquisition/reference catalog.

The reconciliation record retained with the release must contain the proof-document SHA-256 for each pass, all three architecture SHA-256 values, the pass sequence number, gap count, corrections made (if any), and reviewer identity. A correction resets the clean-pass counter. Two complete consecutive passes over identical proof bytes with `gap_count = 0` are mandatory before the specification itself may be called forensically reconciled.

## Final certification statement

A scope is certified only when every applicable test in its architecture-ordered sequence is PASS, every required gate is frozen, every acceptance item is individually evidenced, every exact required artifact is present and identified, cross-host/compiler deterministic comparisons required by the architecture agree, and the unexplained-failure count is zero.

The certification statement must name the exact architecture file hashes and machine profiles. It must not claim universal transistor-level identity with every ZX Spectrum revision.

## Forensic maintenance rule

Any architecture edit invalidates the prior reconciliation. Update the test block(s), restart audit from Core/System line 1, then UI line 1, then MIC/EAR line 1. If any gap is corrected during a pass, the consecutive-clean counter resets to zero. Certification-document convergence requires at least two consecutive complete architecture-ordered audits over identical saved proof bytes with zero gaps.

