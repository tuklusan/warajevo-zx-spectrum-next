<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Debugger monitor interrupt contract

This production-path contract dispatches `! 1 0`, `! 1 1`, and `! 1 2` while
paused, resumes the shared machine, and samples the asserted maskable interrupt.
It verifies that the selected interrupt mode controls delivery, vector/stack
state, IFF clearing, and exact master-tick cost for IM 0, IM 1, and IM 2.
The timing trace also records one maskable-interrupt sample followed by one
accept event for each mode.
