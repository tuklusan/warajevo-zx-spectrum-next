<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Standalone trace recovery contract

This hosted contract builds the production trace dumper, then opens production
trace files after an un-frozen close. It checks low-sequence rollover and an
incomplete append left beyond the committed record count. A three-slot test
ring forces the CLI to recover after actual circular-slot wrap.
