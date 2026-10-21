<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Production live Telnet acceptance

Four hosted desktop runners build the production Release/Sokol host and use its real IPv4 Control Port. The 17 checks per runner cover keyboard responses, path-free MENU TREE and DESCRIBE, STATUS path privacy, remote application-quit denial, and PAUSE/RESET/RESUME state behavior.

Run `gh workflow run live-telnet-acceptance.yml --ref main`.
