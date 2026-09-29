<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Snapshot command acceptance

The hosted snapshot-command contract verifies four cases:

1. Case-insensitive SNA and Z80 filenames route to snapshot loading.
2. Save requires a destination; Save As updates the remembered destination
   only after accepting a valid path.
3. A 48K Z80 snapshot round-trips machine state, while truncated state input is
   rejected without changing machine state.
4. The 128K model writes representable SNA and Z80 snapshots.

Run on the hosted Ubuntu runner with the command recorded in
`test-drivers/snapshot-command.driver.json`. The proof is pinned in
`test-results/snapshot-command.json`.
