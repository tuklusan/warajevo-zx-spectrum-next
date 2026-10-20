<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Timing trace reconstruction across CPU, bus, interrupt, and ULA

The hosted full Release build compiles the Sokol application and runs this
CTest on Windows x64, Linux x64/ARM64, and macOS Intel/ARM64. The test executes
one full 48K PAL frame through the production runner with timing tracing
enabled, then writes and recovers the binary trace. It checks interrupt-line,
sample, and acceptance order; synchronized CPU registers at the interrupt
acknowledge bus event; CPU instruction events; and all 12,288 ULA fetches in
one monotonic sequence. The result is pinned in
[`timing-trace-bus-context.json`](../test-results/timing-trace-bus-context.json).
