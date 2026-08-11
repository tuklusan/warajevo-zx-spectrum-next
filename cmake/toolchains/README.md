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

The checked-in files define the target identity and the expected compiler
family. They deliberately do not contain machine-local absolute paths.

Configure examples:

```text
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/windows-x86_64.cmake
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/linux-x86_64.cmake
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/linux-aarch64.cmake
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/macos-aarch64.cmake
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/macos-x86_64.cmake
```

The Windows file expects `cl` from a Visual Studio developer environment. The
Linux files expect `gcc` or the named cross-GCC in `PATH`. The macOS files use
Apple Clang and set `CMAKE_OSX_ARCHITECTURES`; SDK selection remains the
responsibility of the runner. Remote harnesses must create build directories
under their approved project directory and return outputs under
`test-artefacts/` only.
