<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Debugger breakpoint workflow

The production Debugger / Monitor exposes the shared machine breakpoint through
paused-only controls and the local-only `B <address>` / `B CLEAR` commands.
The core binds a breakpoint to the physical page currently mapped at that
logical address. It stops before executing the target instruction, without
advancing the master tick or changing program bytes. Mapping bank 7 at C000 and
selecting it for the shadow display does not trigger a breakpoint bound to bank
6; restoring bank 6 triggers it again. The contract covers
open/running/paused/closed gating, malformed commands, stop address and master
tick, byte preservation, clear and resume, and execution after clearing. CTest
runs it on all five native targets.
