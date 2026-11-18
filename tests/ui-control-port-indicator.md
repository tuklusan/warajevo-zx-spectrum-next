<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# UI Control Port indicator contract

The hosted regression verifies that an available IPv4 listener displays its
selected numeric port. It passed four assertions on Linux in run
`36601246845`, source `532250f965df896f0d62fcae8d9b67944e55c2c8`; proof is
pinned in [`ui-control-port-indicator.json`](../test-results/ui-control-port-indicator.json).
The complementary unavailable-state display and preference-serialization
checks are covered by [ui-control-port-state](ui-control-port-state.md), and
full candidate-range exhaustion is covered by
[`control-port-exhaustion.json`](../test-results/control-port-exhaustion.json).
