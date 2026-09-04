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

## Phase-8 Frozen Snapshot Matrix

Snapshot loaders must parse into the isolated temporary state object and must
not publish any field to the live machine until the complete input has passed
length, range, model, and representability checks. All unrecognized or
unsupported variants are controlled errors; no best-effort truncation is
permitted.

| Format / variant | Exact accepted form | Compression / paging | Disposition |
| --- | --- | --- | --- |
| SNA 48K | 27-byte header plus exactly 49,152 RAM bytes; total 49,179 bytes | Uncompressed; RAM maps 0x4000-0xffff | SUPPORTED |
| SNA 128K | 27-byte base header plus paging extension and complete 16 KiB pages | Uncompressed; page-selection fields must be validated | SCAFFOLD_REQUIRED |
| Z80 v1 | 30-byte header plus exactly 48 KiB RAM, or compressed payload with valid end marker | Optional v1 RLE; 48K model only | SUPPORTED_INPUT |
| Z80 v2 | 30-byte base header, 23-byte extended header, and length-prefixed 16 KiB pages | Per-page uncompressed or Z80 RLE | SUPPORTED_INPUT_AND_CANONICAL_SAVE |
| Z80 v3 | 30-byte base header, 54-byte or 55-byte extended header, and validated pages | Per-page uncompressed or Z80 RLE | SUPPORTED_INPUT |
| Timex/other hardware variants | Any header or hardware code outside the frozen 48K/128K set | Variant-specific paging or device state | UNSUPPORTED_CONTROLLED_ERROR |

The SNA 48K header fields are read in the historical order: I, alternate and
main register pairs, IY, IX, IFF2, R, F, A, SP, IM, and border. The loader
recovers PC from the little-endian word at the current SP and advances the
temporary SP by two; a missing or out-of-range stack word is invalid. IFF1 is
initialized from IFF2 because SNA does not carry an independent IFF1 field.
The border value is limited to the three-bit color and IM is limited to 0, 1,
or 2. The 48K RAM bytes are copied only after all header and stack checks pass.

For Z80 input, v1 PC is the base-header PC and v2/v3 PC is the extended-header
PC. A compressed stream must terminate exactly at the declared page boundary;
truncated runs, overlong runs, missing v1 end markers, duplicate page numbers,
invalid page lengths, and trailing bytes are controlled errors. The initial
canonical writer is the frozen Z80 v2 48K representation and must reject any
active state covered by the historical representability predicate.

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
