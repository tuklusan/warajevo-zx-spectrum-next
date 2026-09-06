<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Phase-11 Regression Coverage Matrix

This matrix records the CR-0254 comparison of the closed monitor/printer
inventories against current shared APIs and regression evidence. `COVERED`
means a tracked test exercises the current API; `MISSING` means the required
workflow still needs an implementation CR and must not be counted as a Phase-11
closure pass.

## Monitor and Debugger

| Inventory workflow | Status | Current evidence or next action |
|---|---|---|
| Snapshot and read-only memory | COVERED | `test_debugger_read_only_inspection` |
| Controlled register mutation | COVERED | `test_debugger_read_only_inspection` |
| Controlled memory mutation | COVERED | `test_debugger_memory_mutation` |
| Copy memory | COVERED | `wz_debugger_memory_tools_tests` |
| Find memory | COVERED | `wz_debugger_memory_tools_tests` |
| Fill memory | COVERED | `wz_debugger_memory_tools_tests` |
| Put bytes/words | COVERED | `wz_debugger_memory_tools_tests` |
| Breakpoint | COVERED | `test_debugger_breakpoint` |
| Single step and continue | COVERED | `test_debugger_step_continue` |
| Disassemble and value formatting | COVERED | `wz_debugger_disassembly_tests` |
| Jump/program-counter mutation | COVERED | `wz_debugger_jump_undo_tests` |
| Undo register mutation | COVERED | `wz_debugger_jump_undo_tests` |
| Abort/warm restart | COVERED | `wz_machine_reset_tests` through CR-0259 |
| Hex/value formatting | COVERED | `wz_debugger_disassembly_tests` |
| Load/save diagnostic block | COVERED | `wz_diagnostic_block_tests` through CR-0260 |
| Memory pointer/page configuration | COVERED | `wz_debugger_page_controls_tests` through CR-0261 |
| Quit/lifecycle command | COVERED | `wz_application_lifecycle_tests` through CR-0262 |
| Screen inspection | COVERED | `wz_screen_inspection_tests` through CR-0263 |
| Refresh/monitor presentation | COVERED | `wz_presentation_snapshot_tests` through CR-0264 |

## ZX Printer

| Inventory workflow | Status | Current evidence |
|---|---|---|
| Port status/control | COVERED | `wz_printer_tests` |
| Motor/readiness transitions | COVERED | `wz_printer_tests` |
| Four printer modes | COVERED | `wz_printer_tests` |
| Pixel buffering and row flush | COVERED | `wz_printer_tests` |
| Captured output export | COVERED | `wz_printer_export_tests` |
| Host BIOS/LPT output | REPLACED | Explicitly outside deterministic core |
| Tape/snapshot print-to-host | DROPPED | Explicit UI architecture disposition |

## Exit Decision

CR-0254 cannot close and task 236 cannot be claimed complete while any
`MISSING` monitor workflow remains unclassified or without an owning follow-up
CR. The covered rows are retained as regression evidence and are not silently
expanded to imply coverage of the missing legacy behaviors.
