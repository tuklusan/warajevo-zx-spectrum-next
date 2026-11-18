<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Timing trace diagnostics contract

This independent hosted contract uses the production trace dumper and a
production-format 48K bus trace. It verifies the decoded Z80 bus cycle and
contention delay, the bus-request and delayed-access raster coordinates, and
the active ULA fetch position for each timestamp. It supplements the standalone
CPU-state dump contract without modifying its harness.
