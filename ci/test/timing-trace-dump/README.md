<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Timing trace dump contract

This independent hosted contract creates a production-format trace and runs
the built `wz_trace_dump` executable against it. It checks event names and raw
event fields, preserved same-tick sequence order, the complete reconstructed
main and alternate Z80 register sets, special registers, interrupt state, and
the recovered-record count. It also recovers a typed border transition with
its old/new colors and raster position. It does not modify the existing
trace-file test.
