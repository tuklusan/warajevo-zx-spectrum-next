<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# GUI tape mode selection

The hosted test runs the production GUI under X11, captures the toolbar's
initial `Normal` mode, selects `Instant`, then restores `Normal`. OCR verifies
each visible mode, and a control-port status check confirms mode selection
leaves the machine running. Proof from current source
`791c3bad2ded2cef8efa903bd249e28cf090d926` is recorded in
[`gui-tape-mode.json`](../test-results/gui-tape-mode.json), with screenshots
and the raw runner proof under
[`gui-tape-mode/`](../test-results/gui-tape-mode/).

This establishes that Instant mode is explicitly selectable in the production
GUI. It does not claim accelerated trap loading or preference persistence.
