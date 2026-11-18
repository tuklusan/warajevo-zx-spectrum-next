<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# ULA fetch trace contract

This hosted contract makes timestamped writes across a bitmap and attribute
fetch boundary, then checks that the production timing trace and dumper record
the fetched memory addresses, bytes, raster positions, and fetch kinds.
