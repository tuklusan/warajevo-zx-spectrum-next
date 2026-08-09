<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# WZSN Private Test Media

This directory is the local difficult-media corpus referenced by the project
architecture.

Private TAP, TZX, SNA, Z80, MDR, and related regression files may be placed
directly in this directory.

## Public-versus-private rule

- `README.md` and `.gitignore` are public and are committed.
- All other files in this directory are local-only and ignored by Git.
- Private media files from this directory must not be copied into release
  artifacts or source archives.

## Suggested local environment variable

```text
WZSN_PRIVATE_TEST_MEDIA=./WZSN-PRIVATE-TEST-MEDIA
```
