<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Debugger monitor jump command

Paused-only `J` decodes the instruction at the current PC and resumes through
the normal frame runner until its sequential successor is reached. This steps
over a called routine and stops before executing the successor. `J <address>`
uses the same runner to stop before an explicit logical address.

Both forms bind the temporary stop to the physical page mapped at command
dispatch, preserve persistent breakpoints, and refresh the open debugger when
paused. Manual pause or an earlier persistent breakpoint cancels the temporary
stop.

The 63-assertion CTest contract covers instruction lengths across base, ED,
IX/IY, indexed-memory, indexed-CB, and repeated-prefix forms; call/return
stepping; exact PC/register/stack/tick state; explicit targets; address wrap;
48K/128K mapping identity; breakpoint preservation; cancellation; pause gating;
and malformed commands.
The required hosted gate is the full Release/Sokol build and integrated CTest on
Windows x64, Linux x64/ARM64, and macOS ARM64/Intel.
