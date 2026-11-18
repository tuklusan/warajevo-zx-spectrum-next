<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned CPU fetch and memory write acceptance

The hosted acceptance executes NOP followed by LD (9001h),A from RAM. It
checks both M1 fetches, two operand reads, and the final write with exact
master ticks, values, addresses, and durations. It also checks PC, refresh,
WZ/memptr, memory contents, and matching TIMING_FULL bus, instruction, and
opcode-byte records.

Hosted proof: [cpu-fetch-write.json](../test-results/cpu-fetch-write.json).
