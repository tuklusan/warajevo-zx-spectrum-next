<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Original ZX Net trace fixture

The production trace records accepted read-block delivery, claim transitions,
collision-read byte progress, collision-write bit progress, and successful
write commit with the canonical tick and current state/position. It records no
block payload bytes. The fixture drives the production core API through the
machine's master-tick path and checks block IDs, state transitions, framing
progress, and write callback completion in the standalone trace dumper.
