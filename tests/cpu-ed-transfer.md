<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned ED 16-bit transfer acceptance

The hosted test covers all four register pairs in both `LD (nn),rr` and
`LD rr,(nn)` directions at a middle RAM address and the upper 64 KiB
boundary. The 32 cases cross two flag states and independently check
low/high byte order, the selected register pair, preserved flags and other
state, WZ/memptr, PC, R, Q, duration, every bus cycle, and matching
`TIMING_FULL` bus/opcode/instruction records.

Hosted proof: [cpu-ed-transfer.json](../test-results/cpu-ed-transfer.json).
