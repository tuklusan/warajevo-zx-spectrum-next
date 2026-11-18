<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Original ZX Net block loopback contract

The eight-case production-core contract verifies invalid-block rejection
without mutation; all 256 received bytes and their nine-phase rotation;
busy/free countdown behavior; accepted and rejected claim transitions; a
full 256-byte transmit payload; preservation after a failed write callback;
successful retry/commit to the next block ID, and loopback readback of the
committed full-size block. It runs with synthetic payloads and makes no
routed-network or physical-hardware claim.
