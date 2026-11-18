<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Synthetic timing-trace density measurement

The contract emits one ordered production-format record for every 48K PAL CPU
T-state across eight complete frames. It then freezes and recovers the fixed
16 MiB ring, requiring every synthetic record to remain recoverable and the
retained time span to cover all eight frames. This is a deliberately dense
backend-only workload; it does not claim a physical hardware event rate.
