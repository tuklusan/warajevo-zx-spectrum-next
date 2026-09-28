<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Warajevo ZX Spectrum Next — Linux

This is the Linux distribution of Warajevo ZX Spectrum Next. Based on original
work by Supratim Sanyal of SANYALnet Labs. Use and redistribution are subject
to the included `LICENSE`; commercial use requires separate written
authorization.

## Requirements

The host operating system must provide GTK 3.20 or newer, GTK3's native X11
file chooser, X11, OpenGL, ALSA, and the C runtime. These system libraries are
not bundled. A graphical X11 session with an OpenGL-capable display and an
available ALSA audio device is required for interactive use.

## Start

Run `./Warajevo-ZX-Spectrum-Next` from this directory. Supply compatible
Spectrum ROM firmware through the application's supported ROM selection flow;
firmware is not included. Tape and Microdrive media are selected by the user
through the application. Settings are stored under
`${XDG_CONFIG_HOME:-$HOME/.config}/warajevo-zx-spectrum-next/`.

## Contents and notices

This archive contains the application executable, this README, `LICENSE`, and
`THIRD-PARTY-NOTICES.md`. It contains no ROMs, firmware, test or private media,
snapshots, screenshots, or project-supplied shared libraries. Required
third-party notices are included in `THIRD-PARTY-NOTICES.md`.
