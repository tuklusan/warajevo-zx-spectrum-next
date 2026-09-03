<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# zlib Dependency Pin

Status: REQUIRED_FOR_TZX_Z_RLE

- Repository: `https://github.com/madler/zlib.git`
- Immutable revision: `925af44f3cde53c6b076611c297850091b5dc7bb`
- Release tag: `v1.3.1`
- License: zlib license, as supplied by the zlib project.
- Acquisition: the exact source release is vendored under `third_party/zlib/`;
  CMake uses a system zlib when available, otherwise it builds that vendor
  tree. The recorded hashed release archive remains a fallback for hosted
  environments where the vendor tree is intentionally absent.
- Boundary: zlib is used only by the TZX CSW Z-RLE decoder; it is not part of
  canonical machine state or host presentation behavior.

The project does not relicense zlib. Its license and provenance remain governed
by the upstream distribution and the applicable terms in `NOTICE.md`.
