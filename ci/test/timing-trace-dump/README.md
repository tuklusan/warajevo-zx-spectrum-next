<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Timing trace dump contract

This independent hosted contract creates a production-format trace and runs
the built `wz_trace_dump` executable against it. It checks event names and raw
event fields, preserved same-tick sequence order, the complete reconstructed
main and alternate Z80 register sets, special registers, interrupt state, and
the recovered-record count. It does not modify the existing trace-file test.
