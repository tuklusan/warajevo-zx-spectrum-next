<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned ED control acceptance

The hosted acceptance covers all eight NEG encodings over seven edge
operands; all eight IM encodings; LD I,A, LD R,A, LD A,I, and LD A,R with
both IFF2 states; all seven RETN encodings and RETI with both IFF2 states;
and RRD/RLD over three accumulator/memory pairs. The 94 cases independently
check full CPU state, flags including undocumented X/Y and parity, refresh
wrap, memory effects, duration, exact bus cycles, and matching
`TIMING_FULL` bus/opcode/instruction records.

Hosted proof: [cpu-ed-control.json](../test-results/cpu-ed-control.json).
