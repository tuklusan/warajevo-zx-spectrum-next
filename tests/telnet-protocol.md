<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Telnet protocol regression

The hosted regression checks HELP/STATUS response contracts, quoted and escaped
arguments, literal shell-like text, the exact 1024-byte line limit and recovery
after an overlong line, malformed control-byte rejection, Telnet option
negotiation, and screenshot parsing/error framing. Together with the registry
discovery and DIZZY4K hosted tests, coverage includes MENU, DESCRIBE, DO,
aliases, permission outcomes, and a live screenshot request.

Run with the command in
[`telnet-protocol.driver.json`](../test-drivers/telnet-protocol.driver.json).
The passing hosted proof is
[`telnet-protocol.json`](../test-results/telnet-protocol.json).
