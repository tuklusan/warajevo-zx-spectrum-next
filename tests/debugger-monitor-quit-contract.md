<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Debugger monitor quit command

The production debugger accepts the no-argument `Q` command. It switches
mutation access back to read-only and closes the debugger window while leaving
the emulated CPU, memory, clock, paging, and pending breakpoint context intact.
The command contract checks strict arguments, closed-window behavior, pause
state, and reopening without losing the breakpoint stop.

The 16-assertion contract passed on Windows x64, Linux x64/ARM64, and macOS
ARM64/Intel in exact-source Release/Sokol run `39716631366` at commit
`50a23d6c8b576da009d280e3fd89691d76cbcd2b`. All 33 integrated CTests and all
five Phase-16 gates passed; the maximum measured slowdown was 30.6%.

Proof and fixture hashes: [debugger-monitor-quit-contract.json](../test-results/debugger-monitor-quit-contract.json)
and [debugger-monitor-quit-contract.driver.json](../test-drivers/debugger-monitor-quit-contract.driver.json).
