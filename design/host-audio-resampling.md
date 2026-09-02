<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Host Audio Resampling Decision

The host presentation layer uses bounded linear interpolation when converting
canonical 44,100 Hz PCM to a physical device rate. The source-position phase
is host-only and uses a 64-bit unsigned fixed-point accumulator; samples are
clamped to the canonical signed sample type after interpolation.

Runtime speed changes the source consumption rate within the audible 0.5x to
2.0x range, preserving the architecture's natural duration and pitch change.
It does not alter the canonical sample timeline, mixer state, machine timing,
or event ordering. Outside the audible range, the host policy discards output
and does not accumulate canonical samples for later playback.

The device adapter owns interpolation state and may reset only at an explicit
host stream restart. A device callback consumes prepared samples; it never
advances the machine or requests core work.

This is a host-presentation decision, not a deterministic-core algorithm.
