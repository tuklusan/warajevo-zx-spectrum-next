<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Warajevo ZX Spectrum Next — Notices and Provenance

## 1. Project identity

**Warajevo ZX Spectrum Next** is a modern, portable continuation of the classic
Warajevo Spectrum emulator.

Current project repository:

`https://github.com/tuklusan/warajevo-zx-spectrum-next.git`

Preserved Warajevo 2.50 source reference:

`https://github.com/tuklusan/warajevo-spectrum-2.50.git`

Official historical Warajevo source/download page:

`https://worldofspectrum.net/warajevo/Download.html`

Official historical Warajevo revision page:

`https://worldofspectrum.net/warajevo/Revision.html`

The preserved Warajevo 2.50 repository is a reference and provenance source.
Warajevo ZX Spectrum Next is a separate modern project.

## 2. Original Warajevo authorship

The classic Warajevo source identifies the principal Warajevo authors as:

- Željko Jurić
- Samir Ribić

Some original source notices spell these names without diacritics as
`Zeljko Juric` and `Samir Ribic`. Those historical spellings should not be
silently rewritten inside preserved upstream source files.

Warajevo ZX Spectrum Next does not claim authorship or ownership of the
original Warajevo work.

## 3. Upstream GPL status

The official Warajevo revision history states that release 2.52, dated
February 2006, was identical to 2.51 except that the license was changed to
the GNU General Public License and source code was made available.

The official Warajevo download page states that Warajevo has been open source
under the GPL since February 2006 and that the source available there is from
Warajevo 2.50.

The preserved-source licensing audit found no explicit GNU GPL version in the
four inspected Warajevo source archives, no obvious bundled GPL license text,
and no source-file header establishing a specific GPL version or an
"or later" clause.

For that reason, this project records the historical upstream statement as
**GNU GPL, no version number stated**.

GNU GPL versions 1, 2, and 3 each contain a revised-versions clause providing
that, if a Program does not specify a GPL version number, the recipient may
choose any GNU GPL version ever published by the Free Software Foundation.
That clause is relevant evidence when interpreting Warajevo's unnumbered GPL
statement.

The project nevertheless does not rewrite history by labeling the upstream
release itself as GPL-2.0-only, GPL-2.0-or-later, GPL-3.0-only,
GPL-3.0-or-later, or another specific historical designation. Nor does the
project assume that the general Warajevo GPL statement overrides separate
third-party notices found in particular archived files.

See `LICENSE.txt` for the license granted by Supratim Sanyal for new original
Warajevo ZX Spectrum Next contributions and for the compatibility provision
for combining those contributions with Warajevo-derived material.

## 4. New Warajevo ZX Spectrum Next work

New original Warajevo ZX Spectrum Next code, documentation, tests, build
material, and other project-authored material created by Supratim Sanyal are:

Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs.

Those new contributions are licensed as stated in `LICENSE.txt`.

This copyright notice applies only to new copyrightable material created by
Supratim Sanyal. It does not replace, absorb, or supersede upstream Warajevo
copyrights or third-party copyrights.

## 5. Derivation and provenance policy

The original Warajevo source is an architectural and behavioral migration
reference for Warajevo ZX Spectrum Next.

Where a Warajevo ZX Spectrum Next implementation is copied from, translated
from, adapted from, or otherwise derived from original Warajevo code, that
provenance must remain identifiable and the applicable upstream copyright and
license obligations must be preserved.

A rewrite from Pascal or x86 assembly into C is not treated by this project as
a way to erase upstream provenance.

New code that is independently authored for Warajevo ZX Spectrum Next should
carry the project's new-work copyright notice and license identifier.

Where practical, source files derived substantially from a particular
Warajevo routine or module should record that provenance in a source comment
or associated design/provenance record.

## 6. Legacy third-party material

The historical Warajevo source archives contain material with separate
copyright notices or licensing terms that are not established by the general
Warajevo GPL statement.

The preserved-source audit identified examples including:

- `CHAIN.PAS` — TurboPower Software;
- `EDITORS.PAS` — Borland International;
- `GADGETS.PAS` — Borland International;
- `HELPFILE.PAS` — Borland International;
- `OPNDLG.PAS` — Borland International;
- `DETECT.PAS` — C. L. Burke, with portions credited to Borland;
- `TVGRAPH.PAS` — associated with C. L. Burke's TVGRAPH code;
- `TAPE2TAP.ASM` — Rui Fernando Ferreira Ribeiro;
- historical binary utilities containing Borland- or PKWARE-related material.

Warajevo ZX Spectrum Next must not assume that these components are licensed
for reuse merely because the Warajevo project as a whole was later described
as GPL.

The preferred policy for the modern project is to avoid importing legacy
third-party implementation code unless its redistribution and modification
rights are separately established. Functionality should instead be replaced
with newly written portable code or with a separately licensed modern
dependency whose terms are compatible with the project.

Preserved historical copies, when retained solely for provenance or research,
must keep their original notices and should remain clearly separated from new
implementation code.

## 7. Sokol

Warajevo ZX Spectrum Next is designed to use Sokol for portable host
presentation, input, audio, timing, and related host services.

When Sokol source is vendored into this repository, it remains separately
licensed under the license supplied by the Sokol project. Its copyright and
license notice must be preserved with the vendored source, for example under
`third_party/sokol/`.

Sokol is not original Warajevo code and is not owned by SANYALnet Labs merely
because it is compiled into a Warajevo ZX Spectrum Next executable.

## 8. ROMs, firmware, software images, and media

The Warajevo ZX Spectrum Next license does not grant rights to ZX Spectrum ROM
images or other firmware.

It also does not grant rights to games, applications, tape images, disk
images, snapshots, manuals, fonts, artwork, sound recordings, or other
third-party material.

Such material must not be committed to, bundled with, or redistributed by the
project unless its rights and distribution terms are separately established.

Users may be required to supply legally obtained ROM or media files where
applicable.

## 9. Names, marks, and endorsement

Use of the names **Warajevo**, **ZX Spectrum**, **Spectrum**, or names of
emulated peripherals and software describes historical provenance,
compatibility, or emulated functionality.

No affiliation with or endorsement by the original Warajevo authors, any
hardware manufacturer, any software publisher, or any owner of related
trademarks is claimed by this notice.

This notice does not grant trademark rights.

## 10. Distribution and contributor notices

Do not remove applicable copyright, license, authorship, provenance, or
warranty notices from upstream or third-party material.

New contributors should clearly identify the copyright holder for their new
contributions and use licensing terms compatible with `LICENSE.txt` and with
any GPL-covered Warajevo-derived material to which the contribution is
combined.

A public release should include all license texts and third-party notices
required by the code and assets actually shipped in that release.

## 11. Licensing uncertainty

The absence of a historical GPL version number in the upstream Warajevo
statement is a known provenance fact. GNU GPL versions 1, 2, and 3 each
contain an express rule for programs that do not specify a GPL version number,
but applying that rule does not resolve the separate question of whether every
file in the historical archives is covered by the Warajevo GPL grant.

If stronger upstream evidence is found, the project should update its license
metadata and release packaging to reflect that evidence while preserving all
rights already granted by the relevant copyright holders.

Questions about whether a particular historical file is covered by the
Warajevo GPL statement, a separate third-party license, or another legal
status should be resolved before that file is incorporated into distributed
Warajevo ZX Spectrum Next code.

## 12. Legal note

This file is a project provenance and licensing notice. It records the
project's intended licensing policy and the evidence presently available to
the maintainers. It is not legal advice.
