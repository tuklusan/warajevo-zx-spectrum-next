<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Monitor and Debugger Inventory

Status: CLOSED_INVENTORY_FOR_CR-0241

Authority: `reference/original-warajevo/source/src/spectrum-kernel/SPECMON.ASM`
was scanned in full (3,451 lines, 372 labels). The command inventory below is
derived from the monitor help table and reconciled with the implementation
entry points. Internal labels are retained as routine families where they are
assembly control-flow helpers rather than independent user workflows.

## User Workflows

| Legacy command | Entry point/family | Disposition | Architecture replacement |
|---|---|---|---|
| `A` Abort / warm restart | `ABORT`, `ANEW`, `ASOFT` | REQUIRED | debugger safe restart/state boundary |
| `B` Breakpoint | `BREAKP`, `BREAK1`, `BPSUB*` | REQUIRED | shared breakpoint API |
| `C` Copy memory | `COPYMEM`, `COPYLOOP*` | REQUIRED | controlled memory tool |
| `D` Disassemble | `DISS`, `DISS_*`, `DISP`, `TOKEN` | REQUIRED | read-only disassembly view |
| `E` Execute | `EDITOR`, `EDENT`, `EDRET` | REQUIRED | controlled continue/execute policy |
| `F` Find memory | `FINDMEM`, `FIND*` | REQUIRED | bounded memory search |
| `H` Toggle hexadecimal | `HEXMODE` | REQUIRED | debugger display preference |
| `I` Single step | `ISTEP`, `SSTRAP`, `SSTOP*` | REQUIRED | scheduler-backed single-step |
| `J` Jump | `JUMP`, `JUMP*` | REQUIRED | controlled program-counter mutation |
| `K` Fill memory | `KILLMEM`, `KILLLOOP` | REQUIRED | controlled memory mutation |
| `L` Load block | `LOAD`, `LOADFORMON` boundary | REQUIRED | explicit application import |
| `M` Memory pointer/edit | `MEMPOKE`, `MEMPAGE`, `MEMPT` | REQUIRED | memory inspection/editor |
| `N` Interrupt handler | `NLOOP`, interrupt setup helpers | REQUIRED | traceable interrupt inspection |
| `O` Output file | `SETOUTPUT`, `OUTCLOSE`, `OUTERROR` | REPLACE | diagnostic export outside core |
| `P` Put bytes | `POKE`, `PO1`, `WRTBYTES` | REQUIRED | controlled memory mutation |
| `Q` Quit | `MONQUIT`, `MONEXIT` | REQUIRED | application lifecycle command |
| `R` Register change | `SETREG`, `STOREREG`, `BACKREG` | REQUIRED | shared register mutation API |
| `S` Save block | `SAVE`, `SAVEBLOCK`, `SAVEERR` | REQUIRED | explicit diagnostic/state export |
| `T` List memory | `TABULATE`, `TAB_LINE` | REQUIRED | bounded memory inspection |
| `U` Undo registers | `UNDOREG`, `UNDOLOOP` | REQUIRED | debugger mutation transaction undo |
| `V` View screen | `VIEWSCR`, `REFRESH` | REQUIRED | debugger screen inspection |
| `W` Put words | `POKEWORD`, `WRLOOP` | REQUIRED | controlled memory mutation |
| `X` Decrypt | `SETXOR`, `MWRITE` | REQUIRED | explicit compatibility memory tool |
| `Y` Memory configuration | `MEMPAGE`, `SETMMUEND` | REQUIRED | model-aware memory/state API |
| `Z` Print screen | `DUMPSCR`, `PRTOUT` | REQUIRED | printer/export boundary |
| `!` Interrupt setup | `SETIFFS`, `SETIMODE` | REQUIRED | controlled CPU state API |
| `?` Show number | `CONVNUM`, `NUM8`, `NUM16` | REPLACE | debugger value formatter |
| arithmetic operators | `ADDNUM`, `SUBNUM`, `MULNUM`, `DIVNUM`, `MODNUM`, `ANDNUM`, `ORNUM`, `XORNUM` | REPLACE | debugger expression/value service |
| `_` Refresh | `REFRESH`, `CLRPANEL` | REQUIRED | presentation refresh, no second core |

## Routine Families

- `MONITOR`, `MONMAIN`, `MONITOR1`, `PANENTRY`, `REGENTRY`, and `SINENTRY`
  implement monitor lifecycle, panel refresh, register display, and command
  dispatch. These map to one shared debugger session over the live machine.
- `DISPBP`, `DISPBPRAM`, `DISPBPPG`, `DISPIM`, `ZXINT1`, and `BYTLOOP` expose
  breakpoint, memory-page, interrupt, and register display state.
- `STOREREG`, `BACKREG`, `GETAF`, `SETREG`, `SETIFFS`, and `SETIMODE` comprise
  register snapshot, restore, mutation, and interrupt-mode workflows.
- `MEMPAGE`, `MEMSHAD`, `NOSETSHAD`, `NOSETROM`, and `SETMMUEND` comprise the
  48K/128K memory and ROM configuration workflows.
- `BREAKP`, `BPSUB*`, `JUMP`, `ISTEP`, `SSTRAP`, `SSTOP*`, and `DUMPSCR*`
  comprise breakpoint, jump, step, trap, stop, and screen-dump control.
- `EDITOR`, `SCANLINE`, `F1KEY`, `LETTER`, `ASCLOOP`, and `WAITKEY` comprise
  monitor command input and line-editing behavior; host UI input must be
  normalized before entering the shared command service.
- `DISS`, `DISS_*`, `INDREG`, `INDCB`, `OPC`, `BITS`, `RSTN`, `RP`, `TOKEN`,
  and numeric helpers comprise the disassembler and expression formatter.
- `COPYMEM`, `KILLMEM`, `FINDMEM`, `POKE`, `POKEWORD`, `SAVE`, `LOAD`, and
  `WRTBYTES` comprise bounded memory/data tools and must not bypass bus/state
  ownership in the replacement.
- `SETOUTPUT`, `OUTCLOSE`, `OUTERROR`, `PRTOUT`, and `PANELOUT` are host-facing
  output boundaries and are not canonical machine state.

## Disposition Rules

Required workflows receive a shared debugger API and regression coverage before
Phase-11 closure. Replace workflows preserve their observable diagnostic result
without copying DOS handles, host paths, or a second CPU/memory model. No
historical assembly label is treated as permission to expose host internals.

## Zero-Gap Reconciliation

The complete source label scan, help-table command list, architecture sections
41, 44, and 49.2, and UI architecture section 21 are the authorities for this
inventory. Any routine not mapped above must be added before CR-0241 closes.
