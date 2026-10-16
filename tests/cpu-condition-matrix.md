<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned JR and JP condition matrix

The hosted matrix runs each of NZ, Z, NC, and C through taken and not-taken
JR paths, and NZ, Z, NC, C, PO, PE, P, and M through taken and not-taken JP
paths. It also covers forward and backward unconditional JR, absolute JP,
and JP (HL): 28 cases total. Every case checks PC, WZ/memptr, flags,
refresh count, master ticks, bus cycle type/address/value/duration, and
matching TIMING_FULL bus/opcode/instruction records.

Together with [cpu-branch-trace](cpu-branch-trace.md), this closes the
project-owned conditional JR/JP/DJNZ path matrix in Task 55.

Hosted proof: [cpu-condition-matrix.json](../test-results/cpu-condition-matrix.json).
