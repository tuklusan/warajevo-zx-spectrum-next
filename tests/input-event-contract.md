<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Host input-event acceptance

The production host routes Sokol key edges through the shared
`wz_input_event` adapter. The contract checks invalid host codes, mapped
Spectrum-key projection, overlapping host aliases with reference counts,
Telnet/local source ownership, focus-loss release, and independent Kempston
mapping. The adapter remains host-side; only normalized matrix and joystick
state is projected into the machine.

CTest name: `wz_input_event_contract`.
