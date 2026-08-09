<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Warajevo ZX Spectrum Next

Warajevo ZX Spectrum Next is the working repository for a modern, portable
continuation of Warajevo guided by the architecture authorities in
`design/`.

## Repository baseline

- The core and UI architecture authorities live in `design/`.
- The bootstrap C11 build lives in `CMakeLists.txt`.
- Repository gates live in `tools/validate_project_gates.py`.
- Local change tracking lives in `issues/change-requests.json`.
- Git and GitHub workflow notes live in `README-GIT-GITHUB.md`.

## Initial tree

```text
cmake/
design/
issues/
reference/
src/
tests/
third_party/
tools/
```

The detailed source-tree intent comes from
`design/warajevo-zx-spectrum-next-architecture.md`. The tracked tree in this
repository is the initial Phase-0 scaffold that preserves those module
boundaries while the implementation is still a bootstrap.

## Build bootstrap

Configure and build the current bootstrap with:

```text
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Governance

Every new source or document artifact must carry the project notice header.
Repository validation also enforces the reserved banned-term rule and the CR
tracker structure.
