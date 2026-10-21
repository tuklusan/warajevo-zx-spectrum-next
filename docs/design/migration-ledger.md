<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Migration and provenance ledger

This ledger follows Core Architecture §§41 and 48. A missing row is not
clearance. An item marked `RESEARCH` is not authorization to translate or copy
the preserved source.

## Source authority and identity

| Field | Recorded value |
|---|---|
| Upstream repository | `https://github.com/tuklusan/warajevo-spectrum-2.50.git` |
| Pinned upstream identity | `94f69bd8f4acb6c0c320ae34f9b1c3ee29bc5545` |
| Imported location | `reference/original-warajevo/source/src/` |
| Import policy | Read-only reference; excluded from all product build targets. |
| Upstream license classification | The official Warajevo source page says the available source is 2.50 and the project was released under GPL in February 2006. Per-file headers and third-party notices still take precedence where present. |
| Project license compatibility | Unresolved repository-wide: the root `LICENSE` is a custom non-commercial license, while several source headers state GPL-2.0-or-later. No compatibility conclusion is implied by a routine row. |

Authority: [official source and license notice](https://worldofspectrum.net/warajevo/Download.html), [official revision history](https://worldofspectrum.net/warajevo/Revision.html). The official history says 2.52 was identical to 2.51 except for the license/source publication, and the official download page says a complete 2.51 source tree was not available.

## Record fields

Each record must identify the upstream repository/commit/blob, source file and
routine, purpose, copyright/license classification, migration type and
clearance, inputs/outputs, emulated and host state effects, timing, external
calls, error behavior, C destination, regression evidence, source comment, and
status. The record's evidence must be pinned to exact source and fixture
identities. `TRANSLATE` requires explicit license-compatible clearance;
uncertain source or license defaults to `RESEARCH` or `EXCLUDE`.

## Routine records

### Z80 NOP — reimplementation sample

| Field | Recorded value |
|---|---|
| Upstream identity | Warajevo 2.50 import from commit `94f69bd8f4acb6c0c320ae34f9b1c3ee29bc5545`; imported Git blob `4555e60aa55e92aa558a7a3adff98895eaaad3c7`; imported-file SHA-256 `20fcdfcd598d7cfbeff5ea01b31390c5874cca96f40e5f5ec418e2acddd8fe5b`. |
| Source routine | `reference/original-warajevo/source/src/spectrum-kernel/Z80.ASM`, label `NOP_`, lines 5207–5211. Its `RF 4` macro accounts for the legacy cycle accumulator and program-counter advance before instruction dispatch. |
| Original purpose | Execute the Z80 no-operation instruction and continue decoding at the following byte. |
| Original copyright/notice | The source file header names Zeljko Juric and Samir Ribic. The official source page classifies the published source as GPL; this row does not clear reuse of other file contents. |
| New material license | `src/core/wz_z80.c` carries a GPL-2.0-or-later source header. Repository-wide compatibility with the root `LICENSE` remains unresolved. |
| Migration type / clearance | `REIMPLEMENT`: the C branch operates on the portable CPU state and advances explicit master ticks; the x86 `RF` macro and register mapping were not translated. Clearance is limited to this routine's standard Z80 behavior and does not authorize copying other assembly. |
| Inputs | Fetched opcode, current program counter, CPU refresh register, and machine master time. |
| Outputs | Program counter advances one byte; refresh state advances through the shared instruction-entry path; master time advances eight half-T-state ticks (four Z80 T-states). |
| Emulated state read/written | Reads decoded opcode and current CPU state; writes PC, refresh register, and master tick. |
| Host state | None. |
| Timing consequences | Four Z80 T-states represented by eight master ticks; no wall-clock or host-width dependency. |
| External calls / error behavior | No NOP-specific external call. Invalid fetch/state errors are returned by the shared instruction-entry path before this branch. |
| C destination | `src/core/wz_z80.c`: `wz_z80_step_impl`, primary opcode decode and `WZ_Z80_PRIMARY_OP_NOP` branch. |
| Regression evidence | Pinned Fuse Z80 conformance passed all 1,356 cases on run `39172660308`, source `2a6e8350388dc69a4c757df7a7f833823c37b5b2`. The `src/core/wz_z80.c` implementation was byte-identical from that tested commit through `efc2cd06921cfd7d7181a734a32583a90d5d4bf3`; only this provenance comment is added by the present change. |
| Provenance comment | Added beside the NOP branch; it points to this ledger and the exact imported source label/revision. |
| Status | This sample is accounted for. The ledger is not a complete inventory of all migrated routines. |

### Interface 1 Microdrive sector stream — behavior reimplementation

| Field | Recorded value |
|---|---|
| Upstream identity | Warajevo 2.50 import from commit `94f69bd8f4acb6c0c320ae34f9b1c3ee29bc5545`; imported source file `MDRIVE.ASM` SHA-256 `a438796586ecc970efbfb63cee718854a3f8ce716318efd17565ffa17ab695f8`; imported Git blob `a823929a015943260a6b1c1b4faebb27afd805ed`. |
| Source routines | `reference/original-warajevo/source/src/spectrum-kernel/MDRIVE.ASM`: `MDV_IN`, `MDV_OUT`, `ZXI1_CTRL`, `NEXT`, `FLUSH`, and `INCSEC`. |
| Original purpose | Transfer Interface 1 Microdrive bytes from/to the selected cartridge, advancing from its 15-byte header to its 528-byte record block and then to the next sector. `FLUSH` skips each 12-byte synchronization preamble when copying into the stored 543-byte image sector. |
| Original copyright/notice | The source file header names Zeljko Juric and Samir Ribic. The official source page classifies the published source as GPL; this record does not clear reuse of source text. |
| New material license | `src/core/wz_microdrive.h` retains the project's existing source license. Repository-wide compatibility with the root `LICENSE` remains unresolved. |
| Migration type / clearance | `REIMPLEMENT`: only observable sector layout and stream ordering are represented in portable C; no assembly or source expression was copied. Clearance is limited to these behaviors and does not authorize translating other routines. |
| Inputs | Mounted MDR image bytes, selected motor, current sector and byte offset, and Interface 1 data-port reads. |
| Outputs | The stored 15-byte header followed by the 528-byte record block; after the block, position advances to the next sector and wraps at cartridge length. |
| Emulated state read/written | Reads image identity, sector count, selected motor, phase, and offset; updates the selected drive's phase/offset and sector. |
| Host state | Mounted image storage and persistence callbacks remain outside the machine-visible byte stream. |
| Timing consequences | This record covers byte ordering only. Synchronization pulse widths, gaps, erase timing, random seeks, and physical tape behavior are not claimed. |
| External calls / error behavior | Uses existing machine bus and MDR transport APIs; invalid/missing motor or image returns the existing error result. |
| C destination | `src/core/wz_microdrive.h` sector offsets and `src/core/wz_microdrive.c` read-position transitions. |
| Regression evidence | Exact-source run `39533151050` passed the differential contract and all 23 CTests on five native targets; Phase-16 passed all 30 checks with a 25.7% maximum slowdown. Proof: `test-results/warajevo-mdr-differential.json`; Phase-16: `test-results/phase16-performance-current.json`. Differential fixture: `ci/test/warajevo-mdr-differential/warajevo-mdr-differential.c`; driver: `test-drivers/warajevo-mdr-differential.driver.json`. |
| Provenance comment | This record and the differential fixture identify the preserved source revision, file digest, and exact behavior under comparison. |
| Conflict precedence | The preserved source and companion Interface 1/Microdrive spec agree on stored offsets and stream order. No physical-hardware byte/timing evidence is available; timing behavior is explicitly outside this differential test. |
| Status | Behavior reimplementation and hosted differential proof are recorded. This does not clear source reuse or close other Interface 1 timing criteria. |

### ZX Printer — function-level disposition inventory

Source identity: Warajevo 2.50 import from commit
`94f69bd8f4acb6c0c320ae34f9b1c3ee29bc5545`; source file
`reference/original-warajevo/source/src/spectrum-kernel/ZXPRINT.ASM`, Git blob
`d248fa59050235ddbf094f0d9881d30ba1cf6369`, SHA-256
`e96b015c3e4fda7f6ea42d593a6c70f07ccef9100eacc782c76c2de5d71404b5`.
The file header names Zeljko Juric and Samir Ribic; upstream is classified as
GPL by the official source notice. This inventory does not clear reuse.

| Historical routine | Original purpose / host boundary | Disposition | WZSN destination | Regression strategy |
|---|---|---|---|---|
| `ZXP_IN` (public entry) | Expose the ZX Printer status-port read to the Spectrum machine. | REQUIRED | `wz_machine_printer_read` and the printer branch in `wz_bus.c`; no x86 far call. | Read the emulated port in each mode; assert absent-device status, ready/status bits, address decode, and no printer-state mutation. |
| `ZXP_OUT` (public entry) | Expose the ZX Printer control-port write to the Spectrum machine. | REQUIRED | `wz_machine_printer_write` and the printer branch in `wz_bus.c`. | Write every relevant control-bit transition at explicit master ticks; check motor, pixel, row, flush, and state-hash behavior. |
| `Z_IN_FAR` | Compute printer status, mixing emulated port latches with BIOS LPT1 presence/status and the prior output latch. | REPLACE | `wz_printer_status` models the Spectrum-visible deterministic status; host printer discovery/status is removed from core state. | Cover all four configured printer modes and `NONE`, status-latch semantics, and absence of host-device dependence. |
| `Z_OUT_FAR` | Interpret motor/pixel/row control, expand Epson or HP output, and synchronously send control/data bytes to BIOS LPT1 while suspending/restoring the DOS timer. | REPLACE | `wz_printer_write_control` captures bounded Epson/HP flush events; the application-owned printer manager renders/exports them. DOS timer changes, BIOS calls, and LPT writes are excluded. | Compare per-mode pixel accumulation, row progression, motor transitions, flush ordering/ticks, bounded event data, and deterministic state across all required native targets. |
| `INT17` | Gate and invoke the BIOS printer interrupt used for host LPT output. | REPLACE | Completed output is delivered as a bounded core flush event and consumed by the host printer manager. | Verify machine state is independent of host printer availability and export failures; host export/path failures must not mutate the core. |
| `SETASCII` | Convert HP printer coordinates to ASCII and construct host-specific LaserJet command parameters. | DROP | No equivalent host-device command stream; the HP raster remains represented by the printer mode's pixel flush event and host bitmap projection. | Assert the HP output pixels and mode-specific bitmap dimensions; do not test BIOS/LPT command bytes. |

The direct port/status/pixel/flush contract passed 3,909 assertions on all five
native targets in hosted run `39646431461` at source
`2470446f9b8617905a787532ea7426d5f0fed79b`; proof:
`test-results/printer-core-contract.json`, implementation:
`ci/test/printer-core-contract.c`. Its source-review warning was adjudicated
non-actionable because `wz_printer_set_mode` initializes the state before
reading it. The named local labels in `Z_OUT_FAR` (`MOTON`, `MOTOFF`, `FLUSH`, `HPFLUSH`,
`HPLASER`, and related branch/loop labels) are control-flow blocks, not
independently callable routines. Their behavior is covered by the `Z_OUT_FAR`
regression strategy above. Existing printer settings evidence validates mode
selection only; the additional core contract supplies direct software proof
for the tested port/status/pixel/flush behavior. Legacy BIOS/LPT parity and
physical timing remain unclaimed. Per-file copyright/license
compatibility remains unresolved, so this is an inventory and disposition only,
not a license grant or source-derivation clearance.

### Built-in monitor/debugger — command-dispatch inventory

Source identity: Warajevo 2.50 import from commit
`94f69bd8f4acb6c0c320ae34f9b1c3ee29bc5545`; source file
`reference/original-warajevo/source/src/spectrum-kernel/SPECMON.ASM`, Git blob
`840fcc86680c7cb8018884c8735353c3f413588d`, SHA-256
`e107984f22e35d31e4d442192a6c70f07ccef9100eacc782c76c2de5d71404b5`.
The header names Zeljko Juric and Samir Ribic; upstream is classified as GPL
by the official source notice. This inventory does not clear reuse. The handler
names below are resolved from the file's `COMS`/`COMADDR` dispatch tables, not
inferred from user-interface labels. Conditional 48K/128K behavior is retained
as a profile-specific case where applicable.

| Command | Handler | Original purpose | Disposition | Regression strategy |
|---|---|---|---|---|
| `A` | `ABORT` | Abort to BASIC, optionally using a RAM top. | REPLACE | Verify the modern machine/reset or return workflow preserves the specified run/pause and device state. |
| `B` | `BREAKP` | Set/remove a breakpoint, including shadow-memory location. | REQUIRED | Assert breakpoint insertion/removal, saved byte restoration, profile/page mapping, and stop/resume behavior. |
| `C` | `COPYMEM` | Copy a memory range. | REQUIRED | Copy boundary/wrapping ranges through the shared memory editor and verify only the requested mapped bytes change. |
| `D` | `DISS` | Disassemble a memory range. | REQUIRED | Compare supported base/prefix/indexed instructions and address advancement against the Z80 decoder; reading must not mutate the machine. |
| `E` | `MONEXIT` | Execute/resume from the selected PC. | REQUIRED | Resume at the selected PC, preserve registers, and verify the debugger returns on the next stop. |
| `F` | `FINDMEM` | Search memory for byte/word patterns and wildcards. | REQUIRED | Cover exact/wildcard matches, range boundaries, no match, and read-only behavior. |
| `H` | `HEXMODE` | Toggle hexadecimal/decimal monitor display. | REPLACE | Verify modern debugger value formatting and parsing for both supported radices. |
| `I` | `ISTEP` | Single-step, optionally to an end address. | REQUIRED | Verify one-instruction stepping, step-to-address stop, interrupt behavior, and exact register/tick trace. |
| `J` | `JUMP` | Set a jump/execute address. | REQUIRED | Verify address selection and subsequent execute/resume state. |
| `K` | `KILLMEM` | Fill a memory range with a value. | REQUIRED | Cover range boundaries, value validation, mapped-bank selection, and exact changed-byte set. |
| `L` | `LOAD` | Load a memory block through DOS file handling. | LATER | If retained, test native file selection, address/range validation, short/read failure rollback, and resulting mapped memory. |
| `M` | `MWRITE` | Set the memory pointer and optionally write bytes/XOR-decoded bytes. | REQUIRED | Verify pointer navigation, byte sequence writes, wildcard skips, bank mapping, and bounded address behavior. |
| `N` | `IMADDR` | Inspect the interrupt handler/vector address. | REPLACE | Verify shared interrupt-state inspection reports IM 0/1/2 and the effective vector without mutating state. |
| `O` | `SETOUTPUT` | Redirect monitor text to a DOS output file. | REPLACE | If replaced by diagnostics export, verify captured text, path failure, and no machine-state mutation. |
| `P` | `POKE` | Write bytes at the current memory pointer. | REQUIRED | Verify byte writes and errors for absent/invalid arguments and mapped-memory boundaries. |
| `Q` | `MONQUIT` | Leave the monitor and return to the interrupted program. | REQUIRED | Close the debugger without changing the selected machine state or losing pending stop context. |
| `R` | `SETREG` | Edit the selected register value. | REQUIRED | Production debugger monitor now dispatches paused-only `R SETREG <register> <hex>` through the shared core/window register API. Case-insensitive names, strict token/hex validation, local-only availability, state refresh, and shared undo are covered by the integrated CTest and seven production GUI assertions. Exact-source five-target Release/Sokol build, CTest, and Phase-16 gate passed in run `39690861672` at binary-source commit `016025970299e5a2acc45933bbf727ef3a77ec8b`; GUI edit/undo passed in run `39694046580` at `3d043916093d127045310ab2942d62e7a8bc1469`. Proof and seven inspected, SHA-256-pinned captures: `test-results/gui-debugger-registers.json`. The later source commits only stabilized test input/capture timing and did not change product binaries. |
| `S` | `SAVE` | Save a memory block through DOS file handling, optionally with a header. | LATER | If retained, test native destination selection, exact byte/header output, no-clobber policy, and failed-write behavior. |
| `T` | `TABULATE` | Display a memory range as bytes/words. | REQUIRED | Verify addresses, byte/word formatting, page boundaries, and read-only behavior. |
| `U` | `UNDO` | Restore saved register state. | REQUIRED | Verify a register edit can be undone once, unrelated machine state is preserved, and empty undo is harmless. |
| `V` | `VIEWSCR` | Temporarily display the emulated screen. | REPLACE | Verify the debugger's live display view leaves raster and machine state unchanged. |
| `W` | `POKEWORD` | Write words at the current memory pointer. | REQUIRED | Verify low/high byte order, pointer advance, mapped-bank boundaries, and invalid input rejection. |
| `X` | `SETXOR` | Set the XOR byte used by monitor memory entry/decryption. | LATER | If retained, cover zero/default and nonzero key behavior, decoded writes, and wrong-width rejection. |
| `Y` | `MEMPAGE` | On 128K, inspect/set RAM, ROM, and video paging; on 48K, swap ROM. | REPLACE | Verify the shared hardware editor honors profile capability, paging lock, ROM/bank identity, and state-reset policy. |
| `Z` | `DUMPSCR` | Print the current screen through legacy printer output. | REPLACE | Verify screenshot capture contains the Spectrum raster only and performs no printer-device or machine-state mutation. |
| `!` | `SETIFFS` | Edit interrupt enable and interrupt mode. | REQUIRED | Cover IFF1/IFF2 and IM 0/1/2 mutation, invalid values, pause requirement, and subsequent interrupt behavior. |
| `?` | `CONVNUM` | Convert/display a number in supported forms. | REPLACE | Verify modern debugger numeric input/display accepts decimal, hexadecimal, and character forms with deterministic bounds. |
| `+` | `ADDNUM` | Add two numbers. | LATER | If retained as a monitor calculator, test signed/range behavior and exact displayed result. |
| `-` | `SUBNUM` | Subtract two numbers. | LATER | If retained as a monitor calculator, test signed/range behavior and exact displayed result. |
| `*` | `MULNUM` | Multiply two numbers. | LATER | If retained as a monitor calculator, test signed/range behavior and exact displayed result. |
| `/` | `DIVNUM` | Divide two numbers. | LATER | If retained as a monitor calculator, test quotient, divide-by-zero, and range behavior. |
| `%` | `MODNUM` | Compute a remainder. | LATER | If retained as a monitor calculator, test positive/negative operands, zero divisor, and range behavior. |
| `&` | `ANDNUM` | Compute bitwise AND. | LATER | If retained as a monitor calculator, test zero, sign-bit, and width-boundary operands. |
| `|` | `ORNUM` | Compute bitwise OR. | LATER | If retained as a monitor calculator, test zero, sign-bit, and width-boundary operands. |
| `^` | `XORNUM` | Compute bitwise XOR. | LATER | If retained as a monitor calculator, test equal/different and width-boundary operands. |
| `_` | `REFRESH` | Refresh the legacy display/panel. | DROP | No separate regression; modern presentation redraw is host-owned and cannot alter machine state. |
| `@` | `DEBUG` | Display the internal eight-byte debug zone (undocumented). | DROP | No separate regression; this legacy internal-memory diagnostic has no declared user-facing contract. |

The `MONITOR`/`MONMAIN` entry and loop, register save/restore path, far-call
wrappers, and breakpoint patch helpers are infrastructure for these command
handlers; they are not separate user commands. The table is a source-dispatch
inventory and records intended disposition, not implementation acceptance.
The standalone [debugger core contract](../../tests/debugger-core-contract.md)
directly verifies a subset of shared inspection, disassembly, memory editing,
CPU undo, stepping, breakpoints, and 128K paging behavior on five native
targets, including byte-exact and masked wildcard sequence search. It does not
complete the per-row regression strategies above; mapped copy uses logical
banked reads/writes and preserves overlap semantics while linear 48K copy keeps
the `memmove` path.
monitor/debugger migration remains open pending detailed source-level semantics
review, full direct acceptance proof, and any required design decisions.

## Explicit gaps

- Detailed monitor/debugger routine-level records and direct regression proof
  remain incomplete beyond this command-dispatch inventory; routine-level
  coverage also remains open for other migrated Z80, ULA, media, peripheral,
  machine, and host/UI areas.
- Every source file containing third-party material must be individually
  excluded or cleared before any reuse. The preserved source includes files
  with Borland and TurboPower notices as well as a converter naming a separate
  author.
- Repository-wide license compatibility and missing/legacy notice references
  require owner-level resolution before any claim that the complete project is
  cleared for distribution.
- No entry may be upgraded to `TRANSLATE` based only on matching behavior or
  on the upstream repository's top-level GPL statement.
