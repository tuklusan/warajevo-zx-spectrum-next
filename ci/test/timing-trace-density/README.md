<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Synthetic timing-trace density measurement

The contract emits one ordered production-format record for every 48K PAL CPU
T-state across eight complete frames. It then freezes and recovers the fixed
16 MiB ring, requiring every synthetic record to remain recoverable and the
retained time span to cover all eight frames. This is a deliberately dense
backend-only workload; it does not claim a physical hardware event rate.
