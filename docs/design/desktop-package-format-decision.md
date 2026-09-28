<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Initial Windows and macOS package format decision

## Decision

The Windows x86-64 build uses a portable `.zip` archive. It contains the single
application executable, `README.md`,
`LICENSE`, and `THIRD-PARTY-NOTICES.md` under one product directory. Windows
system APIs and the C runtime remain operating-system dependencies. No
installer, project DLL, ROM, firmware, test media, private media, or generated
user data is included.

macOS Intel and Apple Silicon builds use architecture-specific `.zip` archives
containing `Warajevo ZX Spectrum Next.app` plus the same three documentation
files. The app bundle contains one native executable and a generated
`Info.plist`; it contains no project-supplied `.dylib`, ROM, firmware, test
media, private media, or generated user data. Apple system frameworks remain
operating-system dependencies.

All archives have stable member order and timestamps and a SHA-256 sidecar.
Third-party notices are generated from the pinned Nuklear license and pinned
Sokol helper notice. Each archive's member inventory and executable format and
architecture are checked before upload.

The macOS archive is an unsigned build artifact. It is not a public release;
normal public GUI distribution still requires the project's authorized Apple
signing and notarization process. No signing identity or notarization
credentials are available to this workflow. The initial format does not
include `.msi`, `.pkg`, `.dmg`, or package-manager installers.

## Basis

The Windows target is one executable linked with Sokol and Warajevo code. The
macOS `.app` is the architecture's GUI distribution container around one
program binary. Existing native dependency audits determine whether any
non-system runtime library is present. The archive formats keep user data
outside the install container and make the architecture and contents
inspectable without an installer.

## Acceptance state

This freezes the initial Windows and macOS archive layouts. The first hosted
package audit passed in run `36462115153` at source
`be2ad86aa34d2d2ffbb05f338c1bdee112956aff`: Windows x86-64 and macOS Intel
and Apple Silicon archives passed their inventory, executable-architecture,
checksum, license/notice, media-exclusion, and runtime-dependency checks. The
macOS archives are unsigned and not notarized; Task 456 remains partial until
the authorized public-release signing/notarization process is available.
