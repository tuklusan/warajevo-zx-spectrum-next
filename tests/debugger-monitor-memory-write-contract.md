<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Debugger monitor memory write command

The production debugger accepts `M <address> [bytes...]` to set the current
memory pointer and optionally write up to sixteen bytes. Exact `-1` entries
skip writes. All arguments and the complete nonwrapping writable range are
validated before writes; successful writes advance the pointer and refresh the
view. The contract covers navigation while running, paused-only writes, malformed
arguments, unchanged state on rejection, the 16-bit ceiling, ROM/RAM crossing,
and independent mapped 128K banks.

The 25-assertion contract passed on Windows x64, Linux x64/ARM64, and macOS
ARM64/Intel in exact-source Release/Sokol run `39719951673` at commit
`6484511a109e4197e30747f07906ad7925a1e9eb`. All 34 integrated CTests and all
five Phase-16 gates passed; the maximum measured slowdown was 29.0%.

Proof and fixture hashes: [debugger-monitor-memory-write-contract.json](../test-results/debugger-monitor-memory-write-contract.json)
and [debugger-monitor-memory-write-contract.driver.json](../test-drivers/debugger-monitor-memory-write-contract.driver.json).
