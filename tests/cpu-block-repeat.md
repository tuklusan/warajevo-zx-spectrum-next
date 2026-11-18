<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned block instruction repeat traces

The hosted acceptance exercises all 16 ED block transfer, search, input,
and output opcodes. The eight repeat forms execute a repeating and a
terminating iteration; the eight single forms execute their terminating
path. CPIR and CPDR also stop early on a match while the count remains.
For all 26 instruction steps, the test checks exact bus cycle type, tick,
address, byte, and duration against the emitted full timing trace, plus
PC, total timing, refresh R, Q, and transfer/input memory effects.

Hosted proof: [cpu-block-repeat.json](../test-results/cpu-block-repeat.json).
