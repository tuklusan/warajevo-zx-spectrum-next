<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Debugger monitor interrupt command

The production debugger accepts `! <IFF|-1> [IM]`. It sets both interrupt
flip-flops to 0 or 1 and optionally sets interrupt mode 0, 1, or 2. `-1`
preserves the flip-flops. The operation is paused-only, updates the displayed
CPU snapshot, and creates one undo state for the complete command.

The isolated monitor contract passed 28 assertions on Windows x64, Linux x64,
Linux ARM64, macOS ARM64, and macOS Intel in exact-source Release/Sokol run
39704779175 at commit `90f6c8252b5e581fdb9d7024b46a5ec3cc554b86`. The run passed
all 30 integrated CTests per target and the Phase-16 fingerprints and
throughput gate. This contract does not yet verify subsequent interrupt
delivery behavior.

Proof and fixture hashes: [debugger-monitor-command-contract.json](../test-results/debugger-monitor-command-contract.json)
and [debugger-monitor-command-contract.driver.json](../test-drivers/debugger-monitor-command-contract.driver.json).
