<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned CPU trace synchronization

The hosted acceptance executes 5,000 HALT M1 fetches with a full timing
trace attached. It checks every bus/opcode-byte record, monotonic same-tick
sequence, one initial instruction boundary, and two complete five-chunk
absolute register-state groups. The periodic group must report HALT state.
This catches loss of absolute synchronization during prolonged HALT after
a circular trace wraps.

Hosted proof: [cpu-trace-sync.json](../test-results/cpu-trace-sync.json).
