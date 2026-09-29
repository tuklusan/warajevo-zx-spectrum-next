<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Snapshot inspector acceptance

The hosted snapshot-inspector contract verifies three cases:

1. A live 48K machine exposes formatted model, CPU register, paging, AY, memory-page, and warning data.
2. A live 128K machine exposes its paging value and all RAM banks.
3. Invalid arguments are rejected and close clears the open inspector state.

The contract ran on the hosted Ubuntu runner using the command recorded in
`test-drivers/snapshot-inspector.driver.json`. Proof for run `36580828701` is
pinned in `test-results/snapshot-inspector.json`. Snapshot load/save behavior is
covered separately by `test-results/snapshot-command.json`.
