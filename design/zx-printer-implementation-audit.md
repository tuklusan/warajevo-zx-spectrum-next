<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# ZX Printer Implementation Audit

Status: CR-0251 IMPLEMENTATION SCOPE

Authority: the complete 340-line
`reference/original-warajevo/source/src/spectrum-kernel/ZXPRINT.ASM`, the
printer port dispatch in `SPECSIM.ASM`, and `design/zx-printer-inventory.md`.

## Branch Mapping

| Upstream surface | Modern implementation | Regression evidence |
|---|---|---|
| `Z_IN_FAR` absent/present status | `wz_printer_status` | mode/status and bus tests |
| `LAST_OUT` bit 7 latch | `wz_printer_t.last_output` | status test |
| `Z_OUT_FAR` bit 2 motor control | `wz_printer_write_control` | motor-edge and partial-flush tests |
| `MOTON` first-write consumption | motor transition before pixel dispatch | HP capture test |
| `NOPRINTER` absent-mode rejection | mode `NONE` remains stopped | mode/status test |
| `MOTOFF` pending flush | `flush_pending` on motor-off | partial motor-off test |
| `PRTMODE` values 1 and 2 | Epson and enlarged Epson modes | Epson flush tests |
| `PRTMODE` values 3 and 4 | HP and enlarged HP modes | HP mode test |
| `ZXP_BUF` and `ZXP_CNT` | bounded 256-byte buffer/count | Epson flush tests |
| `ZXP_ROW` progression | mode-sized row progression and reset | Epson flush tests |
| `NOPIXDUP` | enlarged Epson pixel expansion | enlarged Epson test |
| `ZXCONT` row completion | count reset, motor swallow, row advance | Epson flush tests |
| `FLUSH` slow/row threshold paths | bounded captured event | normal and partial-flush tests |
| `HPLASER`/`HPFLUSH` byte accumulation | eight-pixel HP capture event | HP capture test |
| `INT17`, BIOS/LPT, host coordinates | explicitly excluded from core | architecture contract |

## Boundary Confirmation

Port `0xfb` is routed by `wz_bus.c` to the printer only when hardware I/O decode
is enabled. The printer owns only deterministic mode, latches, motor, bounded
pixel state, and captured output events. No BIOS interrupt, LPT address, host
handle, filesystem path, wall clock, or host availability is stored or queried.

The host presentation/export adapter is deliberately deferred to Phase-11 task
234. Canonical serialization of this state is covered by the later Phase-11
state-serialization task and is not silently duplicated in this CR.

## Zero-Gap Result

Every upstream printer state field and control branch is mapped above. The only
omitted behavior is the explicitly replaced host BIOS/LPT path; no unclassified
printer behavior remains in CR-0251.
