<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned decoder and prefix-chain acceptance

The hosted acceptance checks every primary, CB, and ED decode entry for a
stable opcode identity and valid operation/status: 768 entries. Seven DD/FD
cases check ignored prefixes, repeated prefixes, last-prefix index selection,
and exact M1/operand bus events, master ticks, PC, refresh, and IX/IY state.
The pinned Fuse suite separately compares its DD/FD and DDFD cases against
reference state and external transfer timestamps.

Hosted proof: [cpu-decode-prefix.json](../test-results/cpu-decode-prefix.json).
