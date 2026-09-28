<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Telnet Registry Discovery Regression

This independent hosted regression covers `MENU`, `MENU TREE`, `MENU <id>`,
case-insensitive `MENU FIND`, `DESCRIBE`, and generic `DO`. It verifies the
frozen record terminator, stable disabled reasons, static-only metadata,
argument-schema rejection before availability and permission checks, remote
denials, and that rejected commands never reach their handler.

The runner matrix compiles and executes the same test on Windows x64/ARM64 and
macOS Intel/ARM64. `test-drivers/telnet-registry-discovery.driver.json` pins the
test, workflow, and production source fixtures used to create each proof.
