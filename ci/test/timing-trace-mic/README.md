<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# ULA MIC trace contract

This contract verifies that `OUT (FE)` writes record typed MIC line transitions
at the exact write tick, preserve their previous/new levels, and omit redundant
writes. It runs as part of the full native build gate on all five targets.
