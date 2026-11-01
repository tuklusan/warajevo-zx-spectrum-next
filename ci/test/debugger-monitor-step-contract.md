<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Debugger monitor step command

Paused-only `I` executes one Z80 instruction through the debugger window.
`I <end>` requests normal frame execution to a temporary, mapping-aware stop
before the instruction at the target address. Persistent breakpoints remain
unchanged; an earlier persistent breakpoint or manual pause cancels the target.
The step-to-address path preserves normal interrupt sampling and refreshes the
selected target in the debugger after stopping.

The 51-assertion CTest contract checks one-instruction register/tick state,
step-to-address stopping, preservation of an existing breakpoint, target
cancellation, interrupt entry and stack state, command validation, and a
128K bank switch that must pass the same logical address in the wrong bank and
stop only when the original physical bank is mapped again. It runs on all five
native Release/Sokol targets. Proof:
[debugger-monitor-step-contract.json](../../test-results/debugger-monitor-step-contract.json).
