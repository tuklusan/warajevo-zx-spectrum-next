<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
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
