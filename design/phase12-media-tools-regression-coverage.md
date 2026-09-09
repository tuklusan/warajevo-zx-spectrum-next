<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Phase-12 Media And Tools Regression Coverage

This ledger closes the task-329 trace requirement against the UI architecture
and the current CTest registration. Every acceptance family has a named source
test and executable; the product matrix remains the authority for cross-platform
execution of these tests.

| Acceptance family | Source test | Registered executable | Contract covered |
|---|---|---|---|
| Microdrive Manager operations | `tests/wz_microdrive_manager_tests.c` | `wz_microdrive_manager_tests` | mount, eject, default, catalog, format, optimize, allocation, rename, protection, file and sector operations |
| Destructive and raw-sector confirmation boundaries | `tests/wz_microdrive_manager_tests.c` | `wz_microdrive_manager_tests` | dangerous raw edit is distinct; command metadata and dispatch remain explicit |
| ZX Printer Manager presentation | `tests/wz_printer_manager_tests.c` | `wz_printer_manager_tests` | empty state, flush view, render, and command registration |
| Printer export | `tests/wz_printer_export_tests.c` | `wz_printer_export_tests` | bitmap export, invalid arguments, buffer limits, and unsupported state |
| Debugger memory/register tools | `tests/wz_debugger_memory_tools_tests.c`, `tests/wz_debugger_jump_undo_tests.c` | `wz_debugger_memory_tools_tests`, `wz_debugger_jump_undo_tests` | paused mutation, bounds, search, copy/fill, register jump, undo, and read-only rejection |
| Debugger page/disassembly controls | `tests/wz_debugger_page_controls_tests.c`, `tests/wz_debugger_disassembly_tests.c` | `wz_debugger_page_controls_tests`, `wz_debugger_disassembly_tests` | paging validation, formatting, disassembly, and invalid state handling |
| Debugger window surface | `tests/wz_debugger_window_tests.c` | `wz_debugger_window_tests` | open/close, pause gate, refresh, memory selection, step/continue, and command registration |
| Compatibility availability and routing | `tests/wz_compatibility_tools_tests.c` | `wz_compatibility_tools_tests` | available/later state, unknown handling, native versus explicit conversion routing |
| Loss disclosure | `tests/wz_compatibility_tools_tests.c` | `wz_compatibility_tools_tests` | every deferred conversion exposes a non-empty warning; native route remains disclosure-free |

The listed executables are registered with `add_test` in `CMakeLists.txt`.
This ledger records coverage and does not claim a local execution result;
remote and hosted evidence remains mandatory.
