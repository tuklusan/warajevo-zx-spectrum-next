<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Legacy UI disposition audit

Task 453 is audited at the specification level. Every category in UI
Architecture Section 20.1–20.21 has an explicit destination or disposition;
no historical menu category is unclassified. This confirms the mapping, not
that each destination is implemented or accepted. The UI acceptance audit
records the outstanding implementation and proof gaps. No Architecture-#3
behavior is implemented or promoted by this reconciliation.

| UI Architecture §20 | Legacy category | Reconciliation | Remaining implementation/evidence work |
|---|---|---|---|
| 20.1 | TapeFiles > Tapes | Insert/Eject quick actions, Tape Manager, report export, and dropped print paths are individually dispositioned. | Verify all retained actions use the shared command and media paths; retained media operations lack criterion-level proof. |
| 20.2 | TapeFiles > Blocks | Retained block operations map to Tape Manager; the native-only exclusion operation maps to Compatibility Tools. | Verify format-specific availability, destructive-operation behavior, and block edits with pinned tests. |
| 20.3 | TapeFiles > Implode | Native tape compression/linearization and efficiency operations map to Compatibility Tools. | These conversion/maintenance destinations lack implementation and acceptance evidence. |
| 20.4 | TapeFiles > Communications > RS232 | Legacy PC-to-Spectrum bridge and DOS helpers are explicitly dropped; authentic Interface 1 serial behavior remains a separate machine concern. | No legacy host bridge work is required for the initial UI. Any future hardware bridge needs a separately approved architecture. |
| 20.5 | TapeFiles > Communications > Cassette | Live physical capture is explicitly deferred; DOS calibration is dropped. | No initial destination; future capture requires its own feature design and acceptance. |
| 20.6 | TapeFiles > Convert > source-language/text conversions | Each listed language/text conversion is assigned to Compatibility Tools. | Conversion inventory has no implementation or conversion-fixture proof. |
| 20.7 | TapeFiles > Convert > SCREEN$ | Legacy TIFF conversions map to Compatibility Tools; normal screenshots use PNG. | TIFF compatibility conversion remains unverified. |
| 20.8 | TapeFiles > Convert > other-emulator/media formats | Every listed format conversion is assigned to Compatibility Tools; native runtime support remains distinct. | Conversion inventory has no implementation or format-fixture proof. |
| 20.9 | Z80Snaps > Snaps | Select maps to Load Snapshot, Rename to Save Snapshot As, Info to Snapshot Inspector, and mounted-media unselect is dropped. | Snapshot Inspector is an OPEN UI acceptance criterion; snapshot format compatibility evidence remains incomplete. |
| 20.10 | Z80Snaps > Edit | Register, hardware, and memory editing map to shared debugger/state-inspection tools. | Verify the shared implementation and mutation boundaries with UI and core regressions. |
| 20.11 | Z80Snaps > Convert | Each listed snapshot conversion maps to Compatibility Tools; SNA/Z80 load remains native. | Conversion implementation and compatibility fixtures are missing. |
| 20.12 | MdriveFiles > Microdrive | Cartridge mount/eject and retained management functions map to Microdrive Manager. | Verify each retained operation and destructive confirmation with pinned tests. |
| 20.13 | MdriveFiles > Sectors | Verify/edit map to the Microdrive Manager advanced sector surface. | Raw-sector safety and write/flush regressions are missing. |
| 20.14 | MdriveFiles > Files | Retained logical-file operations map to Microdrive Manager. | Verify file operations and media ownership under multi-instance use. |
| 20.15 | MdriveFiles > Convert | MDR enlargement maps to Compatibility Tools. | Conversion implementation and fixture proof are missing. |
| 20.16 | DockFiles | Every operation is deferred until a Timex/DCK machine target exists. | No initial UI destination; keep the branch absent until that target is separately in scope. |
| 20.17 | DataBase | The historical database UI is dropped; Run's user goal maps to Open / Run. | Open / Run is covered by open UI acceptance criteria; a future Library would be a new feature. |
| 20.18 | DataBase > Convert | Each listed conversion maps to low-priority Compatibility Tools. | Conversion implementation and fixtures are missing; low priority does not change the documented disposition. |
| 20.19 | Setup | Emulator/video/speed/test/network/sound/Interface 1/joystick/printer/ROM controls map to Machine, Settings, toolbar, or diagnostics as specified; host-era ROM and display hacks are explicitly excluded. | Verify all destinations, availability, and shared handlers; most relevant UI criteria remain OPEN. |
| 20.20 | DOS | About and Exit map to Help and File; directory changes use native file dialogs; shell launch is dropped. | Verify native dialog and quit paths through the supported front ends. |
| 20.21 | Start/status/window controls | Start/run, help, quit, fullscreen/window behavior, and status map to modern application surfaces; DOS process/heap/child-command details are explicitly dropped. | Verify the required keyboard workflows, status, window behavior, and canonical-state invariance. |

## Item-level trace for Task 353

This crosswalk has 174 rows, one for every legacy item/conversion/facility listed in UI §20.1–20.21 (including one-column conversion inventories). `RETAIN` rows link to implementation/backlog tasks; `LATER`, `REPLACE`, and `DROP` are explicit dispositions. A task link records traceability and does not upgrade that task’s implementation/evidence status. UI §20 remains the authority for full semantics.

| UI subsection | Legacy item | WZSN destination | Disposition and task trace |
|---|---|---|---|
| 20.1 TapeFiles > Tapes | Select | Toolbar / quick action | RETAIN; Tasks 303–312 |
| 20.1 TapeFiles > Tapes | Unselect | Toolbar / quick action | RETAIN; Tasks 303–312 |
| 20.1 TapeFiles > Tapes | Parameters | Media-manager function | RETAIN; Tasks 303–312 |
| 20.1 TapeFiles > Tapes | View | Media-manager function | RETAIN; Tasks 303–312 |
| 20.1 TapeFiles > Tapes | Print > Screen | Dropped legacy host function | REPLACE; Tasks 303–312 |
| 20.1 TapeFiles > Tapes | Print > Printer | Dropped legacy host function | DROP; Tasks 303–312 |
| 20.1 TapeFiles > Tapes | Print > File | Media-manager function | DROP; Task 308 (report export excluded from initial backlog) |
| 20.1 TapeFiles > Tapes | Copy to New | Media-manager function | RETAIN; Tasks 303–312 |
| 20.2 TapeFiles > Blocks | Reorder | Media-manager function | RETAIN; Tasks 306–310 |
| 20.2 TapeFiles > Blocks | Extract | Media-manager function | RETAIN; Tasks 306–310 |
| 20.2 TapeFiles > Blocks | Delete | Media-manager function | RETAIN; Tasks 306–310 |
| 20.2 TapeFiles > Blocks | Add | Media-manager function | RETAIN; Tasks 306–310 |
| 20.2 TapeFiles > Blocks | Edit | Media-manager function | RETAIN; Tasks 306–310 |
| 20.2 TapeFiles > Blocks | Exclude | Compatibility utility | LATER; Tasks 306–310 |
| 20.2 TapeFiles > Blocks | Change Position | Media-manager function | DROP; Tasks 306–310 |
| 20.3 TapeFiles > Implode | Compress > All blocks | Compatibility utility | LATER; Tasks 307 and 324 |
| 20.3 TapeFiles > Implode | Compress > Linearize only | Compatibility utility | LATER; Tasks 307 and 324 |
| 20.3 TapeFiles > Implode | Compress > Compress Selected | Compatibility utility | LATER; Tasks 307 and 324 |
| 20.3 TapeFiles > Implode | Decompress | Compatibility utility | LATER; Tasks 307 and 324 |
| 20.3 TapeFiles > Implode | Efficiency | Compatibility utility | LATER; Tasks 307 and 324 |
| 20.4 TapeFiles > Communications > RS232 | Send to Spectrum | Dropped legacy host function | DROP; Task 310 |
| 20.4 TapeFiles > Communications > RS232 | Receive from Spectrum | Dropped legacy host function | DROP; Task 310 |
| 20.4 TapeFiles > Communications > RS232 | Send communication program | Dropped legacy host function | DROP; Task 310 |
| 20.4 TapeFiles > Communications > RS232 | Configure RS232 | Dropped legacy host function | DROP; Task 310 |
| 20.5 TapeFiles > Communications > Cassette | Receive from cassette | Dropped legacy host function initially | LATER; Task 310 |
| 20.5 TapeFiles > Communications > Cassette | Sample from cassette | Dropped legacy host function initially | LATER; Task 310 |
| 20.5 TapeFiles > Communications > Cassette | Advanced setup | Dropped legacy host function initially | DROP; Task 310 |
| 20.6 TapeFiles > Convert > source-language/text conversions | ASCII -> BASIC | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.6 TapeFiles > Convert > source-language/text conversions | BASIC -> ASCII | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.6 TapeFiles > Convert > source-language/text conversions | ASCII -> HiSoft GENS | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.6 TapeFiles > Convert > source-language/text conversions | HiSoft GENS -> ASCII | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.6 TapeFiles > Convert > source-language/text conversions | ASCII -> HiSoft C | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.6 TapeFiles > Convert > source-language/text conversions | HiSoft C -> ASCII | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.6 TapeFiles > Convert > source-language/text conversions | ASCII -> HiSoft Pascal | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.6 TapeFiles > Convert > source-language/text conversions | HiSoft Pascal -> ASCII | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.6 TapeFiles > Convert > source-language/text conversions | ASCII -> Tassword 2 | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.6 TapeFiles > Convert > source-language/text conversions | Tassword 2 -> ASCII | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.6 TapeFiles > Convert > source-language/text conversions | ASCII -> Tassword 3 | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.6 TapeFiles > Convert > source-language/text conversions | Tassword 3 -> ASCII | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.6 TapeFiles > Convert > source-language/text conversions | ASCII -> The Last Word | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.6 TapeFiles > Convert > source-language/text conversions | The Last Word -> ASCII | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.6 TapeFiles > Convert > source-language/text conversions | ASCII -> Machine Lightning | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.6 TapeFiles > Convert > source-language/text conversions | Machine Lightning -> ASCII | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.6 TapeFiles > Convert > source-language/text conversions | ASCII -> Abersoft Forth | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.6 TapeFiles > Convert > source-language/text conversions | Abersoft Forth -> ASCII | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.6 TapeFiles > Convert > source-language/text conversions | ASCII -> Sinclair Logo | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.6 TapeFiles > Convert > source-language/text conversions | Sinclair Logo -> ASCII | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.7 TapeFiles > Convert > SCREEN$ | SCREEN$ -> color TIFF | Compatibility utility | LATER; Tasks 310 and 324 |
| 20.7 TapeFiles > Convert > SCREEN$ | SCREEN$ -> black/white TIFF | Compatibility utility | LATER; Tasks 310 and 324 |
| 20.8 TapeFiles > Convert > other-emulator/media formats | Roman & Easy -> Warajevo | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.8 TapeFiles > Convert > other-emulator/media formats | Warajevo -> Roman & Easy | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.8 TapeFiles > Convert > other-emulator/media formats | Lunter TAP -> Warajevo TAP | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.8 TapeFiles > Convert > other-emulator/media formats | Warajevo TAP -> Lunter TAP | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.8 TapeFiles > Convert > other-emulator/media formats | Irish SpecEm -> Warajevo TAP | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.8 TapeFiles > Convert > other-emulator/media formats | Warajevo TAP -> Irish SpecEm | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.8 TapeFiles > Convert > other-emulator/media formats | Polish SP/SPC -> Warajevo TAP | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.8 TapeFiles > Convert > other-emulator/media formats | Warajevo TAP -> Polish SP/SPC | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.8 TapeFiles > Convert > other-emulator/media formats | Spectrum 2.00 BLK -> Warajevo TAP | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.8 TapeFiles > Convert > other-emulator/media formats | Warajevo TAP -> BLK | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.8 TapeFiles > Convert > other-emulator/media formats | ZX Garabik LTP -> Warajevo TAP | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.8 TapeFiles > Convert > other-emulator/media formats | Warajevo TAP -> LTP | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.8 TapeFiles > Convert > other-emulator/media formats | ZX Brukner -> Warajevo TAP | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.8 TapeFiles > Convert > other-emulator/media formats | Warajevo TAP -> ZX Brukner | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.8 TapeFiles > Convert > other-emulator/media formats | TZX -> Warajevo TAP | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.8 TapeFiles > Convert > other-emulator/media formats | Warajevo TAP -> TZX | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.8 TapeFiles > Convert > other-emulator/media formats | TZX conversion setup | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.8 TapeFiles > Convert > other-emulator/media formats | ZX Museum ZXS -> Warajevo | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.8 TapeFiles > Convert > other-emulator/media formats | Warajevo TAP -> ZX Museum ZXS | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.8 TapeFiles > Convert > other-emulator/media formats | TR-DOS TRD -> Warajevo TAP | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.8 TapeFiles > Convert > other-emulator/media formats | Warajevo TAP -> TRD | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.8 TapeFiles > Convert > other-emulator/media formats | ZX32 ZXT/ZXS -> Warajevo TAP | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.8 TapeFiles > Convert > other-emulator/media formats | Warajevo TAP -> ZX32 ZXT/ZXS | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.8 TapeFiles > Convert > other-emulator/media formats | VOC -> Warajevo TAP | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.8 TapeFiles > Convert > other-emulator/media formats | Warajevo TAP -> VOC | Compatibility utility | RETAIN; Tasks 310 and 324 |
| 20.9 Z80Snaps > Snaps | Select | Toolbar / quick action | RETAIN; Tasks 290, 322, and 327 |
| 20.9 Z80Snaps > Snaps | Unselect | Dropped legacy host function | DROP; Tasks 290, 322, and 327 |
| 20.9 Z80Snaps > Snaps | Rename | Dropped legacy host function | REPLACE; Tasks 290, 322, and 327 |
| 20.9 Z80Snaps > Snaps | Info | Media-manager function | RETAIN; Tasks 290, 322, and 327 |
| 20.10 Z80Snaps > Edit | Processor registers | Debugger tool | RETAIN; Tasks 290, 322, and 327 |
| 20.10 Z80Snaps > Edit | Hardware devices | Debugger tool | RETAIN; Tasks 290, 322, and 327 |
| 20.10 Z80Snaps > Edit | Memory | Debugger tool | RETAIN; Tasks 290, 322, and 327 |
| 20.11 Z80Snaps > Convert | Spanish SPECTRUM SP -> Z80 | Compatibility utility | RETAIN; Tasks 324 and 327 |
| 20.11 Z80Snaps > Convert | Z80 -> Spanish SPECTRUM SP | Compatibility utility | RETAIN; Tasks 324 and 327 |
| 20.11 Z80Snaps > Convert | old VGASPEC SP -> Z80 | Compatibility utility | RETAIN; Tasks 324 and 327 |
| 20.11 Z80Snaps > Convert | Z80 -> old VGASPEC SP | Compatibility utility | RETAIN; Tasks 324 and 327 |
| 20.11 Z80Snaps > Convert | Irish SpecEm PRG -> Z80 | Compatibility utility | RETAIN; Tasks 324 and 327 |
| 20.11 Z80Snaps > Convert | Z80 -> Irish SpecEm PRG | Compatibility utility | RETAIN; Tasks 324 and 327 |
| 20.11 Z80Snaps > Convert | JPP SNA -> Z80 | Compatibility utility | RETAIN; Tasks 324 and 327 |
| 20.11 Z80Snaps > Convert | Z80 -> JPP SNA | Compatibility utility | RETAIN; Tasks 324 and 327 |
| 20.11 Z80Snaps > Convert | SpecEmu-G SEM -> Z80 | Compatibility utility | RETAIN; Tasks 324 and 327 |
| 20.11 Z80Snaps > Convert | Z80 -> SpecEmu-G SEM | Compatibility utility | RETAIN; Tasks 324 and 327 |
| 20.11 Z80Snaps > Convert | SP_UKV SNA 128 -> Z80 | Compatibility utility | RETAIN; Tasks 324 and 327 |
| 20.11 Z80Snaps > Convert | Z80 -> SNA 128 | Compatibility utility | RETAIN; Tasks 324 and 327 |
| 20.11 Z80Snaps > Convert | Nuclear ZX SNP -> Z80 | Compatibility utility | RETAIN; Tasks 324 and 327 |
| 20.11 Z80Snaps > Convert | Z80 -> Nuclear ZX SNP | Compatibility utility | RETAIN; Tasks 324 and 327 |
| 20.11 Z80Snaps > Convert | X128 SLT -> Z80 + TAP | Compatibility utility | RETAIN; Tasks 324 and 327 |
| 20.11 Z80Snaps > Convert | Z80 + TAP -> X128 SLT | Compatibility utility | RETAIN; Tasks 324 and 327 |
| 20.11 Z80Snaps > Convert | Spectrum 2.00 SIT -> Z80 | Compatibility utility | RETAIN; Tasks 324 and 327 |
| 20.11 Z80Snaps > Convert | Z80 -> Spectrum 2.00 SIT | Compatibility utility | RETAIN; Tasks 324 and 327 |
| 20.11 Z80Snaps > Convert | ZX32 ZXS RIFF -> Z80 | Compatibility utility | RETAIN; Tasks 324 and 327 |
| 20.11 Z80Snaps > Convert | Z80 -> ZX32 ZXS RIFF | Compatibility utility | RETAIN; Tasks 324 and 327 |
| 20.11 Z80Snaps > Convert | Any Z80 -> 48K without Interface 1 | Compatibility utility | RETAIN; Tasks 324 and 327 |
| 20.11 Z80Snaps > Convert | Any Z80 -> 128K without Interface 1 | Compatibility utility | RETAIN; Tasks 324 and 327 |
| 20.11 Z80Snaps > Convert | Z80 snapshot -> TAP | Compatibility utility | RETAIN; Tasks 324 and 327 |
| 20.12 MdriveFiles > Microdrive | Select | Toolbar / quick action | RETAIN; Tasks 313–320 and 327 |
| 20.12 MdriveFiles > Microdrive | Unselect | Toolbar / quick action | RETAIN; Tasks 313–320 and 327 |
| 20.12 MdriveFiles > Microdrive | Set default drive (legacy \`Default\`) | Media-manager function | RETAIN; Tasks 313–320 and 327 |
| 20.12 MdriveFiles > Microdrive | Catalog | Media-manager function | RETAIN; Tasks 313–320 and 327 |
| 20.12 MdriveFiles > Microdrive | Format | Media-manager function | RETAIN; Tasks 313–320 and 327 |
| 20.12 MdriveFiles > Microdrive | Optimize | Media-manager function | RETAIN; Tasks 313–320 and 327 |
| 20.12 MdriveFiles > Microdrive | View | Media-manager function | RETAIN; Tasks 313–320 and 327 |
| 20.12 MdriveFiles > Microdrive | Rename | Media-manager function | RETAIN; Tasks 313–320 and 327 |
| 20.12 MdriveFiles > Microdrive | Write protect | Media-manager function | RETAIN; Tasks 313–320 and 327 |
| 20.12 MdriveFiles > Microdrive | Write unprotect | Media-manager function | RETAIN; Tasks 313–320 and 327 |
| 20.13 MdriveFiles > Sectors | Verify | Media-manager function | RETAIN; Tasks 313–320 and 327 |
| 20.13 MdriveFiles > Sectors | Edit | Media-manager function | RETAIN; Tasks 313–320 and 327 |
| 20.14 MdriveFiles > Files | Delete | Media-manager function | RETAIN; Tasks 313–320 and 327 |
| 20.14 MdriveFiles > Files | Rename | Media-manager function | RETAIN; Tasks 313–320 and 327 |
| 20.14 MdriveFiles > Files | Hide | Media-manager function | RETAIN; Tasks 313–320 and 327 |
| 20.14 MdriveFiles > Files | Unhide | Media-manager function | RETAIN; Tasks 313–320 and 327 |
| 20.14 MdriveFiles > Files | Copy | Media-manager function | RETAIN; Tasks 313–320 and 327 |
| 20.15 MdriveFiles > Convert | Enlarge MDR to 254 sectors | Compatibility utility | LATER; Tasks 313–320 and 327 |
| 20.16 DockFiles | Select Dock | Toolbar / quick action | LATER; Task 328 |
| 20.16 DockFiles | Unselect Dock | Toolbar / quick action | LATER; Task 328 |
| 20.16 DockFiles | View Dock | Media-manager function | LATER; Task 328 |
| 20.16 DockFiles | Merge Dock | Media-manager function | LATER; Task 328 |
| 20.16 DockFiles | ROM -> DCK | Compatibility utility | LATER; Task 328 |
| 20.16 DockFiles | Binary -> LROS | Compatibility utility | LATER; Task 328 |
| 20.16 DockFiles | Binary -> AROS | Compatibility utility | LATER; Task 328 |
| 20.17 DataBase | Select DB directory | Dropped legacy host function | DROP; Tasks 327 and 324 |
| 20.17 DataBase | Unselect DB directory | Dropped legacy host function | DROP; Tasks 327 and 324 |
| 20.17 DataBase | Run | Dropped legacy host function | REPLACE; Tasks 327 and 324 |
| 20.17 DataBase | Edit/Browse | Dropped legacy host function | DROP; Tasks 327 and 324 |
| 20.17 DataBase | Sort | Dropped legacy host function | DROP; Tasks 327 and 324 |
| 20.17 DataBase | Mark sort priority > Conditional | Dropped legacy host function | DROP; Tasks 327 and 324 |
| 20.17 DataBase | Mark sort priority > Swap marker | Dropped legacy host function | DROP; Tasks 327 and 324 |
| 20.17 DataBase | Mark sort priority > Unmark all | Dropped legacy host function | DROP; Tasks 327 and 324 |
| 20.17 DataBase | Report | Dropped legacy host function | DROP; Tasks 327 and 324 |
| 20.18 DataBase > Convert | Warajevo -> SpecPic | Compatibility utility | RETAIN; Tasks 327 and 324 |
| 20.18 DataBase > Convert | SpecPic -> Warajevo | Compatibility utility | RETAIN; Tasks 327 and 324 |
| 20.18 DataBase > Convert | Warajevo -> SGD | Compatibility utility | RETAIN; Tasks 327 and 324 |
| 20.18 DataBase > Convert | SGD -> Warajevo | Compatibility utility | RETAIN; Tasks 327 and 324 |
| 20.18 DataBase > Convert | Warajevo -> ZX Rainbow | Compatibility utility | RETAIN; Tasks 327 and 324 |
| 20.18 DataBase > Convert | ZX Rainbow -> Warajevo | Compatibility utility | RETAIN; Tasks 327 and 324 |
| 20.18 DataBase > Convert | Warajevo -> SpecBase | Compatibility utility | RETAIN; Tasks 327 and 324 |
| 20.18 DataBase > Convert | SpecBase -> Warajevo | Compatibility utility | RETAIN; Tasks 327 and 324 |
| 20.19 Setup | Emulator | Main menu | RETAIN; Task 354 |
| 20.19 Setup | Video | Settings | RETAIN; Task 354 |
| 20.19 Setup | Speed | Toolbar / quick action + main menu | RETAIN; Task 354 |
| 20.19 Setup | Test | Debugger tool | RETAIN; Task 354 |
| 20.19 Setup | Network | Settings | RETAIN; Task 354 |
| 20.19 Setup | Sound | Settings | RETAIN; Task 354 |
| 20.19 Setup | Interface 1 | Settings | RETAIN; Task 354 |
| 20.19 Setup | Joystick | Settings | RETAIN; Task 354 |
| 20.19 Setup | Printer | Settings | RETAIN; Task 354 |
| 20.19 Setup | ROM | Settings / Machine | RETAIN; Task 354 |
| 20.19 Setup | MS Windows | Dropped legacy host function | DROP; Task 354 |
| 20.20 DOS | About | Main menu | RETAIN; Task 355 |
| 20.20 DOS | Directory | Dropped legacy host function | REPLACE; Task 355 |
| 20.20 DOS | Change directory | Dropped legacy host function | REPLACE; Task 355 |
| 20.20 DOS | OS Shell | Dropped legacy host function | DROP; Task 355 |
| 20.20 DOS | Exit | Main menu | RETAIN; Task 355 |
| 20.21 Legacy Start/status/window controls | Start button | Toolbar / quick action | RETAIN; Task 355 |
| 20.21 Legacy Start/status/window controls | F10 Emulator | Toolbar / quick action | RETAIN; Task 355 |
| 20.21 Legacy Start/status/window controls | F1 Help | Main menu/shortcut | RETAIN; Task 355 |
| 20.21 Legacy Start/status/window controls | Alt+F4 Exit | Native shortcut/main menu | RETAIN; Task 355 |
| 20.21 Legacy Start/status/window controls | Ctrl+F4 Close window | Native window behavior | RETAIN; Task 355 |
| 20.21 Legacy Start/status/window controls | Alt+F5 Zoom | Native/fullscreen/view behavior | REPLACE; Task 355 |
| 20.21 Legacy Start/status/window controls | Ctrl+F5 Resize | Native window behavior | REPLACE; Task 355 |
| 20.21 Legacy Start/status/window controls | Heap/free-RAM indicator | Dropped legacy host function | DROP; Task 355 |
| 20.21 Legacy Start/status/window controls | Names window | Status panel | REPLACE; Task 355 |
| 20.21 Legacy Start/status/window controls | Generated command parameters | Dropped legacy host function | DROP; Task 355 |
| 20.21 Legacy Start/status/window controls | Emulator executable filename | Dropped legacy host function | DROP; Task 355 |
| 20.21 Legacy Start/status/window controls | Temporary directory display | Dropped legacy host function | DROP; Task 355 |

## Task 327 conversion inventory and frozen availability

Each retained conversion below remains a Compatibility utility and is
`LATER`/unavailable until its converter and format-specific evidence exist.
This mapping does not authorize implementing the converter. Native loading of
SNA and Z80 snapshots remains available independently.

### §20.11 snapshot conversions — 23 entries, all LATER

| Legacy conversion | Frozen disposition |
|---|---|
| Spanish SPECTRUM SP -> Z80 | Compatibility Tools; LATER |
| Z80 -> Spanish SPECTRUM SP | Compatibility Tools; LATER |
| old VGASPEC SP -> Z80 | Compatibility Tools; LATER |
| Z80 -> old VGASPEC SP | Compatibility Tools; LATER |
| Irish SpecEm PRG -> Z80 | Compatibility Tools; LATER |
| Z80 -> Irish SpecEm PRG | Compatibility Tools; LATER |
| JPP SNA -> Z80 | Compatibility Tools; LATER |
| Z80 -> JPP SNA | Compatibility Tools; LATER |
| SpecEmu-G SEM -> Z80 | Compatibility Tools; LATER |
| Z80 -> SpecEmu-G SEM | Compatibility Tools; LATER |
| SP_UKV SNA 128 -> Z80 | Compatibility Tools; LATER |
| Z80 -> SNA 128 | Compatibility Tools; LATER |
| Nuclear ZX SNP -> Z80 | Compatibility Tools; LATER |
| Z80 -> Nuclear ZX SNP | Compatibility Tools; LATER |
| X128 SLT -> Z80 + TAP | Compatibility Tools; LATER |
| Z80 + TAP -> X128 SLT | Compatibility Tools; LATER |
| Spectrum 2.00 SIT -> Z80 | Compatibility Tools; LATER |
| Z80 -> Spectrum 2.00 SIT | Compatibility Tools; LATER |
| ZX32 ZXS RIFF -> Z80 | Compatibility Tools; LATER |
| Z80 -> ZX32 ZXS RIFF | Compatibility Tools; LATER |
| Any Z80 -> 48K without Interface 1 | Compatibility Tools; LATER |
| Any Z80 -> 128K without Interface 1 | Compatibility Tools; LATER |
| Z80 snapshot -> TAP | Compatibility Tools; LATER |

### §20.15 Microdrive conversion — 1 entry, LATER

| Legacy conversion | Frozen disposition |
|---|---|
| Enlarge MDR to 254 sectors | Compatibility Tools; LATER until the enlargement converter and media fixtures exist |

### §20.16 Dock conversions — 3 entries, deferred with the Dock branch

| Legacy conversion | Frozen disposition |
|---|---|
| ROM -> DCK | Deferred until Timex/DCK support; no initial Dock UI or converter |
| Binary -> LROS | Deferred until Timex/DCK support; no initial Dock UI or converter |
| Binary -> AROS | Deferred until Timex/DCK support; no initial Dock UI or converter |

### §20.18 database conversions — 8 low-priority entries, all LATER

| Legacy conversion | Frozen disposition |
|---|---|
| Warajevo -> SpecPic | Low-priority Compatibility utility; LATER |
| SpecPic -> Warajevo | Low-priority Compatibility utility; LATER |
| Warajevo -> SGD | Low-priority Compatibility utility; LATER |
| SGD -> Warajevo | Low-priority Compatibility utility; LATER |
| Warajevo -> ZX Rainbow | Low-priority Compatibility utility; LATER |
| ZX Rainbow -> Warajevo | Low-priority Compatibility utility; LATER |
| Warajevo -> SpecBase | Low-priority Compatibility utility; LATER |
| SpecBase -> Warajevo | Low-priority Compatibility utility; LATER |

## Disposition boundary

The categories marked retained, mapped, or assigned above follow the exact
item-level destinations and exceptions in UI Architecture Section 20. This
summary does not promote every retained conversion or later machine feature
into the initial milestone. It also does not count a specification mapping as
implementation evidence. Task 451's UI acceptance inventory and the applicable
final release gates remain authoritative for completion.
