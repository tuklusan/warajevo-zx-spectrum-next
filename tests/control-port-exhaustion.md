<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Control Port exhaustion contract

The hosted Linux contract occupies every port from 30740 through 32787 over
IPv4, then confirms probing reports the service unavailable without retaining
a listener or selecting a port outside the configured range. It passed on run
`36589928461` at commit `4e780f8a2eb9f0475fb7cf710b591c1ff35abc37`. The pinned
proof is [control-port-exhaustion.json](../test-results/control-port-exhaustion.json);
the reproducible command and source fixtures are listed in
`test-drivers/control-port-exhaustion.driver.json`.
