<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Timing-trace state and failure contract

On hosted Windows and macOS runners, run the production core for 4096
instructions with trace disabled, enabled, toggled, wrapped, and frozen.
Compare canonical machine hashes across every run, verify a trace-file
creation error does not alter or stop emulation, and require two independent
trace runs to produce the same ordered event digest. The test must pass on all
four desktop runner families.
