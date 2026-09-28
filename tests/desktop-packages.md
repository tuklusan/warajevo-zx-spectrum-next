<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Windows and macOS desktop package audit

The hosted three-runner audit builds and inspects Windows x86-64, macOS
Intel, and macOS Apple Silicon portable archives. It verifies archive
inventory, executable architecture, SHA-256, required license and third-party
notices, exclusion of ROMs and private media, and runtime dependencies.

The macOS archives are unsigned and not notarized audit artifacts; they are
not public releases.
