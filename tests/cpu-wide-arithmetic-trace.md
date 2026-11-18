<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned 16-bit arithmetic trace acceptance

The hosted matrix checks ADD HL,BC, ADC HL,BC, SBC HL,BC, ADD IX,BC, and
ADD IY,BC over 360 edge and carry cases. Independent arithmetic predicts
the result and all flags. Every case also checks full CPU state, WZ/memptr,
PC, refresh, duration, exact M1 bus cycles, and matching `TIMING_FULL`
bus/opcode/instruction records.

Hosted proof: [cpu-wide-arithmetic-trace.json](../test-results/cpu-wide-arithmetic-trace.json).
