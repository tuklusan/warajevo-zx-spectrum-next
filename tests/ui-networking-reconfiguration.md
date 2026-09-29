<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# UI networking reconfiguration contract

The 41-case hosted contract checks Interface-1 and None transitions through the
shared Networking command. It verifies that the application pause state stays
unchanged while CPU, RAM, tape, input, bus hooks, and other machine state reset;
the active profile and loaded Interface-1 ROM remain available. Both paused and
running states passed on run `36606295113`, source
`84244d7c22a54d62a9ad58471cd406f4300163d5`; proof is pinned in
[`ui-networking-reconfiguration.json`](../test-results/ui-networking-reconfiguration.json).