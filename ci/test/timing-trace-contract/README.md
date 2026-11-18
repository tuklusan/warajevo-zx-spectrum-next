<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Timing trace file contract

The hosted regression exercises independent no-clobber files, the fixed-size
allocation, sparse global event sequences through ring wrap, freeze behavior,
and recovery across an interrupted write at the ring boundary and an
incomplete append beyond the committed header cursor. A three-slot compile-time
ring keeps wrap coverage deterministic and fast; the file remains the
production 16 MiB size.
