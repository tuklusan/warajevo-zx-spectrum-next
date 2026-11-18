<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Interface 1 timing-trace contract

The production timing trace records decoded Interface 1 I/O transactions on
their canonical master ticks. Each record preserves the full port address,
read/write direction, observed or transmitted byte, current control latch,
motor shift register, and active drive. The five-case fixture covers mirrored
EF latch writes, an E7 Microdrive byte read, and F7 network-port reads/writes.
It verifies deterministic software bus events; it does not claim physical
waveform or hardware-timing accuracy.
