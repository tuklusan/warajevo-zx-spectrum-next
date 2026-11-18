<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms, warranty disclaimer, termination,
patent, trademark, and governing-law provisions. -->

# Runner bootstrap fixtures

These fixtures are used only on hosted project runners. The local workstation
must never build, execute, or bootstrap the product.

Each platform directory contains the dependency setup for its runner family.
The manifest files are part of the cache key. A cache miss or manifest change
requires a fresh bootstrap.

Linux support must install the X11 development and runtime packages required
by the emulator host. The current four-runner matrix is Windows Intel, Windows
ARM64, macOS Intel, and macOS ARM64.
