<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned ED single-port I/O acceptance

The hosted acceptance covers all eight ED `IN r,(C)` and eight `OUT (C),r`
targets, including undocumented `IN (C)` and `OUT (C),0`. The 80 input
cases cross five values around zero, parity, X/Y, and sign with both carry
states; 16 output cases cross all targets and carry states. An independent
reference checks every register, all flags, WZ/memptr, PC, R, Q, duration,
the exact I/O address/value/tick, and the full bus/opcode/instruction trace.

Hosted proof: [cpu-ed-io.json](../test-results/cpu-ed-io.json).
