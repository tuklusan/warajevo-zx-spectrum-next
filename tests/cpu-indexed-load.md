<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned indexed displacement load acceptance

The hosted matrix covers all seven `LD r,(IX/IY+d)` targets, all seven
`LD (IX/IY+d),r` sources, and `LD (IX/IY+d),n` for both DD and FD prefixes.
Each operation crosses positive displacement, negative displacement with
16-bit address wrap to `FFFFh`, and the `-128` signed displacement edge.
The 90 cases independently check full CPU state, H/L register semantics,
effective address, WZ/memptr, memory effects, PC, R, exact bus cycles,
and matching `TIMING_FULL` bus/opcode/instruction records.

Hosted proof: [cpu-indexed-load.json](../test-results/cpu-indexed-load.json).
