<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Sokol Dependency Pin

Status: PINNED_FOR_PHASE_5

- Repository: `https://github.com/floooh/sokol.git`
- Ref: `master`
- Immutable revision: `1847290135f95e57e6d220b0a41208306aafc0dd`
- License: zlib, as supplied by the upstream repository.
- Acquisition: remote and hosted jobs must fetch the exact immutable revision;
  floating branch contents are not an acceptable build input.
- Boundary: Sokol is host/presentation infrastructure only. It must not be
  included by deterministic machine, timing, raster, audio, trace, or state
  serialization modules.
- Scope: later host work may use the pinned headers and any required platform
  glue, subject to the architecture's backend and provenance gates.

The revision was resolved from the upstream `master` ref on 2026-09-02. The
upstream README identifies the project as standalone C headers and links the
zlib license; this record does not relicense upstream material.
