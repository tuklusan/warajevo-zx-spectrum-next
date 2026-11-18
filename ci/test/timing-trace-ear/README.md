<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Cassette EAR trace contract

This contract advances the production CPU runner through tape segments and
checks that EAR transitions are emitted at their exact master ticks, in order.
The five-target build gate runs the contract as part of integrated CTest.
