<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Warajevo-native TAP compression contract

The contract exercises the production type-5 encoder and decoder with literal,
short-distance, long-distance, and long-match data. It checks that compressed
payloads round-trip exactly and that repetitive records receive a smaller
encoded payload before the host maintenance action commits a replacement.
