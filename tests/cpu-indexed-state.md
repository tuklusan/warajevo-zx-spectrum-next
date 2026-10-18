<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned index register and stack acceptance

The hosted matrix covers DD and FD forms of index-half register and
immediate loads, 16-bit immediate and absolute index transfers, index
increment/decrement, indirect JP, SP transfer, POP/PUSH, and stack exchange.
The 32 independent cases check full CPU state, unaffected IX/IY counterpart,
memory and stack byte order, WZ/memptr, PC, R, duration, exact bus cycles,
and matching `TIMING_FULL` bus/opcode/instruction records.

Hosted proof: [cpu-indexed-state.json](../test-results/cpu-indexed-state.json).
