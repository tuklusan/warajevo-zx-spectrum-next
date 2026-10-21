<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Screenshot state and master-time contract

The hosted five-target Release/Sokol build compiles and runs the production
`wz_telnet_screenshot_save` path against a published presentation snapshot.
The contract hashes an initialized 48K machine at a nonzero master tick before
and after both a successful PNG write and a forced output failure. Each result
must preserve the hash and exact master tick. The test also verifies successful
PNG output and that the failed write returns no path. It uses an isolated
temporary build directory and removes its output files.
