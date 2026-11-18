<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned memory and indexed arithmetic trace acceptance

The hosted matrix checks all eight accumulator ALU operations against (HL),
DD/FD indexed memory, and immediate operands with both incoming carry values.
It also checks INC/DEC of (HL) and DD/FD indexed memory. The 76 independent
cases assert full CPU state, flags, memory effects, exact bus cycles and
`TIMING_FULL` bus/opcode/instruction records.

Hosted proof: [cpu-arithmetic-trace.json](../test-results/cpu-arithmetic-trace.json).
