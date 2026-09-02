<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Linux Graphics Backend Decision

Status: FROZEN_FOR_PHASE_5

The initial Linux host backend is **X11**, matching the architecture baseline.
Sokol platform glue and graphics setup remain confined to host code; the
deterministic machine, timing, raster, audio, trace, and serialization modules
must remain usable without X11 or any other window-system dependency.

Wayland and other Linux backends are deferred until an architecture-approved
follow-up task provides their requirements and validation evidence. This
decision does not change canonical raster generation or machine timing.
