<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Debugger monitor word-write command

The production debugger accepts paused-only `W <word...>` commands. Each
hexadecimal word is stored low byte first at the current memory pointer, then
the pointer advances by two bytes per word. The command accepts at most 16
words and rejects malformed input or writes that cross the address-space or
writable-memory boundary before changing memory. The isolated contract checks
little-endian writes, pointer advancement, running-state rejection, malformed
and over-capacity input, address-space limits, atomic ROM-boundary rejection,
and independent 128K banks.

The source-push workflow builds the full Release/Sokol application and runs the
contract through CTest on all five native targets. The matching driver manifest
pins the contract source and production build inputs.
