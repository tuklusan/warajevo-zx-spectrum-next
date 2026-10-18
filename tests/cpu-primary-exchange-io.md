<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned primary exchange and immediate I/O acceptance

The hosted acceptance covers `EX AF,AF'`, `EXX`, `EX DE,HL`, and
`EX (SP),HL`, plus immediate-port `OUT (n),A` and `IN A,(n)` across three
accumulator/input values and two flag states. The 16 independent cases
check full main/alternate CPU state, stack byte order, preserved flags,
WZ/memptr, PC, R, duration, exact bus cycles, and matching `TIMING_FULL`
bus/opcode/instruction records.

Hosted proof: [cpu-primary-exchange-io.json](../test-results/cpu-primary-exchange-io.json).
