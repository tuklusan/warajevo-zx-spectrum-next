<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Telnet screenshot runtime acceptance

The hosted Release/Sokol production host is exercised through the real Control Port. The nine assertions verify the default OS temporary-directory destination, absolute timestamped path, completed readable 448x312 PNG, unchanged running and paused status, byte-identical paused raster, pause/resume behavior, and a failed write that returns no path and preserves status.

Run `gh workflow run telnet-screenshot-runtime.yml --ref main`.
