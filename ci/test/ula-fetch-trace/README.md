<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# ULA fetch trace contract

This hosted contract makes timestamped writes across a bitmap and attribute
fetch boundary, then checks that the production timing trace and dumper record
the fetched memory addresses, bytes, raster positions, and fetch kinds.
