<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Normal ROM tape loading contract

The hosted regression used the pinned 48K ROM and DIZZY4K TAP, entered the
program through keyboard events, executed BASIC `LOAD` and `RUN`, and advanced
the timed tape through payload loading and end-of-tape. It passed on Ubuntu
24.04 x86-64 in run `39389223036`, at exact source
`10814559b19e9d68003bcfe719c4775f5aae4ee3`; proof and ROM/media hashes are
pinned in [`tape-keyboard-load.json`](../test-results/tape-keyboard-load.json).
This proves the Normal ROM/EAR path for the cited tape. Wider media coverage is
tracked separately under Core criterion 56.
