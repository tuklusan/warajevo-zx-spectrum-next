<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned indexed register arithmetic acceptance

The hosted matrix checks all eight accumulator ALU operations with IXH,
IXL, IYH, and IYL operands and both carry inputs. It also checks INC/DEC
of each register half with both carry inputs. The 80 independent cases
assert full CPU state, flags, PC, refresh, duration, exact M1 bus cycles,
and matching `TIMING_FULL` bus/opcode/instruction records.

Hosted proof: [cpu-index-half-arithmetic.json](../test-results/cpu-index-half-arithmetic.json).
