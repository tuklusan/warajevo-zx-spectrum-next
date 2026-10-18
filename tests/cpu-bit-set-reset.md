<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned BIT, RES, and SET acceptance

The hosted test checks all eight bit positions, all 256 operand values, both
incoming carry values, and five targets: B, (HL), positive DD CB displacement
with register copy, negative FD CB displacement with register copy, and DD CB
memory only. Its 61,440 cases independently compute the result and flags,
including carry preservation, BIT X/Y from register value versus WZ/address
high byte, and unchanged flags for RES/SET. Each case checks PC, refresh R,
Q, memory/register effects, duration, every bus cycle, and matching
`TIMING_FULL` bus/opcode/instruction records.

Hosted proof: [cpu-bit-set-reset.json](../test-results/cpu-bit-set-reset.json).
