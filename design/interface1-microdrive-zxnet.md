<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Interface 1, Microdrive, and ZX Net Notes

This repository baseline captures the current architectural relationship among
the networking modes and the preserved legacy features.

## Initial dispositions

- Interface 1 is a required feature for the first architecture-complete
  milestone.
- Microdrive and MDR support are required for the first architecture-complete
  milestone.
- Original ZX Net behavior is required for the first architecture-complete
  milestone.
- Ear+Mic networking belongs to Architecture #3 and must remain out of scope
  for the present backlog except for the reserved mode and disabled-state
  plumbing already required upstream.

## Structural rule

The UI architecture freezes a single networking mode selector with exactly:

- `NONE`
- `INTERFACE1`
- `EAR_MIC`

`INTERFACE1` and `EAR_MIC` are mutually exclusive by data model and by cold
machine reconfiguration semantics.

## Early implementation note

This file exists so later tasks can record Interface 1, Microdrive, and ZX Net
assumptions in one place without overloading the main architecture documents.
