<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Timing trace process ownership

The hosted test launches eight processes against the same new trace path at
once. It requires one successful exclusive creator, seven denials, a frozen
production-format trace with one committed event, and clean process exit.
