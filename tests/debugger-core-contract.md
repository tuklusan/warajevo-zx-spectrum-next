<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Debugger core contract

The hosted Release/Sokol build runs `wz_debugger_core_contract` in integrated
CTest on Windows x64, Linux x64/ARM64, and macOS Intel/ARM64. It passed 241
assertions and all 28 CTests on each target in run `39659158070` at source
`647567ed1c96ab4b02ad6a14f78f1e9b75dcf67c`. The same exact-source run passed
all configured Phase-16 correctness fingerprints and throughput checks on all
five targets; maximum slowdown was 30.6%.

Coverage includes snapshots, memory inspection and exact/masked search,
base/CB disassembly, bounded formatting, pause-gated byte/word/fill/copy edits,
linear and bank-mapped overlap behavior, full CPU-state edit/undo, individual
primary/alternate/index/stack/program/interrupt register edits, width and range
rejection, pause gating, one-step undo, jump, step, continue, breakpoints, and
128K page selection and lock behavior. The proof and fixture hashes are in
[test-results/debugger-core-contract.json](../test-results/debugger-core-contract.json)
and [test-drivers/debugger-core-contract.driver.json](../test-drivers/debugger-core-contract.driver.json).

This contract directly covers shared debugger-core behavior. Full monitor
command equivalence, register editing through the production debugger UI,
remaining mapped-bank boundary cases, interrupt stepping, remaining monitor
routines, and third-party clearance stay open.
