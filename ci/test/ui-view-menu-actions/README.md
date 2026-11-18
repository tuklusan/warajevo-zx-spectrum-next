<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# UI View menu actions

This hosted production GUI regression verifies the two stable View actions in
the live command registry, their local-only Telnet permission, status-panel
hide/show behavior, the shared Display Settings handler, and continued machine
execution. It captures screenshots for inspection and terminates the emulator
after the run. Product compilation and execution stay on the hosted runner.

Dispatch `.github/workflows/cr-build-gate.yml` on `main`.
