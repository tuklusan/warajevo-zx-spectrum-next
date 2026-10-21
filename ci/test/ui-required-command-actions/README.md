<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Required command actions

This independent hosted regression verifies the eight canonical Microdrive
drive selectors and the ROM/Firmware and Telnet Settings actions through the
production command registry. It checks frozen menu metadata, selects every
drive through the canonical ID, and confirms remote dispatch is denied for
local-only Settings actions. The process is terminated by the harness.
