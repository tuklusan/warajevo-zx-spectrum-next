<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Keyboard host-to-core boundary

`ci/test/keyboard-core-boundary/keyboard-core-boundary.c` exercises local and
Telnet ownership through the production host arbiter and Telnet input API,
projects their merged state through the production application boundary, then
reads the authentic ULA keyboard port. It verifies both sources can hold the
same key, releasing one source preserves the other, and releasing Telnet
restores the matrix. The core projection receives only row, column, and pressed
state; source identity remains app-owned.

The 17-assertion contract passed on Windows x64/ARM64 and macOS Intel/Apple
Silicon in run `39393277555` at source commit
`5adbb6b4985ec19fc78f4b22f020ba187a5f11ff`. Proof:
[`keyboard-core-boundary.json`](../test-results/keyboard-core-boundary.json).
