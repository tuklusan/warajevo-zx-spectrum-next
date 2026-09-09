<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Dock UI Negative Gate

CR-0334 verifies the Phase-12 rule that Dock Cartridge UI is absent until
Timex/DCK support is implemented. The initial command registry, UI surfaces,
product source, and product tests must not expose Dock selection, unselection,
view, merge, or Dock conversion actions. The architecture document and retained
reference material may mention the deferred feature because those are authority
and provenance, not product exposure.

Verification performed against the current tree on 2026-09-09:

- `src/`, `tests/`, and command/UI implementation paths contain no Dock, DCK,
  or Timex product symbol or command.
- The only current mentions are in architecture/provenance documentation and
  this negative-gate record.
- Future Dock operations remain `LATER` in the legacy disposition trace and are
  not promoted by this CR.
