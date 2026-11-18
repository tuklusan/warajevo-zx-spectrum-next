<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Telnet single-client gate regression

The four-runner live socket regression confirms that the first client is
accepted, a concurrent second client receives `BUSY` and is closed without
disturbing the first connection, and a new client can connect after disconnect.
Disconnect releases Telnet-owned keys while preserving a local key hold.

Run with the command in
[`telnet-client-gate.driver.json`](../test-drivers/telnet-client-gate.driver.json).
The passing hosted proof is
[`telnet-client-gate.json`](../test-results/telnet-client-gate.json).
