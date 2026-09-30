<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Full timing-trace retention measurement

Hosted Linux x86-64 runs the pinned 48K ROM in the certified 48K PAL machine
with every trace event kind enabled. It advances the CPU for at least eight
complete profile frames, writes through the production 16 MiB trace ring, then
recovers records from that file and verifies the retained timestamp span covers
eight complete frames. The aggregate proof excludes ROM bytes, memory contents,
and trace records.
