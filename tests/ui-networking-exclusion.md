<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# UI networking exclusion contract

The hosted contract verifies Interface-1 selection and dispatch, rejects the
unavailable Ear+Mic option in UI state, then attempts the Ear+Mic mode through
the shared registry command and confirms the core remains in Interface-1. The
registry command remains available for supported modes. All 19 assertions
passed on hosted run `36604424135`, source
`88cbd9e62eaa54b46db8ad7e828dba9e4a8829e4`; proof is pinned in
[`ui-networking-exclusion.json`](../test-results/ui-networking-exclusion.json).

This contract verifies rejection of an unavailable mode. It does not exercise
or implement Architecture #3 behavior.