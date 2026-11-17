<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Historical Z80 snapshot acceptance

The production snapshot codecs load Z80 v1 uncompressed and ED-ED compressed
48K images, preserving CPU registers, interrupt mode, border, and all 48K of
RAM. The canonical v2 writer/load path is round-tripped through the portable
state representation with boundary memory values preserved.

CTest name: `wz_snapshot_historical_contract`.
