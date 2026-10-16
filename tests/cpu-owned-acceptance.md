<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned CPU acceptance

The hosted case executes LD A,(9000h) from uncontended RAM on a 48K PAL
machine. It checks the exact four externally visible bus cycles: M1 fetch,
two operand reads, and the data read, with start ticks 0, 8, 14, and 20;
addresses, bytes, T-state durations, no contention, final registers, and
26 master ticks. It also confirms the TIMING_FULL trace contains the same
bus operations, the opcode byte, instruction boundary, and five full state
sync chunks in strictly increasing sequence order.

This establishes one project-owned exact bus-cycle case. The remaining
opcode, flag, repeat, stack, branch, HALT, and interrupt acceptance
categories in Tasks 50–59 and 61–62 still require local vectors and traces.

Hosted proof: [cpu-owned-acceptance.json](../test-results/cpu-owned-acceptance.json).
