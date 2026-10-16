<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned CPU fetch and memory write acceptance

The hosted acceptance executes NOP followed by LD (9001h),A from RAM. It
checks both M1 fetches, two operand reads, and the final write with exact
master ticks, values, addresses, and durations. It also checks PC, refresh,
WZ/memptr, memory contents, and matching TIMING_FULL bus, instruction, and
opcode-byte records.

Hosted proof: [cpu-fetch-write.json](../test-results/cpu-fetch-write.json).
