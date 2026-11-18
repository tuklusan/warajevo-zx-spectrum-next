<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned CPU stack bus traces

The hosted acceptance covers CALL and RET for all eight conditions, both
taken and not taken, plus unconditional CALL/RET, all eight RST vectors,
and all four register-pair PUSH/POP operations. Five additional cases
exercise 16-bit stack-pointer wraparound. Each of the 55 cases checks bus
cycle type, tick, address, byte, and duration against the full timing trace,
plus SP, PC, memptr, total duration, refresh R, and stack bytes or popped pair.

Hosted proof: [cpu-stack-trace.json](../test-results/cpu-stack-trace.json).
