<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Native runtime dependency audit

The hosted workflow builds the Release Sokol application on Linux x86-64,
Linux AArch64, macOS x86-64, and macOS Apple Silicon. It records each binary's
SHA-256 and inspects the native dependency table. Linux dependencies must
resolve under `/lib`, `/lib64`, or `/usr/lib`; macOS dependencies must resolve
under `/System/Library` or `/usr/lib`. Unresolved or project-local libraries
fail the audit. This establishes the native system-library boundary for the
audited build; ROM/private-media separation and a final distributable remain
separate release checks.

The consolidated hosted proof is
`test-results/native-runtime-dependency-audit.json`.
