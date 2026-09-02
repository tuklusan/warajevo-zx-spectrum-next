<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Media Format Support

This file translates the current architecture disposition into a compact
implementation planning table.

| Format or facility | Initial disposition | Authority |
| --- | --- | --- |
| TAP | Required | Core architecture |
| TZX | Required | Core architecture |
| WAV tape input | Required | Core architecture |
| SNA 48K and 128K | Required | Core architecture |
| Z80 snapshots | Required | Core architecture |
| MDR microdrive images | Required | Core architecture |
| Interface 1 state and media semantics | Required | Core architecture |
| Original ZX Net behavior | Required | Core architecture |
| Timex/DCK media and Dock UI | Later | Core and UI architecture |
| Historical conversion utilities | Later unless required by tests | Core and UI architecture |

The architecture documents remain the source of truth. This file is a working
summary for early repository and backlog organization.

## Phase-7 Frozen Tape Matrix

The following dispositions are the parser contract. `SUPPORTED` means the
block contributes deterministic tape edges or control flow. `IGNORED` means
the block is retained only as non-machine metadata. `UNSUPPORTED` means the
parser must return a controlled error without partially mounting or mutating
tape state.

| TZX ID | Block | Disposition |
| --- | --- | --- |
| `10` | Standard speed data | SUPPORTED |
| `11` | Turbo speed data | SUPPORTED |
| `12` | Pure tone | SUPPORTED |
| `13` | Pulse sequence | SUPPORTED |
| `14` | Pure data | SUPPORTED |
| `15` | Direct recording | SUPPORTED |
| `16` | C64 ROM type data | UNSUPPORTED |
| `17` | C64 turbo data | UNSUPPORTED |
| `18` | CSW recording | SUPPORTED |
| `19` | Generalized data | SUPPORTED |
| `20` | Pause or stop | SUPPORTED |
| `21` | Group start | IGNORED |
| `22` | Group end | IGNORED |
| `23` | Jump to block | SUPPORTED |
| `24` | Loop start | SUPPORTED |
| `25` | Loop end | SUPPORTED |
| `26` | Call sequence | SUPPORTED |
| `27` | Return from sequence | SUPPORTED |
| `28` | Select block | SUPPORTED |
| `29` | Reserved/unknown | UNSUPPORTED |
| `2A` | Stop tape if in 48K mode | SUPPORTED |
| `2B` | Set signal level | SUPPORTED |
| `30` | Text description | IGNORED |
| `31` | Message | IGNORED |
| `32` | Archive information | IGNORED |
| `33` | Hardware type | IGNORED |
| `34` | Reserved/unknown | UNSUPPORTED |
| `35` | Custom information | IGNORED |
| `36`-`3F` | Reserved/unknown | UNSUPPORTED |
| `40` | Emulation information | IGNORED |
| `41`-`59` | Reserved/unknown | UNSUPPORTED |
| `5A` | Glue block | IGNORED |
| other | Unknown block ID | UNSUPPORTED |

The parser must validate each block's declared and variable-length fields
before allocation or state publication. A malformed supported block, an
invalid jump/call/loop target, a non-terminating control path, or a truncated
block is a controlled error, not an implicit skip. Metadata blocks may be
parsed for their length and then ignored as machine input.

### TAP semantics

Standard TAP is identified by its two-byte little-endian block length followed
by exactly that many bytes. Each block is expanded into the canonical Spectrum
pilot/data pulse sequence in Normal mode; the final byte is the XOR checksum
but is not silently repaired. Truncation, impossible lengths, and checksum
failure are reported as load errors before the tape object becomes visible.
Standard TAP writing emits the same length-prefixed blocks and preserves the
defined data bytes. Warajevo-native TAP is a separate signature-detected
compatibility format and must never be conflated with standard TAP.

### WAV semantics

WAV input is read-only tape input. The initial accepted forms are RIFF/WAVE
files with PCM integer samples, one or more channels, and sample rates that
can be represented without overflow by the deterministic decoder. The decoder
converts samples to a defined mono level, applies the frozen threshold and
hysteresis rules, and emits EAR edges on canonical master time. Unsupported
codecs, floating-point/companded formats, malformed chunks, integer overflow,
and truncated sample data are controlled errors. Host playback rate and audio
device state never enter the tape object or canonical machine state.
