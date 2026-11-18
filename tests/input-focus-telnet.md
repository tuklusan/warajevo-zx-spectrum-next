<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Input focus and Telnet ownership acceptance

The four-runner regression holds a key from Telnet while entering text-control
focus and losing GUI focus. It verifies local-source release leaves Telnet's
key ownership and the effective key level active, and that Telnet release-all
then releases the effective key.
