<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned CB bit alias trace acceptance

The hosted matrix checks BIT, RES, and SET at every bit position and target
across primary CB, DD CB, and FD CB forms. Five edge values and both carry
inputs produce 5,760 cases. Each case checks full CPU state, flag source,
register-copy and memory effects, exact bus cycles, and matching
`TIMING_FULL` bus/opcode/instruction records.

Hosted proof: [cpu-cb-bit-alias-trace.json](../test-results/cpu-cb-bit-alias-trace.json).
