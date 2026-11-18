<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
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

Dispatch `.github/workflows/cr-build-gate.yml` on `main`; the retained workflow
is the single hosted build and test entry point.
