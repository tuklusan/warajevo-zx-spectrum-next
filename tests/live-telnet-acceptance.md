<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Production live Telnet acceptance

Four hosted desktop runners build the production Release/Sokol host and use its real IPv4 Control Port. The 17 checks per runner cover keyboard responses, path-free MENU TREE and DESCRIBE, STATUS path privacy, remote application-quit denial, and PAUSE/RESET/RESUME state behavior.

Run `gh workflow run live-telnet-acceptance.yml --ref main`.
