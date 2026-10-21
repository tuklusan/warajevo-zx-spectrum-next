<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# ZX Printer core contract

The hosted Release/Sokol build runs `wz_printer_core_contract` in CTest on the
five native targets. It covers port selection, mode/status and control state,
Epson standard/enlarged pixel buffering, row and partial flush transitions,
HP pixel-bit order, bounded flush output, master-tick propagation, and invalid
API inputs. The test passed 3,909 assertions per target in run
`39646431461` at source `2470446f9b8617905a787532ea7426d5f0fed79b`.

This proves the deterministic software contract. It does not claim physical
printer timing, paper-output equivalence, or external hardware correlation.
