# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

# TapeFiles Disposition Trace

This trace reconciles every TapeFiles branch in UI architecture Sections 20.1
through 20.8. The architecture document remains the detailed migration
authority; this artifact records the destination class and implementation
boundary used by the backlog.

| Authority | Legacy operation set | WZSN destination or disposition | Evidence/boundary |
|---|---|---|---|
| 20.1 | Select, Unselect | Tape Insert/Eject quick actions | CR-0307, CR-0308 |
| 20.1 | Parameters, View | Tape Manager mode/position and block presentation | CR-0309, CR-0310 |
| 20.1 | Print to Screen | Replaced by Tape Manager presentation | CR-0310, CR-0313 |
| 20.1 | Print to Printer | DROP: legacy host printer path | Section 20.1 disposition |
| 20.1 | Print to File | Optional portable Export Tape Report | CR-0313 |
| 20.1 | Copy to New | Tape Manager copy-to-new backend | CR-0311 |
| 20.2 | Reorder, Extract, Delete, Add, Edit, Change Position | Tape Manager block operations | CR-0311, CR-0314 |
| 20.2 | Exclude | Advanced Warajevo-native maintenance applicability | CR-0312 |
| 20.3 | Compress all, linearize only, compress selected, decompress, efficiency | Compatibility Tools; native-only applicability; implementation remains bounded by format support | CR-0312; later mutation/serialization work |
| 20.4 | Send/receive to Spectrum, send communication program, configure RS232 | DROP: old real-Spectrum/DOS host bridge; authentic IF1 serial remains core/peripheral scope | Section 20.4 disposition |
| 20.5 | Receive/sample from cassette, advanced setup | DROP/LATER: physical capture is a separate future Tape Capture workflow | Section 20.5 disposition |
| 20.6 | ASCII/BASIC and ASCII/source-language conversions | Compatibility Tools; explicit conversion only; no ordinary Tape Manager command | Section 20.6 disposition |
| 20.7 | SCREEN$ to color or black/white TIFF | Compatibility Tools; legacy conversion only; ordinary screenshots use shared PNG service | Section 20.7; screenshot architecture |
| 20.8 | Roman & Easy, Lunter TAP, Irish SpecEm, Polish SP/SPC, Spectrum 2.00 BLK, ZX Garabik LTP, ZX Brukner, ZX Museum ZXS, TR-DOS TRD, ZX32 ZXT/ZXS, VOC conversions in both directions | Compatibility Tools; each source/destination pair is explicit and must disclose loss; no silent conversion | Section 20.8 disposition |

## Coverage Rules

- Every operation in Sections 20.1–20.8 has exactly one destination class in
  the table: implemented manager behavior, Compatibility Tools, DROP, or
  LATER/separate workflow.
- Ordinary TAP/TZX transport and block management do not expose native-only
  maintenance or conversion actions.
- Compatibility conversion engines are not claimed complete merely because
  their historical menu entries are traced.
- Any future implementation must add its own CR, upstream functional inventory,
  explicit loss/error behavior, and evidence before changing a disposition.
- No entry introduces Architecture #3 functionality.
