<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Production GUI menu-group check

Dispatch `gui-menu-groups.yml` on `main`. The hosted workflow builds the
Release/Sokol desktop host, inspects canonical root and nested menu labels from
the live GUI, including all Microdrive drive groups and Drive 1 actions, then
selects 128K and 400% through the displayed Machine menu. The real Control Port
reports the resulting machine model and speed. The workflow terminates its
application process and uploads short-retention proof and full-screen captures
so nested native popups are included for inspection.

The standalone driver imports only the existing screenshot helper's public
window, pointer, and socket utilities. It does not modify or replace a working
test harness.
