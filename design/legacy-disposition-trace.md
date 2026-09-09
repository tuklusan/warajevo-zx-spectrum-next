<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Legacy Disposition Trace

This ledger implements the trace requirement in developer task 327. Every
legacy item from UI architecture sections 20.9 through 20.18 is listed once.
`MODERN` is represented by an initial WZSN surface, `UTILITY` is a compatibility
utility, `LATER` is deliberately deferred, `REPLACE` names the modern workflow,
and `DROP` is an explicit non-port disposition.

| Section | Legacy item | Disposition | WZSN destination |
|---|---|---|---|
| 20.9 | Select snapshot | REPLACE | Load Snapshot quick action |
| 20.9 | Unselect snapshot | DROP | Snapshots are not mounted media |
| 20.9 | Rename snapshot | REPLACE | Save Snapshot As |
| 20.9 | Snapshot Info | MODERN | Snapshot Inspector |
| 20.10 | Processor registers | MODERN | Shared debugger register editor |
| 20.10 | Hardware devices | MODERN | Shared debugger state inspector/editor |
| 20.10 | Memory | MODERN | Shared debugger memory inspector/editor |
| 20.11 | Spanish SPECTRUM SP to Z80 | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.11 | Z80 to Spanish SPECTRUM SP | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.11 | old VGASPEC SP to Z80 | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.11 | Z80 to old VGASPEC SP | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.11 | Irish SpecEm PRG to Z80 | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.11 | Z80 to Irish SpecEm PRG | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.11 | JPP SNA to Z80 | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.11 | Z80 to JPP SNA | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.11 | SpecEmu-G SEM to Z80 | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.11 | Z80 to SpecEmu-G SEM | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.11 | SP_UKV SNA 128 to Z80 | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.11 | Z80 to SNA 128 | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.11 | Nuclear ZX SNP to Z80 | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.11 | Z80 to Nuclear ZX SNP | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.11 | X128 SLT to Z80 plus TAP | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.11 | Z80 plus TAP to X128 SLT | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.11 | Spectrum 2.00 SIT to Z80 | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.11 | Z80 to Spectrum 2.00 SIT | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.11 | ZX32 ZXS RIFF to Z80 | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.11 | Z80 to ZX32 ZXS RIFF | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.11 | Any Z80 to 48K without Interface 1 | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.11 | Any Z80 to 128K without Interface 1 | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.11 | Z80 snapshot to TAP | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.12 | Select cartridge | MODERN | Microdrive Manager mount/current-drive |
| 20.12 | Unselect cartridge | MODERN | Microdrive Manager eject |
| 20.12 | Set default drive | MODERN | Microdrive Manager current/default drive |
| 20.12 | Catalog | MODERN | Microdrive Manager logical-file view |
| 20.12 | Format | MODERN | Microdrive Manager, destructive confirmation |
| 20.12 | Optimize | MODERN | Microdrive Manager |
| 20.12 | View | MODERN | Microdrive sector/allocation view |
| 20.12 | Rename cartridge | MODERN | Microdrive Manager |
| 20.12 | Write protect | MODERN | Microdrive Manager |
| 20.12 | Write unprotect | MODERN | Microdrive Manager |
| 20.13 | Verify sectors | MODERN | Microdrive Manager Advanced/Sectors |
| 20.13 | Edit sectors | MODERN | Microdrive Manager dangerous raw mode |
| 20.14 | Delete file | MODERN | Microdrive Manager |
| 20.14 | Rename file | MODERN | Microdrive Manager |
| 20.14 | Hide file | MODERN | Microdrive Manager |
| 20.14 | Unhide file | MODERN | Microdrive Manager |
| 20.14 | Copy file | MODERN | Microdrive Manager |
| 20.15 | Enlarge MDR to 254 sectors | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.16 | Select Dock | LATER | Absent until Timex/DCK support exists |
| 20.16 | Unselect Dock | LATER | Absent until Timex/DCK support exists |
| 20.16 | View Dock | LATER | Future Dock media manager |
| 20.16 | Merge Dock | LATER | Future Dock media manager |
| 20.16 | ROM to DCK | LATER | Future Dock compatibility utility |
| 20.16 | Binary to LROS | LATER | Future Dock compatibility utility |
| 20.16 | Binary to AROS | LATER | Future Dock compatibility utility |
| 20.17 | Select DB directory | DROP | Open/Run replaces legacy host function |
| 20.17 | Unselect DB directory | DROP | No historical database UI |
| 20.17 | Run | REPLACE | Open/Run; future Library may supersede |
| 20.17 | Edit/Browse | DROP | No historical database UI |
| 20.17 | Sort | DROP | No historical database UI |
| 20.17 | Mark sort priority: Conditional | DROP | No historical database UI |
| 20.17 | Mark sort priority: Swap marker | DROP | No historical database UI |
| 20.17 | Mark sort priority: Unmark all | DROP | No historical database UI |
| 20.17 | Report | DROP | No historical database UI |
| 20.18 | Warajevo to SpecPic | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.18 | SpecPic to Warajevo | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.18 | Warajevo to SGD | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.18 | SGD to Warajevo | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.18 | Warajevo to ZX Rainbow | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.18 | ZX Rainbow to Warajevo | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.18 | Warajevo to SpecBase | UTILITY | Compatibility Tools, explicit loss disclosure |
| 20.18 | SpecBase to Warajevo | UTILITY | Compatibility Tools, explicit loss disclosure |

No individual conversion is promoted to an implemented converter by this trace
CR; every conversion remains an explicitly disclosed Compatibility Utility.
