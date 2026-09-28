<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Screenshot path contract

On a hosted Linux runner, the regression fixes the screenshot clock, directs
output to a dedicated directory, asserts the exact documented filename, and
pre-populates that name before saving again. The second save must use the
collision suffix and preserve the existing file byte-for-byte.
