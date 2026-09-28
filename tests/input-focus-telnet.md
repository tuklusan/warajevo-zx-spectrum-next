<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Input focus and Telnet ownership acceptance

The four-runner regression holds a key from Telnet while entering text-control
focus and losing GUI focus. It verifies local-source release leaves Telnet's
key ownership and the effective key level active, and that Telnet release-all
then releases the effective key.