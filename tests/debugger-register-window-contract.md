<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Debugger register window contract

The hosted Release/Sokol build runs `wz_debugger_register_window_contract`
through integrated CTest on Windows x64, Linux x64/ARM64, and macOS Intel/ARM64.
It passed on all five targets in run `39661406687` at source
`93cad5fa9eca608e19e728af2840be54928e4f66`; all 29 CTests and configured
Phase-16 fingerprints and throughput checks passed per target.

The contract verifies that the debugger window model rejects edits while the
machine is running, applies paused register edits to the CPU and refreshed
snapshot, preserves one-step undo, rejects invalid interrupt values without
state changes, and rejects edits after close. The reported repeated close in
test cleanup is safe because close only clears window state and borrowed
pointers; it owns no allocations.

Proof and source fixture hashes:
[test-results/debugger-register-window-contract.json](../test-results/debugger-register-window-contract.json)
and [test-drivers/debugger-register-window-contract.driver.json](../test-drivers/debugger-register-window-contract.driver.json).
