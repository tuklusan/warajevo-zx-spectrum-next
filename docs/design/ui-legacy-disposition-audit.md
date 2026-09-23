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

## Disposition boundary

The categories marked retained, mapped, or assigned above follow the exact
item-level destinations and exceptions in UI Architecture Section 20. This
summary does not promote every retained conversion or later machine feature
into the initial milestone. It also does not count a specification mapping as
implementation evidence. Task 451's UI acceptance inventory and the applicable
final release gates remain authoritative for completion.
