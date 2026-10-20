<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Original ZX Net block loopback contract

The fixture checks full-size block reception and byte rotation, busy/free
responses, accepted and rejected claims, and full-size transmit callback
behavior. A failed commit callback must preserve pending state and payload so
the same block can be retried. The committed payload is then fed through the
production receive path and checked byte-for-byte. It uses synthetic bytes
only; it makes no routed-network or physical-hardware claim.
