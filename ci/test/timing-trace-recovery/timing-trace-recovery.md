<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Timing trace standalone recovery

The production CLI is tested against a three-slot ring that crosses the low
32-bit sequence boundary, wraps circular slots, and closes without a freeze.
A second trace contains a partial, uncommitted append beyond the committed
record count. The committed records must remain readable and ordered.
