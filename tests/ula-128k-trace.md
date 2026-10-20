<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# 128K full-frame ULA trace

The hosted Release CTest builds the production trace dumper and the
project-owned 128K trace fixture with the full GUI host enabled. It validates
the complete 12,288-fetch PAL frame, screen-bank paging across fetch events,
expected addresses and values, and same-tick CPU-bus-before-ULA ordering on all
five delivered native targets. The current proof is
[`ula-128k-trace.json`](../test-results/ula-128k-trace.json), from run
`39477966209` at commit `2301ca32b2443a59e1ddcf03b76be4842c5fce26`.

This contract covers the 128K ULA full-frame trace; it does not by itself
establish arbitrary cross-domain failure reconstruction or real-hardware
correlation.
