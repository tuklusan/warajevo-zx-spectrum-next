<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Tape loading equivalence and fallback acceptance

The hosted regression runs the same 48K ROM and DIZZY4K TAP load from fresh
state in Normal mode and with Instant/Trap selected. It verifies the ROM
reports no recognized trap loader for that tape, consumes the supported BASIC
block through the real ROM loader, and produces the same normalized full-state
hash, BASIC destination, and tape segment position in both runs.

No accelerated trap path is claimed. Unsupported operations remain on the
normal ROM path until a loader has a proven eligibility and equivalence
contract. Proof for run `36585333631` is pinned in
`test-results/tape-loading-equivalence.json`; the command is recorded in
`test-drivers/tape-loading-equivalence.driver.json`.
