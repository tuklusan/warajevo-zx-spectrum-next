<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial Use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Debug Sokol trace startup/shutdown contract

The hosted Debug Sokol host starts a production-format timing trace, runs one
deterministic frame under `WZSN_TRACE_TEST_FRAMES=1`, requests application quit,
and verifies clean process exit. The contract confines the trace to the
configured temporary directory, requires the fixed 16 MiB file size, checks the
frozen header marker, and requires a positive recovered record count.

The Linux x64 runner passed this contract in run `41749531429` at source
`e9ca443bf486a428b429b4257e87e67cdd85aec3`; the pinned result is
[`debug-trace-startup.json`](../test-results/debug-trace-startup.json).
