<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Debugger monitor byte poke command

The production debugger accepts paused-only `P <byte...>` with one to sixteen
hexadecimal byte values. Exact `-1` entries skip writes while still advancing
the memory pointer. The command validates every argument and the entire
nonwrapping writable range before mutation, then refreshes the selected memory
view. The contract covers pause/closed gating, multi-byte and skipped writes,
missing and malformed arguments, pointer advancement, end-of-address-space
rejection, ROM/RAM crossing rejection, and independent mapped 128K banks.

The 34-assertion contract passed on Windows x64, Linux x64/ARM64, and macOS
ARM64/Intel in exact-source Release/Sokol run `39714414602` at commit
`dd10a17f67603aea0050e95a5f5b7d06d6776684`. All 32 integrated CTests and all
five Phase-16 gates passed; the maximum measured slowdown was 35.1%.

Proof and fixture hashes: [debugger-monitor-poke-contract.json](../test-results/debugger-monitor-poke-contract.json)
and [debugger-monitor-poke-contract.driver.json](../test-drivers/debugger-monitor-poke-contract.driver.json).
