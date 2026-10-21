<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# UI menu registry inventory

The Release/Sokol production host passed 130 assertions on Linux x64 in run
`39560252025` at exact source `bef1a67a697897fb406199a813054c0a31a7a23e`.
The check compares the canonical Section 7.1 IDs with live `MENU TREE` and
`DESCRIBE` responses, verifies command metadata completeness and consistency,
and confirms that dynamic Recent-file paths are not exported. Tracked proof and
fixture hashes are in
[`ui-menu-registry-inventory.json`](../test-results/ui-menu-registry-inventory.json).
