<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# ULA MIC trace contract

This contract verifies that `OUT (FE)` writes record typed MIC line transitions
at the exact write tick, preserve their previous/new levels, and omit redundant
writes. It runs as part of the full native build gate on all five targets.
