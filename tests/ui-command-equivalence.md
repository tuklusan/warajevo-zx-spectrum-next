<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# UI command equivalence regression

The hosted four-runner regression compares Reset, all seven Speed settings,
Pause/Resume, and Model through GUI or toolbar activation, Telnet aliases, and
direct command-registry dispatch. It also saves the same raster through the GUI
and Telnet screenshot paths and compares the resulting PNG bytes.

Run with the command in
[`ui-command-equivalence.driver.json`](../test-drivers/ui-command-equivalence.driver.json).
The passing hosted proof is
[`ui-command-equivalence.json`](../test-results/ui-command-equivalence.json).
