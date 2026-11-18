<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
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

Dispatch `.github/workflows/cr-build-gate.yml` on `main`; the retained workflow
is the single hosted build and test entry point.
