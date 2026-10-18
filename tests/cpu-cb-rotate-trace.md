<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned CB rotate and shift trace acceptance

The hosted matrix checks all eight CB rotate and shift operations, all
eight targets, and primary CB, DD CB, and FD CB forms. Five edge values
and both carry inputs produce 1,920 cases. Each case checks full CPU
state, flags, register-copy and memory effects, exact bus cycles, and
matching `TIMING_FULL` bus/opcode/instruction records.

Hosted proof: [cpu-cb-rotate-trace.json](../test-results/cpu-cb-rotate-trace.json).
