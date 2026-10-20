<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# NMI and pending maskable interrupt priority

The project-owned core case asserts the 48K maskable interrupt line, accepts
NMI first, verifies a maskable sample is rejected while IFF1 is clear, executes
RETN, then accepts the still-active maskable interrupt. It checks final CPU
and stack state, exact bus cycles and master ticks, bus-to-TIMING_FULL trace
agreement, and interrupt-event order.

Hosted result: [cpu-interrupt-priority.json](../test-results/cpu-interrupt-priority.json).
