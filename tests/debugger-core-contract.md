<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Debugger core contract

The hosted Release/Sokol build runs `wz_debugger_core_contract` in integrated
CTest on Windows x64, Linux x64/ARM64, and macOS Intel/ARM64. It checks 64
assertions per target for live snapshots, read-only memory inspection and
search including masked wildcard and word patterns, bounded reads, base and
CB-prefixed disassembly, hexadecimal formatting, pause-gated
byte/word/fill/copy mutations, overlap-safe copying, CPU state edit and one-shot
undo, jump, single-step, continue, breakpoint stop/clear, and 128K paging
inspection, bank selection, and paging lock behavior. Copy checks cover
overlapping linear and bank-mapped regions. All five builds, 99 assertions and
28 CTests per target, and configured Phase-16 checks passed in run
`39654672838` at source `e2f6c7293bedb9c22eb99cd8adfc3622e0e16a5a`.

This contract directly covers shared debugger-core behavior only. Full monitor
command equivalence, wildcard memory entry, mapped-bank boundary behavior,
interrupt stepping, full register-edit UI paths, and the remaining per-command
migration strategies stay open.
