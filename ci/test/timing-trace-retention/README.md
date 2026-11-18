<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Full timing-trace retention measurement

The hosted test runs the 48K PAL profile with all trace event kinds enabled,
executes the pinned 48K ROM for at least eight complete profile frames, and
recovers the records retained in the production-size 16 MiB circular file. It
passes only when the recovered event time span covers all eight frames. The
proof stores counts and timing coverage only; it contains no ROM bytes or
emulated memory contents.
