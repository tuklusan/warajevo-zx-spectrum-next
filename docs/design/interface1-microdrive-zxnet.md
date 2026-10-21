<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Interface 1, Microdrive, and original ZX Net contract

Status: frozen deterministic emulator contract for Architecture #1, Phase 10.
Physical Microdrive bit-cell and motor timing are not claimed: no physical
hardware is available. Where the manuals leave behavior unspecified, Fuse at
the pinned revision below is the project's closest emulator comparator. Fuse
behavior is a compatibility reference, not a physical timing measurement. This
document does not authorize redistribution of ROM images and does not specify
the separate routed MIC/EAR system.

## 1. Evidence order

Use evidence in this order when behavior conflicts:

1. Sinclair Interface 1/Microdrive manuals and service procedures, followed by
   observable original hardware behavior when available.
2. Identified Interface 1 ROM revision disassemblies for ROM-visible behavior.
3. Fuse's pinned Interface 1 implementation as an emulator-to-emulator
   comparator when physical behavior is unavailable; it does not prove
   hardware timing conformance.
4. The preserved Warajevo implementation at
   `reference/original-warajevo/source/src/spectrum-kernel/MDRIVE.ASM` for
   migrated emulator behavior and differential cases.
5. This document's deterministic definitions where historical emulation used
   host time or undocumented shortcuts.

The Fuse comparator is pinned to commit
`7edc2418ce11837d9ede799f7e0445440beead1d`, file
`peripherals/if1.c`, SHA-256
`e075056d71dbac41a266629b24afbd590e912b92cb20e483b2bda379c6065b2a`.
Fuse is GPL-2.0; this project uses that file as a test/reference authority and
does not copy or link its implementation into WZSN.

The 1983 Sinclair manual documents a daisy chain of up to eight drives and
drive-numbered BASIC operations. The service manual's port table defines the
three Interface 1 ports and their signal bits. The local ROM disassemblies
identify two revisions; their upstream provenance and licensing notes are in
`reference/rom-disassemblies/README.md`.

## 2. Device availability and ownership

`NONE`, `INTERFACE1`, and reserved `EAR_MIC` are one mutually exclusive machine
mode, as defined by Architecture #1 §24.1. Only `INTERFACE1` exposes this
device. `EAR_MIC` remains unavailable until its separately authorized
architecture is complete; it must not expose Interface 1 or ZX Net behavior.

All register, motor, media-position, and network transitions belong to the
deterministic machine and are ordered on the canonical master-tick timeline.
No wall-clock timer, filesystem operation, socket, worker, or UI callback may
mutate device state. Host media bytes and persistence callbacks stay outside
the deterministic device boundary. Writable media has one process-wide
exclusive owner. Ejection, mode changes, reset, and destruction must resolve
dirty media through an explicit successful flush or an explicit local discard
where the owning workflow allows discard; failure leaves the old mounted state
intact.

## 3. Interface 1 ROM and paging

The Interface 1 firmware image is an 8 KiB ROM mapped at `0000h–1FFFh` while
paged. It overlays the selected Spectrum ROM; it does not add RAM or change the
Spectrum memory map. Interface 1 pages in when execution reaches main-ROM
entry `0008h` (error handling) or `1708h` (`CLOSE#`). At `0008h`, it parses the
rejected BASIC statement and handles it or returns the original error. At
`1708h`, it handles the close operation. The Interface 1 unpage path reaches
`0700h`; the hardware switches back to the main ROM as the CPU executes the
`RET` there. Channel input/output can page the Interface 1 ROM temporarily and
must use the same unpage path. Paging preserves the main ROM image and machine
state outside the ROM selection latch.

The firmware identity is the pair `(revision, SHA-256 of exactly 8192 bytes)`.
Only an explicitly identified supported revision may be selected. Unknown or
wrong-length images fail before machine state changes. Do not embed firmware in
public release artifacts until Task 201's rights decision permits it; a
user-supplied image is the default distribution path. The two initial revision
labels are `OLD` and `NEW`, corresponding to the separately identified
first-revision and v2 disassemblies. Compatibility tests must exercise each
revision independently; revision selection may not be guessed from unrelated
machine state.

When Interface 1 is selected, reset initializes the control latch to `EEh`,
previous latch to `EEh`, motor shift register to `FFh`, no active motor, and ROM
page state to the main ROM. Cold mode reconfiguration discards the prior
machine context as specified by §24.1. A rejected transition or ROM selection
does not partially mutate the machine.

## 4. I/O decode and register contract

Interface 1 partially decodes the low five address bits. Addresses whose low
five bits equal `E7h`, `EFh`, or `F7h` select the corresponding register; upper
address bits are not part of this device's selection. The bus still records the
full CPU address and canonical bus-cycle tick.

| Low-five-bit port | Direction | Register behavior |
|---|---|---|
| `E7h` (`231`) | read/write | Microdrive serial data byte; reads and writes follow the control latch and selected motor. |
| `EFh` (`239`) | read | Status: bit 0 write protect (active high per the service manual), bit 1 sync, bit 2 gap, bit 3 DTR, bit 4 busy; bits 5–7 read high. |
| `EFh` (`239`) | write | Control latch: bit 0 communications data, bit 1 communications clock, bit 2 read/write (`1` read, `0` write), bit 3 erase (`0` erase asserted), bit 4 CTS, bit 5 wait; bits 6–7 are retained but have no device effect. |
| `F7h` (`247`) | read/write | Network/serial data register; with communications-data low, bit 0 is ZX Net input/output. With it high, bit 7 is serial input and bit 0 serial output; serial host redirection remains LATER per Architecture §1. |

The service manual says the WR PROT line is normally low, and the selected
drive pulls it high when the write-protect switch is closed. WZSN therefore
reports EFh bit 0 high for protected media and low for writable/unmounted
media. The pinned Fuse source clears bit 0 when its cartridge is protected;
this known divergence follows the primary service manual and is covered by the
write-protection contract.

The control latch stores the complete written byte. DTR and CTS are RS-232
signals at bits 3 and 4; CTS is not a Microdrive signal. The daisy-chain selection
is a serial shift-register operation: COMMS CLK clocks COMMS DATA through each
drive, and the selected stage asserts COMMS OUT. The hardware exposes one
selected drive at a time; the emulator must preserve the physical one-hot
selection semantics across all eight positions. The preserved Warajevo code
models this as an active-low eight-bit mask and clocks on a falling COMMS CLK
edge; that representation is not the electrical COMMS OUT polarity. A single
selection maps to drive 1–8; no selection or multiple selections yields no
usable drive. Motor numbers in UI/API calls are one-based. `E7h` transfers
serial Microdrive data, one byte per eligible port access in the frozen Fuse
emulator profile. If there is no selected mounted drive, the bus returns the
profile idle value `0xff`; Microdrive data availability does not assert the
network WAIT line. Writes follow bits 2/3 and selected-drive write-protect
state; they may not silently accept data in the wrong direction. UI operations
may run while the machine is paused to resolve a missing cartridge, but they
cannot inject data from another thread.

The Interface 1 bus handlers own all three ports while that mode is active.
They must preserve unrelated ULA, printer, 128K paging, and host bus observer
semantics. The 48K test addresses use the Spectrum's full-width `IN`/`OUT`
cycles; mirrored addresses prove the five-bit decode.

## 5. MDR image, sectors, and transfer state

An MDR image is an ordered byte stream of complete 543-byte sectors. The
variable-length Warajevo form contains 9 through 254 sectors and has no trailer,
matching the preserved loader's accepted range. The fixed 254-sector
compatibility form may append one write-protection byte; nonzero means protected
and omission means writable subject to the host's filesystem claim. The trailer
is valid only after all 254 sectors. A partial sector or any other trailing
length is a parse error.

Each sector consists of a 15-byte header and a 528-byte record block. On real
tape, each portion has a 12-byte synchronization preamble and an intervening
gap; preambles and gaps are not stored in MDR files. Header offsets are: flag
0, logical sector number 1, unused bytes 2–3, cartridge name 4–13, modulo-255
checksum of bytes 0–13 at 14. Record-block offsets relative to its start are:
record flag 0, sequence number 1, little-endian length 2–3, file name 4–13,
modulo-255 descriptor checksum at 14, 512 data bytes at 15–526, and modulo-255
checksum over all 512 data bytes at 527. The record flag uses bit 0 to mark a
record, bit 1 for EOF, and bit 2 to distinguish PRINT files; remaining bits are
zero in canonical records. A malformed checksum makes that header/record
unusable as a valid block (Warajevo's historical transport treats an invalid
header as a gap); it never authorizes unchecked indexing.

The two preambles and gaps are synchronization/spacing behavior, not image
bytes. Image parsers validate length/optional protection metadata without
assuming each field is naturally aligned. Header and record payloads are never
accessed through unaligned casts. Malformed records remain reportable and do
not cause out-of-bounds access.

Each of the eight slots owns a separate image identity, write-protect state,
sector position, header/data phase, byte offset, motor state, and staged dirty
sector. Selecting another drive preserves the unselected drive's transport
position and dirty data. A single MDR image cannot occupy two writable slots in
the same process; the host layer must also acquire an exclusive interprocess
write claim before exposing a writable mount. Read-only duplicate mounts may
be offered only when they cannot share mutable transport state.

The frozen timing profile is transaction-based, matching the pinned Fuse
emulator model: no host timer or wall clock advances a drive. Control-latch
edges, data-port accesses, and status-port accesses are processed in CPU bus
order and carry their canonical `master_tick`. Each eligible E7h read consumes
one byte from the mounted MDR stream; a write consumes one byte only when the
selected drive is writable and the control latch selects write mode. Sector
order is stored header bytes, stored record bytes, then the next sector,
wrapping at the image's sector count. The 12-byte preambles and gaps are not
MDR bytes. Fuse models formatted-sector GAP/SYNC through counters initialized
to 15 and advanced by EFh status reads, and consumes byte data on E7h accesses;
it does not model bit-cell duration or motor spin-up. WZSN follows those
observable transaction semantics and serializes every counter and position
that can affect a later bus read. The WR PROT polarity divergence is the
explicit exception described in §4. Physical bit-cell duration, motor
acceleration, and sector period remain unclaimed rather than guessed. Any future
physical timing profile requires new evidence and a deliberate spec revision.

Erase/format gaps, invalid sector checksums, end-of-sector advancement, motor
changes, and write-protect status are explicit device state, included in
serialization and canonical hashing. Writes stage a complete sector and do not
change the host image until the application-controlled atomic flush callback
succeeds. A failed callback keeps both the mounted image and dirty staged bytes
available for retry or explicit discard.

Warajevo's timer callback and artificial preamble behavior are compatibility
evidence, not permission to use host wall time. Tests compare legacy byte,
header/data, motor, status, and write sequences where the old implementation is
defined; manual behavior wins over an emulator difference. Fuse supplies a
reproducible fallback comparator only where primary evidence is silent. No
cycle duration or gap value is inferred from UI speed or host time, and no test
using Fuse is described as a physical hardware capture.

## 6. ROM revision compatibility cases

The first and v2 ROM disassemblies are maintained as separate revision
authorities. For each revision, regressions cover:

- reset and initial main-ROM visibility;
- an unrecognized BASIC statement entering the Interface 1 `RST 08h` path;
- recognized and rejected Interface 1 commands, including return to the main
  ROM and correct error reporting;
- temporary paging for channel input/output and return through unpage;
- Microdrive drive numbers 1–8 and absent/invalid drive behavior;
- Interface 1 reset, Spectrum reset, and cold networking-mode transition;
- ROM-image identity, wrong size, unsupported identity, and failed transition
  atomicity.

The old/new split is a firmware compatibility choice, not a separate device
model. Register and media behavior remain hardware-owned and deterministic in
both revisions.

## 7. Original ZX Net state machine

The deterministic device states are `CLAIM`, `IDLE`, `BUSY`, `FREE`,
`COLLREAD`, and `COLLWRITE`. Initialization uses a five-read busy interval, a
five-read free interval, a 30-unit inter-block delay setting, and a 256-byte
payload buffer. A read block is accepted only as exactly 256 bytes and only
when no prior read block is pending. `IDLE` compares the delivered block ID to
the last ID to select `FREE` or `BUSY`; consuming that status interval returns
to `IDLE`. Accepted claims begin collection; declined claims report the free
interval. Collection preserves the original least-significant-bit-first byte
rotation and nine-clock framing. Successful completed writes advance the block
ID only after the write callback succeeds; callback failure preserves the
collection state for deterministic retry.

The core has no host transport. Any later transport submits complete 256-byte
blocks through the application normalization queue in §24.5, which stamps
nondecreasing master ticks and sequence numbers; equal-tick delivery is FIFO.
Queue overflow, malformed size, or stale ordering rejects the whole enqueue
without mutation. Delivery occurs on the core owner thread when the stamped
tick is due. Tests cover every state transition, duplicate/changed IDs,
bit/byte boundaries, callback failure/success, wraparound IDs, queue ordering,
and queue saturation.

EFh control bit 5 is the Interface 1 WAIT output. When the control latch
requests WAIT, the Spectrum's I/O cycle remains pending until the ZX Net state
machine supplies the awaited signal; this is represented in canonical
machine-tick state, never by blocking a host thread. Microdrive reads do not
use this line. F7h network input/output is selected only while COMMS DATA is
low; the serial side remains electrically idle in the initial architecture
because historical RS-232 host redirection is LATER.

The legacy delay value is not interpreted as wall-clock time. Deterministic
machine-visible status lengths are read-count based; any future clocked network
delay must be explicitly specified in master ticks and validated against
hardware/reference captures before changing the contract.

## 8. Canonical state and serialization

Canonical serialization includes, in fixed-width little-endian form:

- networking mode;
- Interface 1 ROM presence, revision, SHA-256 identity, selected ROM page,
  current/previous control latch, motor shift register, active motor, and the
  master tick of the last control-latch write;
- for each drive 1–8: mount flag, image identity/length/sector count, sector,
  byte offset, header/data phase, active/write/erase/dirty flags, and the full
  staged sector buffer when dirty;
- ZX Net state, claim byte, current and last block IDs, frozen timing/status
  counters, byte/bit positions, pending-read state, buffer length/contents.

Host pointers, callbacks, file paths/handles, locks, dialogs, sockets, temporary
buffers, and persistence implementation details are excluded. Absent media has
canonical zero identity/length/count and reset transport position. Deserialization
validates enum/range/size/identity consistency before publishing any state; a
failed load leaves the destination machine unchanged. Any field-layout change
increments the canonical state format version.

## 9. Regression and acceptance authorities

The required evidence is:

1. Primary-manual port and paging cases; where the manual is ambiguous and
   physical hardware is unavailable, compare against the pinned Fuse emulator.
   A Fuse comparison is not a hardware-conformance claim.
2. Old and v2 ROM revision tests using exact 8 KiB user-provided images whose
   SHA-256 and revision are pinned in the test driver. Test images are not
   copied into public release artifacts unless Task 201 establishes rights.
3. MDR parser/transport tests for short, oversized, truncated, malformed,
   protected, writable, dirty, failed-flush, and successful-flush images;
   eight-slot isolation, single-writer rejection, and exact round-trip bytes.
4. Deterministic bus tests for mirrored ports, latch edges, zero/one/multiple
   motor selection, all status bits, both transfer directions, sector wrap,
   master-tick ordering, and canonical state round trips.
5. ZX Net transition, framing, block callback, loopback, and normalized queue
   tests, with no host packet timing in core tests.
6. Differential comparisons against the pinned Fuse implementation and
   preserved Warajevo MDRIVE routines only for behaviors that do not conflict
   with primary manual evidence. Every intentional divergence is named in the
   proof and trace.

This specification freezes the deterministic emulator contract and resolves
the reference hierarchy. Task 201's test-image policy is in
`interface1-rom-test-policy.md`. The Phase-10 gate remains open until the
implementation tasks pass hosted tests and pin proof records with source commit
and fixture hashes. Authentic-ROM tests remain unavailable unless an operator
provides the corresponding ROM image out of tree; their absence must never be
reported as a pass.

## Sources

- Sinclair Research, *Microdrive and Interface 1 Manual* (first edition, 1983):
  <https://trastero.speccy.org/cosas/manuales/ZXInterface1_Microdrive_Manual.pdf>
- Sinclair Interface 1/2 and Microdrive Service Manual (reproduced scan, used
  for service tests and port-bit reference):
  <https://spectrumforeveryone.com/wp-content/uploads/2017/08/ZX-Interface-1-2-Microdrive-Service-Manual.pdf>
- Preserved Warajevo MDR-format documentation:
  <https://worldofspectrum.net/warajevo/Fformats.html>
- MDR file-format checksum and descriptor reference:
  <https://worldofspectrum.org/faq/reference/formats.htm>
- Upstream Interface 1 ROM disassembly provenance:
  `reference/rom-disassemblies/README.md`.
- Fuse Interface 1 comparator, pinned to commit
  `7edc2418ce11837d9ede799f7e0445440beead1d`:
  <https://sourceforge.net/p/fuse-emulator/fuse/ci/7edc2418ce11837d9ede799f7e0445440beead1d/tree/peripherals/if1.c>
- Preserved Warajevo Microdrive/Interface 1/ZX Net migration reference:
  `reference/original-warajevo/source/src/spectrum-kernel/MDRIVE.ASM` and
  `reference/original-warajevo/source/src/spectrum-kernel/SPECSIM.ASM`.
- Parent architecture and policy:
  `01-warajevo-zx-spectrum-next-architecture.md`, §§24, 29, 49, 51.
