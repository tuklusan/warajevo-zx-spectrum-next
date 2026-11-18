<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned CB rotate and shift trace acceptance

The hosted matrix checks all eight CB rotate and shift operations, all
eight targets, and primary CB, DD CB, and FD CB forms. Five edge values
and both carry inputs produce 1,920 cases. Each case checks full CPU
state, flags, register-copy and memory effects, exact bus cycles, and
matching `TIMING_FULL` bus/opcode/instruction records.

Hosted proof: [cpu-cb-rotate-trace.json](../test-results/cpu-cb-rotate-trace.json).
