<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned primary register increment acceptance

The hosted matrix checks INC/DEC of all seven 8-bit registers over five
edge values and both incoming carry values, plus INC/DEC of BC, DE, HL,
and SP over three 16-bit edge values. The 164 independent cases assert
full CPU state, all affected and preserved flags, duration, exact M1 bus
cycles, and matching `TIMING_FULL` bus/opcode/instruction records.

Hosted proof: [cpu-primary-increment.json](../test-results/cpu-primary-increment.json).
