<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Nuklear Dependency Pin

Status: PINNED_FOR_PHASE_12

- Repository: `https://github.com/Immediate-Mode-UI/Nuklear.git`
- Immutable revision: `e3e18dc1e4d3de935095d372aaa211f12183befb`
- License: MIT, as supplied by the upstream repository.
- Acquisition: remote and hosted jobs must fetch this exact immutable revision;
  a floating branch is not an acceptable build input.
- Boundary: Nuklear is restricted to host UI widgets and event translation. It
  must not be included by deterministic machine, timing, raster, audio, trace,
  serialization, or headless test modules.
- Viewport: the Spectrum raster remains owned by the Sokol presentation path;
  Nuklear overlays may not modify or reinterpret it.

The revision was resolved from the upstream repository on 2026-09-06. The
license and provenance remain those of the upstream dependency.
