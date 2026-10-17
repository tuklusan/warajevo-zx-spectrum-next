<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned CPU rotate and shift vectors

The hosted matrix checks every byte value and both incoming carry values for
RLC, RRC, RL, RR, SLA, SRA, undocumented SLL, and SRL across B, (HL),
DD CB displacement to B, FD CB displacement to B, and DD CB displacement
to memory. It also checks RLCA, RRCA, RLA, and RRA for every byte and carry.
All 22,528 cases compare result, flags including X/Y and parity, PC, refresh
R, Q, memory effect, and instruction duration against an independent model.

Hosted proof: [cpu-rotate-shift.json](../test-results/cpu-rotate-shift.json).
