<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# UI remote status acceptance

The hosted four-runner regression verifies that the UI's live remote-control
snapshot produces an always-visible status indicator and a detailed settings
page. It covers a degraded listener with an active Telnet client, an unavailable
service, selected-port/range details, family state, plaintext/no-auth warning,
and remote permission summary.

The desktop application exposes this indicator as a clickable status-bar
control. Activating it opens the detailed Telnet Keyboard & Remote Control
panel; closing the panel leaves the live status indicator available.