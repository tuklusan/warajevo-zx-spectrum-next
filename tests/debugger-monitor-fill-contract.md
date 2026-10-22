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
selected memory view. The isolated contract covers pause/closed gating, exact
changed bytes, the `FFFF` endpoint, ROM and ROM/RAM-boundary rejection,
malformed arguments, an 8-bit fill value, and distinct 128K RAM banks through
the current mapping.

The expanded contract expects 35 assertions. Its 128K mapped-bank extension is
pending the next five-target hosted run. The earlier 21-assertion version
passed all 31 integrated CTests on all five native targets in exact-source
Release/Sokol run 39707597289 at commit
`1a61d9861e5818818cbd9f083e494eb330e18cd1`.

Proof and fixture hashes: [debugger-monitor-fill-contract.json](../test-results/debugger-monitor-fill-contract.json)
and [debugger-monitor-fill-contract.driver.json](../test-drivers/debugger-monitor-fill-contract.driver.json).
