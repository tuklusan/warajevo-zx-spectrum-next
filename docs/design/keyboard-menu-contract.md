<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Keyboard menu contract

The app-owned F1 menu exposes core application actions without a mouse. Its
Up/Down selection wraps, Enter activates the selected action, and Escape closes
the menu while restoring the prior UI focus owner. Opening the menu releases
local Spectrum key ownership and preserves independent Telnet ownership. Losing
native window focus closes the menu. The selected row has a textual focus marker
so focus is visible without relying on color.

The 36-case C contract checks focus ownership, menu lifecycle, navigation
boundaries, and Telnet/local input independence. A 49-check integration gate
verifies the host bindings for the required workflows. This does not claim
native screen-reader exposure; that limitation is recorded in the UI
architecture decisions.

Run on Linux with:

```text
cmake -S ci/test/keyboard-menu -B dist/keyboard-menu -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build dist/keyboard-menu --config Release --target wz_keyboard_menu_contract --parallel 2
dist/keyboard-menu/wz_keyboard_menu_contract
```

Hosted proof is recorded in `test-results/keyboard-menu.json`.
