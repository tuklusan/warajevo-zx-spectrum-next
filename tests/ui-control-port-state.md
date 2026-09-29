<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# UI Control Port session-state contract

The hosted contract verifies that the unavailable listener state is shown in
the Control Port indicator and is excluded from serialized host preferences.
It passed eight assertions on Linux in run `36598938370`, source
`458cfaceb892ec7973208836b450d166b2480122`; proof is pinned in
[`ui-control-port-state.json`](../test-results/ui-control-port-state.json).
The separate full-range exhaustion proof is
[`control-port-exhaustion.json`](../test-results/control-port-exhaustion.json).
