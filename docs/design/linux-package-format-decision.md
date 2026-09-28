<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Initial Linux package format decision

## Decision

The initial Linux release has two architecture-specific portable `.tar.gz`
archives:

- `Warajevo-ZX-Spectrum-Next-linux-x86_64.tar.gz`
- `Warajevo-ZX-Spectrum-Next-linux-aarch64.tar.gz`

Each archive has a `Warajevo-ZX-Spectrum-Next/` root containing exactly one
project executable and the release documentation:

```text
Warajevo-ZX-Spectrum-Next/
  Warajevo-ZX-Spectrum-Next
  README.md
  LICENSE
  THIRD-PARTY-NOTICES.md
```

`README.md` states the required host libraries and graphics/audio services,
explains that compatible ROM firmware must be supplied separately, and gives
launch instructions. The notice file carries the applicable notices for
Nuklear and the pinned Sokol helper. Neither archive contains ROMs, private
test media, project-supplied shared libraries, or application data.

The initial release does not produce `.deb`, `.rpm`, AppImage, Snap, or
Flatpak packages. Installation and removal consist of unpacking and removing
the archive directory. Runtime GTK3, X11/OpenGL, ALSA, and C runtime
dependencies are provided by the host operating system and disclosed in the
README; they are not bundled. GTK 3.20 or newer is required for the approved
`GtkFileChooserNative` integration.

The two archives are the complete initial Linux package matrix. Their
architecture names are fixed as `x86_64` and `aarch64`, matching the native
Release targets and dependency audits. Adding a package manager format or
bundling host libraries requires a separate recorded decision before release
scripts or artifacts depend on it.

## Basis

This keeps the Linux distribution container separate from the single ELF
program-binary contract in Core §28. The hosted Release audits found no
project-local companion-library dependencies on either Linux target; GTK3 and
its X11/OpenGL/ALSA stack resolve from operating-system library paths. The
format therefore does not need to embed or install project-owned runtime
libraries.

## External configuration and user data

The host-only settings file is stored in the platform user-configuration
location selected by the Phase-12 decision:

- Windows: `%LOCALAPPDATA%\Warajevo-ZX-Spectrum-Next\host-settings.v1`
- macOS: `~/Library/Application Support/Warajevo-ZX-Spectrum-Next/host-settings.v1`
- Linux: `${XDG_CONFIG_HOME:-$HOME/.config}/warajevo-zx-spectrum-next/host-settings.v1`

Settings contain host preferences only; they do not contain machine/session
state, ROM or media contents, or remote secrets. Writes use the already frozen
exclusive sibling lock, same-directory temporary file, and atomic replacement
policy.

There is no application-owned ROM or media directory. ROM firmware and tape
or Microdrive images are supplied by the user through the supported media
workflows. Snapshots, exports, and other persistent outputs go to a
user-selected destination. Telnet screenshots use the operating-system
temporary directory and the exclusive-create collision policy. Temporary
screenshots are not persisted user data.

The release archives and their README contain no ROMs, firmware, test media,
private corpus, snapshots, or generated screenshots. The README explains that
required firmware must be supplied by the user and that test/private media is
not part of the release.

## Acceptance state

This freezes format and archive layout only. It does not claim that the
archives, license inventory, notice text, ROM/private-media exclusions, or
final package hashes have been implemented or audited. Those remain under
Tasks 369 and 456.
