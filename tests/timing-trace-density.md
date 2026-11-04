<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Synthetic timing-trace density contract

The hosted contract writes one monotonically ordered trace record per 48K PAL
CPU T-state over eight complete frames using the production 16 MiB file size.
It freezes and recovers the file, checks the exact fixed size, verifies that no
record was lost, and requires the recovered time span to cover eight frames.
