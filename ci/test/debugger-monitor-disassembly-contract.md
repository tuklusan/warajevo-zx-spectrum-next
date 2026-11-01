<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Debugger monitor disassembly command

The monitor accepts read-only `D [start [end]]` commands. Addresses are
hexadecimal logical addresses; `end` is inclusive. It displays up to 24
instructions from the active memory mapping and includes each decoded
instruction's start address. The production Z80 decoder supplies instruction
length and text, including CB and indexed prefixes; an instruction may fetch
its trailing bytes across `FFFF` with 16-bit address wrap. The contract checks
base, CB, IX/IY disassembly, inclusive range advancement, the 24-line limit,
boundary behavior, strict arguments, closed-window gating, and unchanged CPU
and machine time.

The source-push workflow builds the full Release/Sokol application and runs the
contract through CTest on all five native targets. The matching driver manifest
pins the contract source and production build inputs.

The latest hosted result is [debugger-monitor-disassembly-contract.json](../../test-results/debugger-monitor-disassembly-contract.json).
