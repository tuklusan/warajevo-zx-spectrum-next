<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned 8-bit ALU flag acceptance

The hosted test runs every accumulator/operand pair with both carry inputs
for immediate ADD, ADC, SUB, SBC, AND, XOR, OR, and CP: 1,048,576 cases. An
independent arithmetic reference computes sign, zero, half carry, signed
overflow/parity, subtract, carry, and undocumented X/Y bits; CP takes X/Y
from the operand. It also checks every accumulator value and both carry inputs
for INC and DEC, including carry preservation: 1,024 cases. Each case checks
the result, all flag bits, PC, and T-state length. Current-source Fuse
conformance independently checks the suite's ALU cases and bus transfers.

Hosted proof: [cpu-alu-flags.json](../test-results/cpu-alu-flags.json).
