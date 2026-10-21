<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# UI View menu actions

The hosted production GUI regression verifies the required View menu entries,
their local-only Telnet permission, status-panel hide/show behavior, Display
Settings opening, and continued emulator execution. The exact-source Linux
x64 run `39553041351` passed 10 assertions at commit
`a6aebfc0ed2765370ba36d8e7359893eaabf0f17`. Its five screenshots were
inspected; their SHA-256 hashes and runner metadata are pinned in
[`ui-view-menu-actions.json`](../test-results/ui-view-menu-actions.json).

Dispatch `.github/workflows/ui-view-menu-actions.yml` on `main`. The workflow
builds and runs the Release/Sokol production application on `ubuntu-24.04`.
