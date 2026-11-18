<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Interface 1 bus trace fixture

The fixture loads a synthetic Interface 1 ROM and MDR image, selects drive 1,
and exercises mirrored EF control, E7 Microdrive data, and F7 network-port
cycles. The production trace file and CLI must preserve each port transaction's
master tick, observed byte, control latch, motor shift register, and selected
drive. The test does not claim physical waveform or hardware timing accuracy.
