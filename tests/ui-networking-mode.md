<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# UI networking mode contract

The hosted contract verifies the three Networking options and their shared
`machine.networking.set` command, frozen metadata and remote permission,
Interface-1 selection and dispatch, cold machine reconfiguration, and rejection
of the unavailable Ear+Mic option without machine-state mutation. The contract
passed 16 assertions on hosted run `39386702812`, exact source
`f6595997d2cf8f44275d7e8263b7275b3ee867b8`; proof is pinned in
[`ui-networking-mode.json`](../test-results/ui-networking-mode.json).

This contract does not establish Architecture #3 networking behavior.
