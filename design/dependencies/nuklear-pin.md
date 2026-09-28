<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Nuklear dependency pin

The in-window C widget layer uses Nuklear release `v4.13.3`, pinned to commit
`a53ad2c658151071501372a5e0e5e978153835aa` from
[the upstream Nuklear repository](https://github.com/Immediate-Mode-UI/Nuklear).
CMake fetches that immutable commit and compiles it into the application.
The upstream project identifies the library as dual-licensed under MIT or
public domain. The final release package must retain the applicable upstream
license notice; that package-level audit remains open.

Rendering and Sokol event forwarding use `util/sokol_nuklear.h` from the
already pinned Sokol commit
`1847290135f95e57e6d220b0a41208306aafc0dd`. Sokol owns the viewport and
presentation upload; Nuklear owns in-window widgets and text rendering. Its
embedded default font is used so the host has no external font-file dependency.

The host presents the canonical semantic menus in-window on every supported
platform. File selection and platform accessibility bridges remain host-shell
workflows; keyboard operation, focus order, labels, and actionable state are
project-owned requirements and must be covered by UI acceptance evidence.
Settings remain host-only, versioned, and interprocess-safe.

The Nuklear upstream license is available at
[`LICENSE`](https://github.com/Immediate-Mode-UI/Nuklear/blob/a53ad2c658151071501372a5e0e5e978153835aa/LICENSE).
The Sokol helper carries its upstream zlib/libpng notice in
[`sokol_nuklear.h`](https://github.com/floooh/sokol/blob/1847290135f95e57e6d220b0a41208306aafc0dd/util/sokol_nuklear.h).
