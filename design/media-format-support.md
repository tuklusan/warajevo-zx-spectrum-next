<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Media format support matrix

This document freezes the initial tape and snapshot media contract required
by Architecture #1 §§23.12 and 25.1–25.3. The architecture remains the
authority. This matrix constrains parser behavior; an implementation detail
must not silently broaden the supported formats.

The matrix is being frozen after parser implementation began. Its presence
does not retroactively prove that the Phase-7/8 prerequisite ordering was
followed. Existing code must be checked against this matrix before its format
acceptance is called complete. New format behavior requires an explicit
architecture decision and an update here before implementation.

## 1. Tape formats

| Format | Identify | Read / normal playback | Write | Other behavior |
|---|---|---|---|---|
| Standard Spectrum TAP | Sequence of little-endian 16-bit block lengths; a block is flag, payload, and XOR checksum. If the native signature below is absent, `.tap` is parsed as standard TAP. | Required. Expand each block with the 48K PAL standard pilot, sync, bit, and pause timings into canonical master-tick EAR segments. A data block with flag `0xFF` stops the transport at its end. | Required. Emit the defined length prefix, payload, and XOR checksum for each non-empty block. | Zero-length blocks, truncated lengths/data, checksum errors, overflow, or malformed block sequences fail the load; do not skip or repair them silently.
| Warajevo native TAP | Signature-detected native header/record chain; a matching signature selects this parser and does not fall back to standard TAP after a malformed native parse. | Required compatibility path. Validate record offsets, predecessor/successor links, record lengths, and extended-record headers before exposing records. Preserve record payload bytes exactly. | Required compatibility path. Emit the canonical linked-record header and supported record types 0–5; reject inconsistent lengths or links. | Unknown record types and malformed/truncated chains fail with a controlled parse error. Native records do not become timed EAR edges unless a documented conversion supplies that representation.
| TZX 1.x | `ZXTape!`, end marker `0x1A`, major version 1, then length-delimited blocks. | Required. Supported timing/control blocks below produce a single ordered timeline. Non-machine metadata blocks below are retained/recognized but do not affect machine state. Unknown blocks fail with an unsupported-format result. | Not required. | Validate every block boundary, jump/select target, and loop pairing before mounting. A malformed file fails atomically. Block names/IDs follow the [TZX 1.20 specification](https://worldofspectrum.net/TZXformat.html).
| RIFF/WAVE PCM | `RIFF`/`WAVE` container with PCM format code 1 and a `fmt ` plus `data` chunk. | Required deterministic decode. Accept interleaved unsigned 8-bit or signed 16-bit PCM with one or more channels, non-zero sample rate, and consistent frame/block/byte rates. Average channels per frame, then apply fixed threshold 128 and hysteresis 8. Map each sample frame to `floor(frame × profile master Hz / sample rate)` ticks; emit only positive-duration level segments. | No. | Skip bounded, even-padded unknown RIFF chunks. Reject compressed/float PCM, other sample widths, inconsistent headers, truncated chunks, empty data, and unsupported containers (including RF64). No resampling or host-audio-device access is involved.
| VOC and other sampled-audio containers | — | Later; not an initial blocker. | No. | Reject as unsupported until separately specified.

### 1.1 TZX block disposition

The parser must use these frozen dispositions. **Supported** means the block
is length-validated and its timing or control behavior is implemented.
**Ignored metadata** means the block is length-validated but cannot change the
machine or tape signal. Every other ID is **unsupported with controlled
error**; it must not be guessed or skipped.

| ID | Block | Disposition |
|---:|---|---|
| `10` | Standard speed data | Supported |
| `11` | Turbo speed data | Supported |
| `12` | Pure tone | Supported |
| `13` | Pulse sequence | Supported |
| `14` | Pure data | Supported |
| `15` | Direct recording | Supported |
| `18` | CSW recording | Supported |
| `19` | Generalized data | Supported |
| `20` | Pause / stop | Supported |
| `21` | Group start | Ignored metadata |
| `22` | Group end | Ignored metadata |
| `23` | Relative jump | Supported; target must be in-file |
| `24` | Loop start | Supported; loops must balance and remain within implementation limits |
| `25` | Loop end | Supported |
| `26` | Call sequence | Supported; every target must be in-file |
| `27` | Return from sequence | Supported |
| `28` | Select block | Supported; every selection target must be in-file |
| `2A` | Stop tape if in 48K mode | Supported |
| `2B` | Set signal level | Supported |
| `30` | Text description | Ignored metadata |
| `31` | Message block | Ignored metadata |
| `32` | Archive information | Ignored metadata |
| `33` | Hardware type | Ignored metadata; it does not select a machine profile |
| `35` | Custom information | Ignored metadata |
| `40` | Deprecated snapshot block | Ignored metadata; it does not load a snapshot |
| `5A` | Glue block | Ignored metadata |
| All others | Reserved, deprecated, or unimplemented IDs | Unsupported with controlled error |

## 2. Snapshot formats

Snapshot load is transactional: parse and validate into temporary state, then
commit the complete candidate. Any structural, range, model, or unsupported
hardware-mode failure leaves the live machine unchanged. Save must reject a
state that the selected format cannot represent; it must not silently lose
state.

| Format / variant | Load | Save | Frozen byte-level policy |
|---|---|---|---|
| SNA 48K | Required | Required | Exactly `27 + 49152` bytes. The 27-byte register header is followed by 48K RAM. PC is recovered from the word at the saved stack pointer; SP is advanced by two. Accept IM 0–2, border 0–7, and the defined IFF encoding only. No trailing data or extension is accepted. |
| SNA 128K | Required | Required | Exactly `27 + 49152 + 4 + 5×16384` bytes. The initial three RAM pages represent banks 5, 2, and the currently paged bank; the extension carries PC, 7FFD paging value, and TR-DOS state; remaining banks follow. Reject paging values selecting bank 2/5 for the third page, reserved paging bits, invalid TR-DOS values, missing pages, or trailing/truncated data. |
| Z80 v1, 48K | Required | No | A 30-byte v1 header with non-zero PC followed by exactly 48K RAM, either raw or ED-ED run-length encoded. RLE must produce exactly 49152 bytes; malformed, short, overlong, or unsupported marker sequences fail. |
| Z80 v2, 48K | Required | Canonical initial `.z80` writer | PC is encoded in the extended header. The 48K RAM pages are page IDs 8, 4, and 5; each appears once and is either raw 16K or a bounded compressed page. The writer emits the canonical v2-class form and refuses unrepresentable machine state. |
| Z80 v2, 128K | Required | Required | All eight 16K RAM pages are represented once using the v2 page records; validate page IDs, page lengths, compression expansion, paging registers, and required machine fields before commit. The writer emits the defined v2-class page set or fails. |
| Z80 v3, 48K | Required | No | Accept only extended-header lengths explicitly implemented by the parser (54 or 55 bytes) and the 48K page IDs 8, 4, and 5. Unsupported hardware-mode fields, duplicate/missing pages, malformed compression, or trailing partial records fail. |
| Z80 v3, base 128K | Required | No | Accept version-3 extended-header hardware mode 4 and page IDs 3–10 exactly once, with each page raw or bounded ED-ED RLE data. Validate the 54/55-byte extended-header size, paging state, page set, compression expansion, and full input consumption. |
| Z80 v3, other hardware modes | Not required | No | Reject with a controlled unsupported-format result. Do not infer +2/+3, Timex, Pentagon, or other hardware from an unknown mode byte. |
| Warajevo Timex Z80 extension and other historical snapshots | Later | Later / no | Not an initial release requirement. No partial import or silent profile substitution. |

SNA and Z80 contain no project-owned Interface-1 or `EAR_MIC` routed-network
state. Import initializes the project networking mode to `NONE`; export never
claims to preserve an external network device or invents a networking ROM.
These snapshot decisions do not define or implement Architecture #3.

## 3. Acceptance evidence

The media corpus, TAP/TZX playback, WAV GUI mounting, snapshot inspection,
snapshot load/save, and atomic save/load proofs remain the behavioral evidence
for these contracts. Their current proof records are listed in
`test-results/` and the Architecture-#1/2 acceptance audit. This document
freezes policy; it does not replace the required malformed-input, round-trip,
timing, and cross-platform tests. Any mismatch between the matrix and the
implementation is an implementation gap to fix, not permission to weaken this
matrix silently.
