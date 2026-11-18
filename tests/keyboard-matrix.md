<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Keyboard matrix acceptance

The hosted contract covers four electrical scan cases: no selected rows,
active-low single-row selection, simultaneous-row union, three-corner ghost
propagation, and disconnected unselected rows. The test exercises the public
keyboard matrix scanner used by machine I/O. Proof for run `36587074073` is
pinned in `test-results/keyboard-matrix.json`; the command is recorded in
`test-drivers/keyboard-matrix.driver.json`.
