<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Windows runtime dependency audit

The hosted Windows x64 Release build audits the packaged application's PE
normal and delay-loaded import tables. The check records the executable hash
and passes only when every imported DLL is a Windows system or API-set library.
This verifies that the executable does not require a project-supplied
multimedia or other companion DLL at runtime.

The pinned proof is `test-results/windows-runtime-dependency-audit.json`.
