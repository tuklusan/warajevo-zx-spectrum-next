<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Debugger monitor copy command

The production monitor accepts paused-only `C <source> <destination> <length>`
commands, with hexadecimal logical addresses and a decimal byte count. It copies
from the active mapped memory view, rejects invalid or unwritable destination
ranges before mutation, preserves overlapping source bytes, and selects the
destination in the shared memory editor after success. The contract covers
pause and open gating, strict arguments, zero-length operations, overlapping
copies, atomic rejection of source or destination ranges that wrap past `FFFF`,
valid copies ending at `FFFF`, ROM protection, and independent 128K RAM banks.

The source-push workflow builds the full Release/Sokol application and runs the
contract through CTest on all five native targets. The matching driver manifest
pins the contract source and production build inputs.
