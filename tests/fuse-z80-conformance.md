<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Fuse Z80 conformance

This test runs all 1,356 cases in Fuse's `z80/tests/tests.in` and
`z80/tests/tests.expected` from revision
`c94a5e611bb0b795d62e904a0f5af4f59f2a4eee`. The hosted runner compares final
CPU state, T-state count, all changed memory, and every MR/MW/PR/PW transfer
and transfer timestamp.

Fuse `coretest` emits MC/PC markers from its synthetic contention helper calls
and memory map. Those markers are excluded from this CPU conformance comparison;
the result does not certify contention behavior for the project's 48K PAL
profile. No Fuse cases are skipped.

The driver is [fuse-z80-conformance.driver.json](../test-drivers/fuse-z80-conformance.driver.json).
The latest committed hosted result is [fuse-z80-conformance.json](../test-results/fuse-z80-conformance.json).
