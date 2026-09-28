<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Telnet KEY PRESS reset acceptance

The four-runner regression verifies a pending Telnet `KEY PRESS` remains owned
across an emulated master-tick reset, expires after its remaining two frame
periods, and preserves an overlapping local key owner.
