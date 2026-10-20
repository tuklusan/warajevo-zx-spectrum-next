<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# ULA fetch trace

The production trace records bitmap and attribute bytes fetched while
reconstructing screen memory during a timed 48K PAL write. The four-runner
contract verifies addresses, values, master ticks, and raster positions across
a bitmap/attribute boundary. It proves timed fetch-event emission for this
fixture; it does not establish a continuous full-frame capture or 128K coverage.
