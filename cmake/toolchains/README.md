<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Toolchain Directory

This directory is reserved for pinned CMake toolchain files that support the
architecture matrix recorded in
`design/wzsn-architectures-1-2-developer-tasks.md`.

Initial target matrix:

- Windows x86-64
- Linux x86-64
- Linux AArch64
- macOS AArch64
- macOS x86-64

Toolchain files are intentionally not frozen yet; this repository baseline
creates the canonical location so later Phase-0 and Phase-13 work can land
without restructuring the tree.
