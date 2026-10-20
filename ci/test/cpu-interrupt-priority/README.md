<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# NMI and pending maskable interrupt priority

This isolated production-core regression asserts the 48K maskable interrupt
line, accepts an NMI first, verifies a maskable sample is rejected while IFF1
is cleared, executes RETN, then accepts the still-active maskable interrupt.
It checks canonical CPU state, exact bus cycles and master ticks, interrupt
trace order, and stack contents. The pre-existing HALT/interrupt harness is not
modified.
