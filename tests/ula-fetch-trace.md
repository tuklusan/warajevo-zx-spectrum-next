<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# ULA fetch trace

The production trace records bitmap and attribute bytes fetched while
reconstructing screen memory during a timed 48K PAL write. The four-runner
contract verifies addresses, values, master ticks, and raster positions across
a bitmap/attribute boundary. It proves timed fetch-event emission for this
fixture; it does not establish a continuous full-frame capture or 128K coverage.
