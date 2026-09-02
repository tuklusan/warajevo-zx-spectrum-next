<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Keyboard Ghosting Evidence and Frozen Policy

## Scope

This record covers the original 48K/128K 8-row by 5-column membrane matrix.
It does not cover host keyboard rollover, host mapping, or application focus.

## Evidence

- The preserved Warajevo source reads the keyboard as eight active-low rows of
  five columns and combines selected rows logically; it contains no electrical
  network or phantom-key calculation.
- The ZX Spectrum service-manual material describes the keyboard as separate
  eight-line and five-line matrix connections.
- Hardware descriptions of the original membrane report no isolation diode at
  each switch. A passive switch matrix without per-switch isolation can conduct
  through a three-switch rectangle and expose an unpressed intersection.
- The architecture explicitly requires multiple-row selection to remain
  authentic and requires any additional electrical behavior to be resolved
  before Phase-5 exit.

## Frozen Policy

The canonical matrix model represents pressed switches as row-column edges.
For a scan, a column is low when it is connected through pressed edges to any
selected row. This preserves ordinary row-AND behavior and additionally
represents passive-network phantom columns when a connected component reaches a
selected row. No host timestamp, UI event, or host key identity participates.

The policy is deterministic and applies to the canonical Spectrum matrix. The
existing simple row representation remains a compatibility storage format until
the machine-state integration task consumes this model; it must not be treated
as evidence that ghosting is absent.

## Upstream Disposition

Upstream behavior is retained for its software-visible row scan contract. The
electrical extension is new project behavior required by the architecture's
hardware-fidelity gate; its implementation and three-key rectangle tests are
owned by CR-0132.

## Sources

- Preserved reference: `reference/original-warajevo/source/src/spectrum-kernel/SPECSIM.ASM`.
- ZX Spectrum service manual, keyboard matrix and interface description:
  https://oldcrap.org/wp-content/uploads/2023/04/sinclair-zx-spectrum-service-manual.pdf
- Sinclair Wiki, original 40-key 8x5 matrix description:
  https://sinclair.wiki.zxnet.co.uk/wiki/Keyboard
- Tynemouth Software, reverse-engineering discussion of the original matrix:
  https://blog.tynemouthsoftware.co.uk/2022/05/reverse-engineering-a-zx-spectrum-keyboard.html
