<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Debugger breakpoint workflow

The production Debugger / Monitor exposes the shared machine breakpoint through
paused-only controls and the local-only `B <address>` / `B CLEAR` commands.
The window adapter reports the active address and hit state, stops bounded
execution before the instruction at the breakpoint, and preserves the machine
until the user clears the breakpoint. The contract covers open/running/paused/
closed gating, malformed commands, stop address and master tick, clear and
resume, and execution after clearing. CTest runs it on all five native targets.
