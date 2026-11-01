<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Debugger monitor tabulate command

`T [start [end [page-rows]]]` renders an eight-byte memory table with an
ASCII column. Addresses are inclusive, the selected memory pointer is the
default start, and each display page is limited to 24 rows. Page size zero or
`-1` selects 24 rows; values 1 through 24 select that many rows. The final row
is clipped at the requested end or the 16-bit address-space boundary. The
table uses the active logical memory mapping, performs no machine mutation,
and remains available while the machine is running.

The isolated contract checks byte/ASCII formatting, default and explicit
ranges, page limits, end clipping, malformed input, current-pointer behavior,
running-state inspection, and unchanged CPU, memory, and master-tick state.
The full Release/Sokol build and integrated CTest must pass on all five native
targets.
