<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Interface 1 ROM paging contract

The hosted CTest uses a synthetic 8 KiB Interface 1 ROM with initialized base
memory. It checks exact-size validation and failure atomicity, M1-triggered
paging at the 48K ROM's `0008h` and `1708h` entries, the Interface 1 `0700h`
return fetch and subsequent unpaging, and non-M1/NONE-mode controls. It does
not claim authentic ROM compatibility or hardware certification.

The test runs as part of `cr-build-gate.yml` with the GUI host enabled on all
five native supported targets.
