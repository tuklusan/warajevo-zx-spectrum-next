<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Timing trace CPU context on bus events

The hosted full Release build compiles the Sokol application and runs this
CTest on Windows x64, Linux x64/ARM64, and macOS Intel/ARM64. The test submits
a real production bus read while timing tracing is attached, then verifies
that its trace record preserves the active PC, SP, main register bank, bus
address, and returned memory value. The result is pinned in
[`timing-trace-bus-context.json`](../test-results/timing-trace-bus-context.json).
