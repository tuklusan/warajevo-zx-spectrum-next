<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# ZX Printer Inventory

Status: CLOSED_INVENTORY_FOR_CR-0242

Authority: `reference/original-warajevo/source/src/spectrum-kernel/ZXPRINT.ASM`
was scanned in full (340 lines). The preserved peripheral exposes port 251,
four presentation modes, pixel buffering, motor/status transitions, row flush,
and a host printer boundary.

| Legacy surface | Preserved routines/state | Disposition | Architecture replacement |
|---|---|---|---|
| Port 251 status read | `ZXP_IN`, `Z_IN_FAR`, `ZXP_IN1` | REQUIRED | deterministic status API |
| Port 251 pixel/control write | `ZXP_OUT`, `Z_OUT_FAR` | REQUIRED | deterministic peripheral API |
| Motor on/off and ready rejection | `MOTON`, `MOTOFF`, `NOPRINTER` | REQUIRED | explicit state transitions |
| Epson normal mode | `PRTMODE=1`, `EPSCODES`, `FLUSH` | REQUIRED | virtual capture mode |
| Epson enlarged mode | `PRTMODE=2`, `NOPIXDUP` | REQUIRED | virtual capture mode |
| HP standard mode | `PRTMODE=3`, `HPLASER`, `HPFLUSH` | REQUIRED | virtual capture mode |
| HP enlarged mode | `PRTMODE=4`, `HPLASER`, `NOBIGHP` | REQUIRED | virtual capture mode |
| Pixel row buffering | `ZXP_BUF`, `ZXP_CNT`, `ZXP_ROW`, `ZXCONT` | REQUIRED | bounded canonical peripheral state |
| Row/line flush | `FLUSH`, `HPFLUSH`, `ZXP_L1`, `ZXP_L2`, `HPLOOP` | REQUIRED | captured output event stream |
| Printer configuration | `SETASCII`, setup mode mapping | REQUIRED | UI settings and semantic command |
| BIOS LPT/INT 17 output | `INT17`, `DOINT17` | REPLACE | application export adapter; never core state |
| Tape/snapshot print-to-host | environment `printtoprinter*` | DROP | UI architecture explicitly drops legacy host print |

## Required Boundaries

The core owns port-visible status, motor, buffering, mode, and flush ordering.
The application may capture or export completed output, but host LPT ports,
BIOS interrupts, handles, paths, and wall-clock availability do not enter
canonical machine state. The four mode identities and status bit semantics must
be covered by regression tests before Phase-11 closure.

## Zero-Gap Reconciliation

The complete `ZXPRINT.ASM` scan, `SPECSIM.ASM` port dispatch, setup mappings,
and UI Architecture 2 printer sections are the authorities. Any unlisted
user-visible behavior must be added before printer implementation proceeds.
