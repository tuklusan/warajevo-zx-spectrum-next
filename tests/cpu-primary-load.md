<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned primary load acceptance

The hosted matrix covers all 63 register/memory `LD r,r'` forms other than
HALT, all eight 8-bit immediate targets, all four 16-bit immediate pairs,
all four `LD A,(BC/DE)` and `LD (BC/DE),A` forms, and four absolute
`LD (nn),HL/A` and `LD HL/A,(nn)` forms. Its 83 independent cases check
full CPU state, memory effects, WZ/memptr, PC, R, duration, exact bus
cycles, and matching `TIMING_FULL` bus/opcode/instruction records.

Hosted proof: [cpu-primary-load.json](../test-results/cpu-primary-load.json).
