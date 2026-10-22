<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Debugger monitor undo command

The production debugger accepts the no-argument `U` command while paused. It
restores the prior CPU register snapshot through the debugger undo path without
changing memory or master tick. An empty undo history is harmless. The isolated
14-assertion contract checks running and closed-window rejection, invalid
arguments, register restoration, unchanged memory and tick, and repeated undo
with an empty history.

The contract passed on Windows x64, Linux x64/ARM64, and macOS ARM64/Intel in
exact-source Release/Sokol run `39721466664` at commit
`1313e894d5a7e0a332df54edef0c87f585f421a5`. All 35 integrated CTests passed
per target.

Proof and fixture hashes: [debugger-monitor-undo-contract.json](../test-results/debugger-monitor-undo-contract.json)
and [debugger-monitor-undo-contract.driver.json](../test-drivers/debugger-monitor-undo-contract.driver.json).
