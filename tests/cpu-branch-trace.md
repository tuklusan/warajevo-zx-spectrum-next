<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Project-owned conditional branch trace

The hosted case checks taken and not-taken paths for JR NZ,e and DJNZ,e
from uncontended 48K RAM. It verifies final PC, B, flags, refresh register,
master ticks, every bus cycle type/address/value/duration, and matching
TIMING_FULL bus, opcode, and instruction records. The four paths take
12, 7, 13, and 8 T-states respectively.

The 7/12-T-state JR NZ behavior follows the
[ZiLOG Z80 CPU User Manual](https://www.zilog.com/docs/z80/um0080.pdf),
JR NZ,e description. The project also compares its transfer events against
the pinned Fuse Z80 suite; the local case additionally checks internal
cycles and production trace ordering.

Hosted proof: [cpu-branch-trace.json](../test-results/cpu-branch-trace.json).
