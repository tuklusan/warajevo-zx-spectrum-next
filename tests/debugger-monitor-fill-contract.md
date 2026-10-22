<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Debugger monitor memory fill command

The production debugger accepts paused-only `K <first> <last> <byte>` with
hexadecimal, inclusive, nonwrapping 16-bit bounds. It validates the complete
command and the full writable range before changing memory, then refreshes the
selected memory view. The isolated contract covers pause/closed gating,
exact changed bytes, the `FFFF` endpoint, ROM and ROM/RAM-boundary rejection,
malformed arguments, and an 8-bit fill value.

The five-target source-push run 39707597289 passed all 31 integrated CTests
with the Release/Sokol host enabled at commit
`1a61d9861e5818818cbd9f083e494eb330e18cd1`. The 21 assertions passed on all
five native jobs in dispatch run 39707820446; proof records were generated in
run 39708600447. The full native build and Phase-16 gate passed on all five
targets. This contract covers the 48K profile only; 128K mapped-bank selection
remains unverified.

Proof and fixture hashes: [debugger-monitor-fill-contract.json](../test-results/debugger-monitor-fill-contract.json)
and [debugger-monitor-fill-contract.driver.json](../test-drivers/debugger-monitor-fill-contract.driver.json).
