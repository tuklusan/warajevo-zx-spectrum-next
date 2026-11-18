<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Input dialog focus contract

Entering text-control focus releases locally owned Spectrum matrix keys and
stops viewport key forwarding. The contract also verifies that an independently
owned Telnet key remains down, viewport forwarding resumes after text focus
ends, and native focus loss releases local keys without disturbing Telnet
ownership.

Run on Linux with:

```text
cmake -S ci/test/input-dialog-focus -B dist/input-dialog-focus -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build dist/input-dialog-focus --config Release --target wz_input_dialog_focus_contract --parallel 2
dist/input-dialog-focus/wz_input_dialog_focus_contract
```

Hosted proof is recorded in `test-results/input-dialog-focus.json`.
