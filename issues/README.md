<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Change Request Tracker

The local issue-tracking system is the machine-readable file:

`issues/change-requests.json`

## Conventions

- Every item uses a stable `CR-####` identifier.
- `next_cr_number` must always be exactly one greater than the highest
  allocated CR number in the file.
- `status` is one of `open`, `in_progress`, `closed`, or `blocked`.
- `source_authority` should point back to the architecture or backlog document
  that justifies the work.
- The file is intentionally simple JSON so it stays easy to diff, script, and
  review.
