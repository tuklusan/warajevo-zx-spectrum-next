<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# ZX Printer Settings contract

The Settings menu now exposes `Peripherals > ZX Printer`, where the selected
virtual printer mode is marked and the five modes dispatch through the shared
printer command. The hosted contract checks that this command remains under the
Settings peripheral group and each mode selection updates canonical printer
state.

Run on Linux with:

```text
cmake -S ci/test/printer-settings -B dist/printer-settings -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build dist/printer-settings --config Release --target wz_printer_settings_contract --parallel 2
dist/printer-settings/wz_printer_settings_contract
```

Hosted proof is recorded in `test-results/printer-settings.json`.
