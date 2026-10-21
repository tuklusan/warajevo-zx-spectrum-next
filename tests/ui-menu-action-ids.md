<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# UI fixed menu action IDs

The hosted production GUI test builds the Release/Sokol desktop application,
starts it under Xvfb, and connects through its production Telnet command
registry. It verifies that both model choices and all seven speed choices occur
exactly once in `MENU TREE` with remote-safe metadata. It invokes all nine IDs
and checks the returned selection and resulting `STATUS` value (27 assertions).
The process is terminated by the harness even after a failure.

Run `.github/workflows/ui-menu-action-ids.yml` by manual dispatch on `main`.
Product build and execution occur on `ubuntu-24.04`. The driver pins the test
script, workflow, ROM, and production command implementation sources.
