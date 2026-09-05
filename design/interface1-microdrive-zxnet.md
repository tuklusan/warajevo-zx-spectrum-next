<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Interface 1, Microdrive, and ZX Net Specification

Status: FROZEN_FOR_IMPLEMENTATION

This document freezes the Phase-10 core contract for authentic Sinclair
Interface 1, Microdrive cartridge images, and the original Warajevo ZX Net
behavior. It does not specify the later Architecture-#3 routed MIC/EAR
network, the host Telnet service, or a second machine-state implementation.

## 1. Authority and scope

The primary preserved behavioral authorities are:

- `reference/original-warajevo/source/src/spectrum-kernel/MDRIVE.ASM`, the
  complete preserved Interface-1/Microdrive/ZX Net implementation;
- `reference/original-warajevo/source/src/spectrum-kernel/SPECSIM.ASM`, the
  I/O dispatch, Interface-1 activation, ROM presence, and snapshot restoration
  integration;
- `design/warajevo-zx-spectrum-next-architecture.md`, sections 24, 25, 26,
  36, 38, 39, 40, and 49.1;
- `design/warajevo-zx-spectrum-next-ui-architecture.md`, sections 5, 11, 14,
  15, 23.5, 24, 25, 39, 44, and 48.

The C implementation must preserve Spectrum-visible behavior and canonical
timing, but must not copy assembly or expose DOS handles, host paths, sockets,
or printer objects through the deterministic core.

## 2. Networking modes and ownership

The machine has exactly one `networking_mode` value:

```text
NONE       no Interface 1, Microdrive, or original ZX Net
INTERFACE1 authentic Interface 1, Microdrive, and original ZX Net active
EAR_MIC    reserved downstream mode, unavailable before Architecture #3
```

`INTERFACE1` and `EAR_MIC` cannot coexist. Selecting a different mode is a
cold machine reconfiguration: the old machine context, RAM, paged ROM state,
resident hooks, and Interface-1 devices are discarded and a new context is
created. Application pause/run state is retained. Selecting the active mode is
idempotent. Leaving Interface 1 requires successful application-controlled
flush or cancellation when writable Microdrive media is dirty.

## 3. Port decode

When Interface 1 is active, decode the low-byte port aliases using the same
address masking convention as the existing bus:

| Port low byte | Direction | Function | Core result |
| --- | --- | --- | --- |
| `$E7` | IN/OUT | Microdrive data | sector byte stream |
| `$EF` | IN/OUT | Interface-1 status/control | status bits and latch transitions |
| `$F7` | IN/OUT | network/RS232 data | network state machine or explicit RS232 boundary |

The preserved dispatch table in `SPECSIM.ASM` maps these ports by their low
five address bits, with unrelated ports retaining the normal user/bus path.
The new bus must record the full port address and canonical master tick, then
apply the same alias rule. RS232 is not silently substituted for ZX Net: the
control-latch data bit selects the explicit serial boundary, which remains
unsupported until separately specified and tested.

## 4. ROM paging and ROM variants

Interface-1 activation is a machine configuration operation, not an arbitrary
host memory overlay. The implementation must provide an explicit Interface-1
ROM identity and presence state, validate the ROM before activation, and expose
the ROM in the machine address map only while the Interface-1 mode owns it.

The old and new Interface-1 ROM choices are distinct identities. Snapshot
restore may request an Interface-1 ROM/page state only when the selected ROM is
present and compatible; otherwise it fails atomically with a controlled error.
The normal 48K/128K ROM remains restorable after Interface 1 is disabled. ROM
selection, activation, and shadow/page transitions participate in canonical
state serialization and hashing.

The preserved source proves ROM presence/variant checks and activation through
`OPTIONE`, `ZXI1P`, `ZXI1OLD`, `SETZXI1`, and snapshot restoration. Exact ROM
bytes are governed by the separate ROM provenance and redistribution CR; this
specification does not authorize embedding unlicensed ROM material.

## 5. Interface-1 control latch

The `$EF` control latch is eight bits wide. Bits 5 through 7 are reserved and
must be stored only according to the project’s defined reserved-bit policy.
The functional bits are:

| Bit | Name | Frozen behavior |
| --- | --- | --- |
| 0 | COMMS DATA | selects serial/network data and supplies the motor shift bit |
| 1 | COMMS CLOCK | a `1 -> 0` transition shifts the motor register |
| 2 | R/W' | `0` permits Microdrive write, `1` permits read |
| 3 | ERASE | `0` enables erase/GAP behavior, `1` normal transport |
| 4 | WAIT | network collection/transfer control |

Reset value is `$EE`, matching preserved `MDVINIT`. Every write is a
master-timestamped latch event. A changed Microdrive functional bit flushes the
pending sector/cache state before the new mode takes effect. A falling edge on
COMMS CLOCK shifts COMMS DATA into an eight-bit motor register; exactly one
selected motor is valid. Zero or multiple selected motors means no active
cartridge and an empty/error device state.

## 6. Microdrive image and sector interpretation

An MDR image is a sequence of sectors, each exactly 543 bytes:

```text
bytes 0..11     lead-in
bytes 12..26    15-byte header
bytes 27..542   528-byte data block
```

The preserved implementation alternates header and data-block interpretation,
wraps at the validated cartridge sector count, and accepts cartridge lengths
from 9 through 254 sectors inclusive. The image length must be an exact
multiple of 543 and malformed, truncated, overlong, or overflowed images are
controlled errors. The cartridge slot identity, length, write protection, and
current sector are deterministic state.

Each cartridge has an independent slot, name/identity, write-protect state,
and open/mounted state. At most one of the eight motors may be active. An
unmounted slot or invalid motor selection returns the defined inactive/error
behavior and cannot mutate image bytes.

## 7. Data/status behavior

`IN $E7` returns zero when the device is inactive, the buffer is empty, the
current sector is a GAP, erase is active, or the latch is not in read mode.
Otherwise it returns the next buffered byte and advances the bounded pointer.
`OUT $E7` accepts a byte only when the device is active, not in GAP, ERASE is
clear, and R/W' selects write; the byte is buffered and the pointer saturates
at 543 rather than overrunning storage.

`IN $EF` exposes the preserved status semantics: write-ready/protection,
synchronization, GAP, and RS232-DTR indicators with the defined inactive and
reserved high bits. Status reads advance the deterministic GAP/sector state as
required by the ROM loader. A checksum failure marks the sector as GAP; it is
not silently repaired.

## 8. Timing, buffering, and flush

Sector transitions, status-driven GAP windows, motor changes, erase behavior,
data-byte visibility, and network bit collection are driven by canonical
emulated master ticks. Host wall time, file latency, and host packet arrival
must never directly change device state.

The reference uses an eight-sector cache. The portable implementation may use a
different container, but must preserve observable ordering and dirty-state
semantics: load the containing eight-sector window, flush dirty bytes before a
cartridge/window change, truncate the final window to its validated length, and
clear dirty state only after a successful atomic media commit. A failed flush
leaves the in-memory dirty state and original image recoverable.

When a buffered write is committed, headers copy 15 useful bytes and data
blocks copy 528 useful bytes; the 12-byte lead-in is not overwritten. Erase
creates the preserved invalid-checksum GAP representation through the normal
flush path. Motor stop/update and application shutdown invoke the same
controlled flush operation and never perform hidden untracked writes.

## 9. Original ZX Net state machine

Original ZX Net is a deterministic Interface-1 device, distinct from the
Architecture-#3 routed network and from the host multi-instance Control Port.
Its state values are:

```text
CLAIM, IDLE, BUSY, FREE, COLLREAD, COLLWRITE
```

`IN $F7` in network mode returns the preserved state-dependent byte behavior:
CLAIM returns the claim byte and returns to IDLE; BUSY/FREE return their
defined status byte for the configured count; COLLREAD rotates the current
network byte and decrements the bit count; IDLE obtains the next two-byte block
identity and selects BUSY for a new identity or FREE for the last identity.

`OUT $F7` in network mode implements claim/acceptance and bit collection. The
WAIT control-latch event starts or advances collection; a completed write
flushes a 256-byte bounded block through the network-device boundary. Network
state transitions, block IDs, bit counts, sequence positions, delay, BUSY
length, and FREE length are deterministic serialized fields.

The reference uses a shared `.NET` file as its interprocess medium. WZSN must
not perform that file operation inside the core. The core exposes a bounded
loopback/network-device interface; an application adapter may translate file or
future host transport input into timestamped device events. A single-process
loopback harness must reproduce all state transitions without filesystem or
packet timing.

## 10. Serialization and hashing

The canonical native state includes, when applicable:

- `networking_mode` and Interface-1 ROM identity/presence/active page;
- control-latch value, previous latch value, motor shift register, active slot;
- all mounted slot identities, validated lengths, write-protect/dirty state,
  current sector/header phase, data pointer, GAP/checksum state, and bounded
  cache contents needed for exact replay;
- ZX Net state, claim byte, block/last IDs, bounded network buffer,
  bit/byte positions, and deterministic timing counters.

Historical SNA/Z80 formats that cannot represent active Interface-1 state must
produce a controlled incompatibility result rather than silently discarding it.
Serialization and hashing are endian-explicit and independent of pointers,
handles, host paths, file timestamps, sockets, and thread scheduling.

## 11. Regression and differential authorities

Before implementation closure, tests must cover:

- every `$E7`, `$EF`, and `$F7` alias and unrelated-port preservation;
- latch reset, every functional bit, falling-edge motor shift, zero/multiple
  motor selection, and mode transitions;
- MDR length validation, header/data alternation, checksum/GAP behavior,
  read/write/protection, erase, wrap, cache boundaries, final short window,
  failed flush, and atomic remount/eject;
- all six ZX Net states, claim/acceptance, repeated BUSY/FREE counts,
  collision read/write, bounded malformed input, and loopback timing;
- serialization/hash round trips, wrong-mode rejection, and no partial
  mutation on failed load or networking-mode change;
- preserved Warajevo differential cases wherever behavior is comparable, with
  hardware/reference evidence taking precedence over an emulator quirk.

The second upstream zero-gap scan must re-read all `MDRIVE.ASM` routines,
Interface-1 dispatch/activation in `SPECSIM.ASM`, ROM variant handling,
snapshot integration, and every test authority before the Phase-10 gate closes.

## 12. Explicit non-scope

This freeze does not implement or decide:

- Architecture-#3 MIC/EAR routing, router behavior, or its resident software;
- host Telnet/control-port transport;
- physical RS232, printer hardware, or undocumented analog behavior;
- arbitrary host-file access from deterministic core code;
- silent lossy snapshot conversion or unlicensed ROM redistribution.

Any change to the frozen port, latch, sector, timing, state, serialization, or
authority contract requires a new architecture CR.
