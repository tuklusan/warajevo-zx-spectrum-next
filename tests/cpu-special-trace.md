<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned special instruction trace acceptance

The hosted matrix checks RLCA, RRCA, RLA, RRA, DAA, CPL, SCF, and CCF over
four accumulator values, four flag patterns, and both prior-Q states. The
256 independent cases assert full CPU state, all flags, exact M1 bus cycles,
and matching `TIMING_FULL` bus/opcode/instruction records.

Hosted proof: [cpu-special-trace.json](../test-results/cpu-special-trace.json).
