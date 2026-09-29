<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Keyboard matrix acceptance

The hosted contract covers four electrical scan cases: no selected rows,
active-low single-row selection, simultaneous-row union, three-corner ghost
propagation, and disconnected unselected rows. The test exercises the public
keyboard matrix scanner used by machine I/O. Proof for run `36587074073` is
pinned in `test-results/keyboard-matrix.json`; the command is recorded in
`test-drivers/keyboard-matrix.driver.json`.
