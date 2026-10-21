<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Required command actions

The hosted Release/Sokol production-host regression passed 21 assertions on
Linux x64 in run `39556128554` at exact source
`79f3ae95c184b1ce862a13cb265479737a5ecc20`. It verifies the eight canonical
Microdrive selectors, ROM/Firmware and Telnet Settings IDs, frozen registry
metadata, selector dispatch, local-only permission denial, and the visible
Telnet settings surface. The screenshot was inspected; its SHA-256 and fixture
hashes are pinned in
[`ui-required-command-actions.json`](../test-results/ui-required-command-actions.json).

Dispatch `.github/workflows/ui-required-command-actions.yml` on `main`.
