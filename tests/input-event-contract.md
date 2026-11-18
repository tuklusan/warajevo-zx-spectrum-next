<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
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
