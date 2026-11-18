<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned undocumented ED NOP acceptance

The hosted matrix executes all 178 unassigned ED opcodes, each directly
and after DD, FD, and a mixed DD/FD prefix chain. The 712 cases assert
unchanged CPU state except PC and refresh, eight T-states per M1 fetch,
exact bus cycles, and one logical `TIMING_FULL` instruction record with
all opcode bytes. The expected behavior follows [The Undocumented Z80
Documented](https://z80.info/zip/z80-documented.pdf).

Hosted proof: [cpu-ed-undocumented-nop.json](../test-results/cpu-ed-undocumented-nop.json).
