<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Cassette EAR trace contract

This contract advances the production CPU runner through tape segments and
checks that EAR transitions are emitted at their exact master ticks, in order.
The five-target build gate runs the contract as part of integrated CTest.
