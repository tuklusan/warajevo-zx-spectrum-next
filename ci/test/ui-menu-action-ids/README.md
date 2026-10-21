<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# UI fixed menu action IDs

This hosted production-host contract verifies the nine fixed model and speed
action IDs required by UI Architecture §7.1. It checks that each ID appears
once in `MENU TREE` as remotely allowed `REMOTE_SAFE`, then invokes each action
through `DO` and verifies the selected model or speed in `STATUS`. The GUI
model and speed selectors dispatch the same fixed action IDs. The shared
parameterized commands remain the operation handlers.

Run `.github/workflows/ui-menu-action-ids.yml` on `main`. Build, GUI startup,
and application execution occur on an Ubuntu hosted runner. The workflow's
proof artifact is processed and deleted immediately after retrieval.
