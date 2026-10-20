<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# ZX Net timing-trace contract

The production trace records accepted read-block delivery, read collection,
claim state transitions, every collision-read bit and collision-write bit,
and successful block commit. Records preserve canonical master ticks, prior
and current states and block IDs, framing bit count, and buffer position; no
block payload bytes are emitted. The 24-case five-runner contract exercises
the production core APIs and verifies event count and representative events
through the standalone trace dumper. It proves deterministic software trace
behavior, not physical waveform or hardware timing accuracy.
