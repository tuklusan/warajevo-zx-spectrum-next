# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is licensed under the GNU General Public License, version 2 or later.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

# Managed host-output atomicity contract

This standalone target compiles the production `wz_host_output.c` implementation
and races independent processes against shared snapshot/export destinations.
The Python runner validates complete atomic replacements, exclusive no-clobber
claims, observed reader contents, and temporary-file cleanup on each supported
desktop runner family. It does not modify existing test drivers or wrappers.
