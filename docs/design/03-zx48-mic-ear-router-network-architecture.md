<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# ZX48 MIC/EAR Router Network - System Architecture

This document is the canonical architecture authority for the ZX48 MIC/EAR
Router Network. It is Architecture #3 in the Warajevo ZX Spectrum Next (WZSN)
program and is intentionally implemented only after the first two architecture
workstreams are complete and their applicable acceptance gates pass:

```text
1. design/warajevo-zx-spectrum-next-architecture.md
2. design/warajevo-zx-spectrum-next-ui-architecture.md
3. design/zx48-mic-ear-router-network-architecture.md
```

The WZSN core/system architecture remains authoritative for emulated machine
timing, the canonical master-tick timeline, port-FE behavior, cassette EAR/MIC
state, machine serialization, deterministic scheduling, and core/host
boundaries. The WZSN UI architecture remains authoritative for WZSN menus,
command-registry behavior, host presentation, local/remote application control,
and WZSN Telnet control semantics. This document may consume those contracts;
it may not redefine them.

This architecture owns the separate routed MIC/EAR network feature: the
Spectrum-resident stack, router protocol, Raspberry Pi router service,
deterministic external timing controller, router-port electronics, physical
network behavior, virtual proof harness, and the integration boundary through
which WZSN can simulate that physical network.

This feature is **not** authentic Interface 1 ZX Net emulation and is not the
WZSN dynamically allocated Telnet Control Port service (base 30740). Those remain separate features
under Architectures #1 and #2. If Architecture #3 later exposes interactive WZSN
controls, they must enter through the UI architecture's shared application
command registry rather than creating a private GUI/Telnet control path.

The physical target is the Sinclair ZX Spectrum 48K, Issue 2 only. The runtime
network link uses only the unmodified machine's MIC and EAR cassette sockets.
The logical router is Raspberry Pi-class hardware assisted by deterministic
external timing hardware for multi-port edge capture and waveform generation.

The Spectrum-side software is delivered as a standard Spectrum tape image
and/or router-generated tape waveform and exposes only an Interface 1-style
BASIC front end.

Accuracy and electrical safety are the canonical priorities. A convenience
shortcut is not permitted to replace pulse-level behavior in the fidelity path.

### Document-state conventions

- **[FROZEN]** - a decision that implementation must preserve unless the
  architecture is explicitly revised.
- **[BASELINE]** - the implementation choice to use unless a named decision
  gate replaces it before dependent tickets are assigned.
- **[PROPOSED]** - a candidate design that requires confirmation at a named
  gate before implementation depends on it.
- **[OPEN]** - an intentionally unresolved engineering decision. Every open
  item must be mapped to Section 32 and must be closed before the specified
  phase gate. An un-gated open item is an architecture defect.

The source baseline for this architecture was the 7-bit-ASCII Revision-01
system design dated 2026-08-06. This Markdown document supersedes that TXT file
as the implementation authority while preserving its scope and terminology.

---

## 1. Executive Summary

**[FROZEN]**

The network is built around original Sinclair ZX Spectrum 48K Issue 2 machines.
Each Spectrum uses:

- its MIC socket as an uplink to a central router; and
- its EAR socket as a downlink from the central router.

Each socket is a mono two-conductor connection:

- signal; and
- ground/return.

No Spectrum MIC outputs are tied together. No Spectrum EAR inputs are tied
together. Every machine has an independent router port.

The physical topology is an active star:

                          +-----------------------+
  ZX 1 MIC -------------->| RX 1                  |
  ZX 1 EAR <--------------| TX 1                  |
                          |                       |
  ZX 2 MIC -------------->| RX 2   ROUTER         |
  ZX 2 EAR <--------------| TX 2   SYSTEM         |
                          |                       |
  ZX n MIC -------------->| RX n                  |
  ZX n EAR <--------------| TX n                  |
                          +-----------------------+

The Raspberry Pi performs logical routing, station naming, address resolution,
message queueing, file staging, status management, and administration.

Deterministic pulse capture and pulse generation are delegated to external
microcontroller, PIO, CPLD, or FPGA hardware. The Raspberry Pi must not depend
on ordinary Linux process scheduling to toggle or sample cassette pulses.

The Spectrum-facing BASIC syntax remains Interface 1-like. The system does not
introduce commands such as NET, MSG, WHO, FILES, or GET. Existing Interface 1
forms are extended to accept a station designator N:

```text
N = "x.y" or "AAAAAA"
```

Examples:

```text
FORMAT "n";"MASTER"
SAVE *"n";"GAME02" CODE 32768,4096
LOAD *"n";"1.3" CODE 32768
OPEN #4;"n";"GAME02"
PRINT #4;"READY"
CLOSE #4
```

Station names are exactly six alphanumeric characters and are unique. The
router maps each name to a two-byte address consisting of:

- one area byte; and
- one node byte.

The high-memory Spectrum network stack is loaded from tape. It hooks the normal
48K BASIC error path so Interface 1-like network syntax can be recognized from
RAM, in the same general user-facing manner that Interface 1 extended BASIC,
but without ROM paging hardware. Because an active Interface 1 owns overlapping
BASIC-extension/error-path semantics and the same `"n"` vocabulary, the initial
Ear+Mic environment requires Interface 1 to be absent; coexistence/chaining is
not part of this architecture.

Incoming messages are asynchronous at the router, not at the full-payload
Spectrum receiver. The router can queue a message at any time. It then sends a
slow attention indication on the destination Spectrum's EAR line. A small
resident interrupt-time detector sets a "message waiting" flag. The full
message is retrieved cooperatively through an Interface 1-style network stream
when the Spectrum is ready to poll the EAR input precisely.

---

## 2. Scope


### 2.1 Included

**[FROZEN]**

This design covers:

- Sinclair ZX Spectrum 48K Issue 2 machines only.
- Two mono connections per Spectrum:
      MIC signal plus return.
      EAR signal plus return.
- A central active router.
- A Raspberry Pi router implementation.
- An external deterministic timing/interface subsystem.
- A tape-loaded, high-memory Spectrum networking stack.
- An Interface 1-style BASIC front end.
- Two-byte area-plus-node addresses.
- Unique six-character alphanumeric station names.
- Routed point-to-point communication.
- Reserved address space for future broadcast/service use where explicitly frozen.
- Stream-oriented messages.
- Spectrum object transfer through SAVE *, LOAD *, VERIFY *, and MERGE *.
- Asynchronous message queueing and attention notification.
- Interface 1-like success and error feedback.


### 2.2 Excluded from the current baseline

**[FROZEN]**

The following are out of scope unless a later revision explicitly adds them:

- ZX Spectrum 128K models.
- ZX Spectrum +2, +2A, +2B, or +3.
- ZX Interface 1 electrical compatibility.
- Coexistence of the resident Ear+Mic BASIC front end with an active Interface 1;
  the initial architecture requires them to be mutually exclusive.
- ZX Net wire-level compatibility.
- Replacement ROM hardware.
- Expansion-port hardware.
- Directly tying multiple MIC outputs together.
- A shared analog cassette bus.
- Full-duplex operation at one Spectrum.
- Unsolicited transfer of complete packets while arbitrary BASIC or game code
    is running.
- A new modern BASIC command vocabulary.
- Background multitasking.
- Internet routing.
- Encryption.
- Offline delivery to powered-off or physically disconnected Spectrums.
- Broadcast delivery in the initial release, unless a Phase-0 architecture revision
  adds complete broadcast routing, queueing, command, and error semantics.
- Final electrical component values.
- Final bit rate.
- Final RAM footprint.


### 2.3 Compatibility statement

**[BASELINE]**

The system imitates the Interface 1 user-facing programming model, not the
Interface 1 physical network. Its physical layer is routed MIC/EAR audio-level
signalling through a central active star.

### 2.4 Interface 1 mutual-exclusion rule

**[FROZEN]**

The initial Ear+Mic resident BASIC front end is not supported concurrently with
an active ZX Interface 1. Both extend 48K BASIC through overlapping error/parser
semantics and both claim Interface-1-style `"n"` forms. Architecture #3 does not
attempt handler chaining, shadow-ROM cooperation, or dual ownership of that
syntax.

On physical Issue-2 machines used for Architecture-#3 acceptance, Interface 1
must therefore be absent/inactive while the Ear+Mic stack and BASIC front end
are installed.

In WZSN, this rule is structural: the core architecture's single networking
mode is `NONE`, `INTERFACE1`, or `EAR_MIC`. Architecture #3 operates only when
`EAR_MIC` is selected; selecting it guarantees that Interface 1, Microdrive,
and original ZX Net are inactive. No Architecture-#3 ROM is paged.

---

## 3. Goals and Non-goals


### 3.1 Goals

**[FROZEN]**

G1. Operate with an unmodified Issue 2 48K Spectrum.

G2. Use only the MIC and EAR cassette sockets for the runtime network link.

G3. Allow many Spectrums to connect safely without tying outputs together.

G4. Preserve an Interface 1-like user experience.

G5. Support human-readable unique station names.

G6. Support compact hierarchical area-plus-node addresses.

G7. Provide meaningful feedback for:
      success;
      malformed station identifiers;
      unknown stations;
      known but unavailable stations;
      busy stations;
      timeouts;
      transport corruption;
      incompatible protocol versions;
      rejected transfers; and
      failed verification.

G8. Allow incoming messages to be queued asynchronously at the router.

G9. Notify a Spectrum whose resident stack and interrupt contract remain active
    that traffic is waiting without corrupting the command line, BASIC program,
    or compatible application state.

G10. Permit the Spectrum stack to be distributed and installed as a tape.

G11. Separate the reusable network core from the BASIC front end.

G12. Make the same protocol usable in a simulator or emulator by modelling MIC
     output edges and EAR input levels.


### 3.2 Non-goals

**[FROZEN]**

NG1. Reproduce the Interface 1 hardware bus.

NG2. Reproduce every Interface 1 firmware bug.

NG3. Add multitasking to the Spectrum.

NG4. Make the Spectrum receive an entire packet in the background.

NG5. Allow arbitrary user-selected source addresses without router validation.

NG6. Treat the Raspberry Pi GPIO subsystem as a real-time pulse engine.

NG7. Provide a modern filesystem shell on the Spectrum.

NG8. Invent commands outside the Interface 1 front end.

NG9. Transparently coexist or chain the Ear+Mic BASIC-extension hook with an
     active Interface 1 in the initial release.

NG10. Page a dedicated Ear+Mic networking ROM in WZSN; the distributed resident
      RAM stack remains the Spectrum-side implementation.

---

## 4. Terms and Conventions


### 4.1 Spectrum

" Spectrum " means an original 48K Issue 2 machine unless otherwise stated.


### 4.2 Router

The logical router service running on the Raspberry Pi.


### 4.3 Timing controller

External deterministic hardware that converts between:

- conditioned MIC waveform edges and framed bytes; and
- framed bytes and conditioned EAR waveform edges.


### 4.4 Router port

One independent bidirectional logical connection consisting of:

- one MIC uplink receiver;
- one EAR downlink transmitter;
- one port identity;
- one set of status counters; and
- one queue binding.


### 4.5 Station

One Spectrum registered with the router.


### 4.6 Station name

Exactly six alphanumeric characters. The canonical alphabet is:

  A through Z
  0 through 9

Canonical storage is uppercase.

Examples:

  MASTER
  GAME01
  LAB002
  ZX0048

Invalid examples:

  GAME1       five characters
  GAME-1      hyphen is not alphanumeric
  GAME_1      underscore is not alphanumeric
  GAME001     seven characters


### 4.7 Station address

A two-byte logical address:

  byte 0: area
  byte 1: node

Human-readable form:

  x.y

Examples:

  1.1
  1.3
  12.42


### 4.8 Station designator N

**[FROZEN]**

`N` denotes an ordinary station and is a quoted string in one of two forms:

```text
"x.y"
"AAAAAA"
```

The six-character lexical shape does not make `"ROUTER"` an ordinary station;
that value is the reserved router endpoint defined in Section 11.4. Likewise,
`"0.0"` is the numeric router endpoint, not an assignable ordinary station.

Examples of ordinary station designators:

```text
"1.3"
"GAME02"
```


### 4.9 Message

A router-staged character stream committed by closing an output channel.


### 4.10 Spectrum object

A transferable object represented by normal Spectrum SAVE/LOAD semantics:

  BASIC program
  BASIC program with LINE autostart metadata
  numeric array
  character array
  CODE block
  SCREEN$


### 4.11 Asynchronous

In this document, "asynchronous incoming" means:

- the router can accept and queue traffic at any time;
- the router can indicate pending traffic asynchronously;
- the Spectrum retrieves the full payload cooperatively.

It does not mean that the Spectrum receives a full message while unrelated code
continues to execute.

---

## 5. System Architecture


### 5.1 Top-level layers

**[FROZEN]**

  +--------------------------------------------------------------+
  | Spectrum BASIC front end                                     |
  | FORMAT, SAVE *, LOAD *, VERIFY *, MERGE *, OPEN #, PRINT #,   |
  | INPUT #, INKEY$ #, CLOSE #, MOVE                             |
  +--------------------------------------------------------------+
  | Resident high-memory network stack                           |
  | parser hooks, channels, object transfer, message handling     |
  +--------------------------------------------------------------+
  | Packet and link services                                     |
  | framing, sequencing, CRC, retries, turnaround                 |
  +--------------------------------------------------------------+
  | Issue 2 ULA cassette interface                               |
  | MIC output through port FE, EAR input through port FE bit 6   |
  +--------------------------------------------------------------+
  | Independent router port electronics                          |
  +--------------------------------------------------------------+
  | Deterministic timing controller                              |
  +--------------------------------------------------------------+
  | Raspberry Pi router                                          |
  | registry, resolution, queues, routing, storage, administration|
  +--------------------------------------------------------------+


### 5.2 Trust boundaries

**[BASELINE]**

The router trusts:

- the physical router port identity;
- its own station registry;
- CRC-validated frames from the timing controller.

The router does not trust:

- a Spectrum-provided source address;
- a Spectrum-provided source name;
- an unvalidated packet length;
- an unvalidated protocol version;
- a payload that exceeds configured limits.


### 5.3 Separation of responsibilities

Spectrum:

- generates and receives pulse-coded traffic;
- presents Interface 1-like BASIC semantics;
- stores only minimal active state and buffers;
- explicitly initiates full payload reception;
- exposes a callable machine-code API.

Timing controller:

- measures MIC edge timing;
- generates EAR edge timing;
- owns deterministic per-port timing;
- prevents Linux scheduling jitter from entering the physical layer;
- reports link diagnostics.

Raspberry Pi:

- owns station registry and uniqueness;
- maps name to area.node address;
- maps port to authenticated station identity;
- routes packets;
- queues messages;
- stages object transfers;
- tracks status;
- provides administration and logs.

---

## 6. Physical Topology


### 6.1 Per-station cabling

**[FROZEN]**

Each station uses two mono TS connections.

Uplink:

  Spectrum MIC tip    -> router RX signal input
  Spectrum MIC sleeve -> router port return

Downlink:

  router TX signal output -> Spectrum EAR tip
  router port return       -> Spectrum EAR sleeve

The two Spectrum sleeves are common inside the Spectrum. The router design must
take that into account.


### 6.2 Active star

**[FROZEN]**

The router presents an independent RX/TX path to each Spectrum.

No analog signal is passively fanned out.

No two Spectrum MIC outputs are electrically connected.

No two router EAR drivers are electrically connected.


### 6.3 Half-duplex Spectrum behavior

**[FROZEN]**

Although uplink and downlink use separate sockets and separate router circuits,
one Spectrum is treated as half-duplex at the protocol level.

The Spectrum software must not be required to transmit and receive at the same
time.


### 6.4 Simultaneous activity across stations

**[BASELINE]**

Different stations may operate concurrently because their analog paths are
independent.

Examples:

  Station 1 may upload while Station 2 uploads.
  Station 3 may receive while Station 4 receives.
  The router may service multiple ports concurrently.

Concurrency is limited by timing-controller resources and router software, not
by a shared analog bus.

---

## 7. Spectrum-side Electrical Boundary


### 7.1 Uplink source

**[FROZEN]**

The Spectrum transmits by controlling the cassette/audio output through port
FE. The router observes the resulting MIC waveform.


### 7.2 Downlink input

**[FROZEN]**

The Spectrum receives by reading EAR state through port FE bit 6 while running
a timing-sensitive polling loop.


### 7.3 No raw logic-level connection

**[BASELINE]**

A Raspberry Pi, microcontroller, CPLD, or FPGA pin must not be connected
directly to the Spectrum EAR or MIC socket.

Every port requires analog conditioning.


### 7.4 Electrical design requirements

**[BASELINE]**

The uplink receiver shall provide:

- high input impedance;
- AC coupling where required by the final circuit;
- input protection;
- amplitude limiting;
- threshold detection;
- hysteresis;
- tolerance of plug insertion transients;
- tolerance of modest amplitude variation among old machines.

The downlink transmitter shall provide:

- controlled audio-level output;
- AC coupling where required by the final circuit;
- current limiting;
- output protection;
- repeatable transition timing;
- adjustable or selectable amplitude during bring-up;
- safe behavior during plug insertion and removal.


### 7.5 Grounding and isolation

**[PROPOSED]**

Prototype:

  A shared router-side ground may be used for a small supervised bench
  prototype after verifying power-supply relationships.

Production-oriented design:

  Each router port should have an isolated analog domain or another proven
  method that prevents ground loops and fault propagation among several
  forty-year-old computers.

**[OPEN]**

The final isolation method, isolation voltage, isolated power topology, and
connector protection network require schematic design and bench measurement.

---

## 8. Router Port Electronics


### 8.1 Uplink receive chain

**[BASELINE]**

  Spectrum MIC
      |
      v
  protection and current limiting
      |
      v
  AC coupling or level restoration
      |
      v
  amplifier or high-gain buffer
      |
      v
  Schmitt comparator
      |
      v
  digital edge capture input


### 8.2 Downlink transmit chain

**[BASELINE]**

  timer or waveform engine
      |
      v
  logic output
      |
      v
  level conversion and waveform shaping
      |
      v
  current-limited protected driver
      |
      v
  Spectrum EAR


### 8.3 Per-port status signals

**[BASELINE]**

Each port exposes at minimum:

- cable present or inferred activity;
- last MIC edge timestamp;
- current decoded state;
- uplink framing error count;
- uplink CRC error count;
- downlink queue depth;
- downlink active state;
- current station binding; and
- analog threshold diagnostics when the selected hardware exposes them.


### 8.4 Port count

**[BASELINE]**

The logical protocol does not embed a physical port count. Hardware scales by
replicating the independent per-station RX/TX port contract.

The implementation sequence is:

- Phase 2: one supervised diagnostic port is sufficient;
- Phase 3: two independent ports are required;
- Phase 5: the multi-port reference build provides at least four ports; and
- Phase 7: the production PCB port count is frozen from measured controller,
  isolation, power, connector, and cost constraints.

---

## 9. Deterministic Timing Controller


### 9.1 Requirement

**[FROZEN]**

The Raspberry Pi is the router computer, but ordinary Linux user-space timing
must not directly generate or measure the cassette pulse train.


### 9.2 Acceptable implementations

**[BASELINE]**

One of the following may own the real-time layer:

- microcontroller with hardware timers and input capture;
- RP2040-style programmable I/O;
- CPLD;
- FPGA;
- another deterministic multi-channel timing device.

The concrete timing-controller technology is selected at the Phase-2 entry gate.
The protocol and Spectrum stack may not depend on vendor-specific timing
semantics that are absent from the architecture contract.


### 9.3 Controller responsibilities

**[BASELINE]**

For each port, the controller shall:

- timestamp MIC transitions;
- classify pulse widths;
- detect pilot/sync/framing;
- assemble validated link frames or report frame errors;
- queue received frames to the Pi;
- accept outbound frames from the Pi;
- generate deterministic EAR transitions;
- generate the slow attention signal;
- suppress attention signalling during active packet reception, transmission,
  and router-generated tape/bootstrap waveforms;
- enforce the defined output idle state;
- enter safe idle on Pi/controller transport loss, controller reset, watchdog
  expiry, or unrecoverable underrun; and
- report overruns, underruns, resets, and link-loss faults.


### 9.4 Pi-controller transport

**[PROPOSED]**

The Pi and timing controller may communicate through:

- SPI;
- USB;
- UART at a sufficiently high and reliable rate; or
- another framed digital interface.

The Pi-controller protocol should include:

- port number;
- operation type;
- frame length;
- frame payload;
- monotonic sequence number;
- controller status;
- CRC or another integrity check.


### 9.5 Buffering

**[BASELINE]**

The controller must buffer complete timing-critical work independently of short
Linux scheduling delays. At the frozen maximum supported physical rate, buffer
capacity and flow-control limits must be proven sufficient by the Phase-2/5
stress tests; buffer exhaustion must fail a transaction explicitly rather than
corrupting an in-progress pulse train.

---

## 10. Raspberry Pi Router Software


### 10.1 Process model

**[BASELINE]**

A primary daemon, `zx48-routerd`, owns the logical router. The listed module
boundaries are architectural responsibilities; their exact source-file split is
frozen with repository placement at Phase 0.

Suggested modules:

  zx48-routerd
    port manager
    station registry
    name resolver
    address allocator
    packet router
    stream/message service
    object transfer service
    queue manager
    storage manager
    timing-controller adapter
    administration API
    diagnostics and logging


### 10.2 Required router functions

**[FROZEN]**

The router must:

- enforce unique six-character names;
- maintain name-to-address mapping;
- maintain address-to-name mapping;
- maintain physical-port-to-station mapping;
- distinguish unknown from known-but-unavailable stations;
- route direct traffic;
- support router-local services;
- queue incoming messages for online stations that are not currently
    receiving;
- notify stations with queued traffic;
- validate packet length and CRC;
- prevent source impersonation;
- return status and error responses.


### 10.3 Administrative interface

**[PROPOSED]**

Administration is outside the Spectrum BASIC front end. The initial
implementation must provide a local-only administrative path, such as a command
line or loopback/local IPC interface. Any later network-reachable web or
management API must require explicit authentication/authorization and may not
ship as an unauthenticated wildcard listener.

Possible presentation mechanisms include:

- local command-line tools;
- a loopback-only local web interface;
- configuration files;
- a serial console; or
- an authenticated/restricted management API.

The Phase-5 administrative interface allows at minimum:

- port enable/disable;
- station name assignment;
- address assignment;
- conflict resolution;
- queue inspection;
- log inspection;
- protocol-version control;
- firmware update of timing hardware;
- link diagnostics.


### 10.4 Persistence

**[BASELINE]**

SQLite is the baseline metadata store for the initial implementation.

Persistent information includes, as applicable:

- stations;
- committed messages;
- object-transfer staging metadata;
- router configuration;
- audit log metadata; and
- protocol metadata.

Payload data may be stored as database blobs or as files referenced by database
metadata. Whichever representation is chosen must provide atomic visibility:
metadata must never advertise a committed message/object whose required payload
is absent or only partially written.

---

## 11. Station Identity and Addressing


### 11.1 Station names

**[FROZEN]**

A station name:

- is exactly six characters;
- uses only A-Z and 0-9;
- is unique within the router's naming domain;
- is stored canonically in uppercase; and
- may not equal the reserved service designator `ROUTER`.

**[BASELINE]**

Lowercase station-name input is normalized to uppercase before validation in
the initial front end. Canonical storage and comparison remain uppercase.


### 11.2 Address structure

**[FROZEN]**

A station address is two bytes:

```text
byte 0: area, 0..255
byte 1: node, 0..255
```

The human-readable notation is decimal:

  area.node

Example:

  1.3


### 11.3 Name/address mapping

**[FROZEN]**

The name is not mathematically encoded into the address.

The router stores an explicit mapping:

  GAME02 <-> 1.3


### 11.4 Reserved service identity and address space

**[FROZEN]**

The local router service has two equivalent designators:

```text
"ROUTER"
"0.0"
```

`ROUTER` is a reserved service name and may not be assigned to an ordinary
station. Address `0.0` is a reserved router-service address and may not be
assigned to an ordinary station.

**[PROPOSED]**

The remaining address-space reservation plan is:

```text
x.0       area control or reserved
x.1-x.254 ordinary station nodes
x.255     area broadcast
255.x     reserved service and future routing space
255.255   global broadcast
```

**[OPEN]**

The exact remaining broadcast/service reservations must be frozen at the
Phase-0 specification gate before the packet format is declared stable.
Broadcast delivery itself is not part of the initial release unless Phase 0
adds explicit command, routing, queueing, and error semantics for it.


### 11.5 Physical identity

**[FROZEN]**

The router knows the physical input port on which every uplink frame arrived.

The Spectrum must not be trusted to assert its own source identity.


### 11.6 Source field policy

**[BASELINE]**

The initial protocol uses the simpler authenticated-port form:

- the Spectrum omits its source address from the uplink routed-packet header;
- the router inserts the source address from the authenticated physical-port
  binding;
- no Spectrum-supplied source name or address is authoritative.

A future protocol version may carry an asserted source field only if the router
verifies it against the physical-port binding. That future form is not part of
the initial packet format.


### 11.7 Registration

**[BASELINE]**

A station must successfully establish local identity before ordinary network
operations.

The Interface 1-style front end uses:

```text
FORMAT "n";N
```

Examples:

```text
FORMAT "n";"MASTER"
FORMAT "n";"1.1"
```

For the Phase-1 through Phase-3 baseline, each physical/virtual port is
preconfigured with its permitted station name and address. `FORMAT` confirms
and activates that binding; it does not grant an arbitrary client the right to
claim another port's identity.

**[OPEN]**

Phase 0 must freeze the final registration policy for the production protocol,
including whether identities may be administratively reassigned, whether any
address allocation is dynamic, and what explicit sequence is required when a
station is moved to another physical port. Dependent packet/registration
tickets may not assume behavior beyond the preconfigured-binding baseline until
that decision is recorded.

---

## 12. Link Operation


### 12.1 General direction

**[FROZEN]**

Uplink:

  Spectrum MIC -> router RX

Downlink:

  router TX -> Spectrum EAR


### 12.2 Spectrum-initiated full downlink

**[FROZEN]**

The router shall not attempt to deliver a full packet unless the Spectrum has
entered a known receive operation.

Reason:

  The EAR input provides no independent packet-receive interrupt. Full
  reception requires a timing-sensitive polling loop.


### 12.3 Turnaround

**[BASELINE]**

A request-response exchange is:

  1. Spectrum transmits request through MIC.
  2. Spectrum stops transmitting.
  3. Spectrum establishes receive state.
  4. Spectrum enters EAR polling loop.
  5. Router waits a defined guard interval.
  6. Router sends response through EAR.
  7. Spectrum validates response.
  8. Spectrum acknowledges when required.

**[OPEN]**

The exact turnaround guard must be measured on real Issue 2 hardware.


### 12.4 Router queue semantics

**[FROZEN]**

For a known and online destination that is not currently receiving:

- the router may accept a completed message stream;
- the router queues it;
- the router signals pending traffic;
- the destination retrieves it later.

For a registered but offline or disconnected destination:

- the current baseline returns "Station not available";
- the current baseline does not silently accept offline delivery.


### 12.5 Busy state

**[BASELINE]**

Station `online` / `unavailable` state is a router lifecycle decision, not a
side effect of one failed receive window. The Phase-3 entry gate must freeze how
physical cable/activity evidence, successful registration, attention
acknowledgment, valid traffic, explicit disable, and liveness timeout contribute
to that state. Mere application idleness must not be confused with `busy`.

A station may be considered busy when:

- it has an incompatible active transfer;
- its downlink is already committed to another operation that requires immediate
  delivery;
- an operation explicitly requires a live receive window that is unavailable; or
- a frozen queue/staging policy refuses another object or stream.

A destination merely being online but not currently in a receive loop does **not**
make an ordinary queued character-message commit busy; that case is the purpose
of router queueing. The router returns `Station busy` rather than waiting
indefinitely only when the requested operation cannot be queued under its frozen
semantics.

---

## 13. Physical Modulation


### 13.1 Modulation class

**[BASELINE]**

The initial physical-link baseline uses a self-clocking, edge-timed, pulse-width
scheme resembling Spectrum tape encoding rather than raw asynchronous UART.
Exact pulse values remain gated by measurement as defined in Sections 30 and 32.

Each bit is represented by two half-pulses of equal duration.

  bit 0: short half-pulse + short half-pulse
  bit 1: long half-pulse  + long half-pulse


### 13.2 Conservative compatibility profile

**[PROPOSED]**

A conservative starting profile may reuse familiar Spectrum-style timings:

  pilot half-pulse: 2168 T-states
  sync pulse 1:      667 T-states
  sync pulse 2:      735 T-states
  zero half-pulse:   855 T-states
  one half-pulse:   1710 T-states

This profile is provisional. It must not be treated as final until tested on
multiple Issue 2 machines.


### 13.3 Direct-link pilot

**[PROPOSED]**

Because the link is active and direct, the pilot can be much shorter than a
cassette file pilot.

Initial test value:

  64 alternating half-pulses


### 13.4 Inter-frame idle

**[PROPOSED]**

A short idle period shall separate frames. Initial test value:

  at least 3 milliseconds


### 13.5 Faster profiles

**[OPEN]**

A later profile may reduce pulse widths after electrical and software margins
are measured. Protocol negotiation must prevent a station from receiving a
profile it does not support.


### 13.6 Bit order

**[BASELINE]**

Multi-byte fields use network order:

  most-significant byte first.

Bit order within a byte is most-significant bit first for the initial
protocol. A later protocol version may change encoding only through explicit
version negotiation; implementation-local convenience may not silently change
wire order.

---

## 14. Packet Framing


### 14.1 Layering

**[BASELINE]**

The system separates:

```text
physical pulse stream
link frame
routed packet
stream or object service
```

For the initial protocol, one physical/link frame carries exactly one routed
packet. The CRC field shown in the routed-packet format covers the routed-packet
header and payload according to the Phase-0 CRC definition and is the integrity
value the timing controller validates before reporting a received frame. There
is no second, silently different link-frame CRC in the initial protocol.


### 14.2 Uplink packet

**[PROPOSED]**

A compact uplink packet may contain:

  version       1 byte
  type          1 byte
  destination   2 bytes
  sequence      2 bytes
  length        2 bytes
  payload       0..N bytes
  CRC-16        2 bytes

The router obtains the source from the port binding.


### 14.3 Downlink packet

**[PROPOSED]**

A downlink packet may contain:

  version       1 byte
  type          1 byte
  source        2 bytes
  destination   2 bytes
  sequence      2 bytes
  length        2 bytes
  payload       0..N bytes
  CRC-16        2 bytes


### 14.4 Packet types

**[PROPOSED]**

Control:

  HELLO
  WELCOME
  ERROR
  POLL
  EMPTY
  ACK
  NAK
  STATUS

Identity:

  REGISTER
  REGISTER_REPLY
  RESOLVE
  RESOLVE_REPLY

Stream/message:

  STREAM_OPEN
  STREAM_OPEN_REPLY
  STREAM_DATA
  STREAM_CLOSE
  STREAM_CLOSE_REPLY
  STREAM_READ
  STREAM_READ_REPLY

Object transfer:

  OBJECT_OFFER
  OBJECT_OFFER_REPLY
  OBJECT_DATA
  OBJECT_DATA_ACK
  OBJECT_COMPLETE
  OBJECT_COMPLETE_REPLY
  OBJECT_ABORT

Notification:

  ATTENTION_ACK
  PENDING_STATUS


### 14.5 Payload size

**[PROPOSED]**

Initial general payload:

  32 or 64 bytes

Initial transfer control:

  stop-and-wait
  one outstanding data packet
  acknowledgment for every data packet
  bounded retry count

**[OPEN]**

Final payload size and retry timing require measurement.


### 14.6 Integrity

**[BASELINE]**

Each routed packet carried by a physical/link frame requires the packet CRC
defined at Phase 0.

A complete multi-packet message and a complete Spectrum object require their
respective frozen end-to-end integrity value, independent of per-packet CRCs.


### 14.7 Sequence numbers

**[BASELINE]**

Sequence numbers provide:

- packet-level duplicate detection;
- retry recognition;
- ordered chunks within a logical operation; and
- safe recovery after a lost packet acknowledgment.

Packet sequence alone is not the identity of a whole stream/message/object
transaction. Stream/object control messages carry or establish a bounded logical
`stream_id` / `transfer_id` (exact representation frozen at Phase 0/4) so that a
repeated close/commit or object-completion request can be recognized
idempotently within the retry/recovery window.

---

## 15. Router Routing and Queueing


### 15.1 Name resolution

**[FROZEN]**

Commands may identify a station by name or address.

For a name:

  1. validate six-character syntax;
  2. resolve through the router registry;
  3. return unknown if no mapping exists;
  4. return unavailable if mapping exists but station is down.


### 15.2 Direct routing

**[BASELINE]**

For a direct packet:

  1. authenticate source from input port;
  2. validate destination;
  3. validate packet type and length;
  4. locate destination state;
  5. route, queue, or reject;
  6. return explicit status.


### 15.3 Message queue

**[FROZEN]**

A stream committed through `CLOSE #` becomes one queued message record when the
commit succeeds.

The logical record includes at minimum:

```text
message identifier
commit sequence
source internal station ID
source address snapshot
source name snapshot
destination internal station ID
destination address snapshot
destination name snapshot
creation time assigned by router (diagnostic metadata, not queue order)
payload length
payload
end-to-end payload integrity value
delivery state
retry state
```

Queued ownership is bound to immutable router-internal station identity, not to
a mutable display name or address. Renaming/readdressing a station therefore
does not retarget already committed messages to another station. Administrative
station replacement/rebinding must explicitly decide the fate of pending data.

Queue ordering uses the monotonic commit sequence, not wall-clock timestamps.


### 15.4 Message delivery guarantee

**[BASELINE]**

`CLOSE #` returning `0 OK` means that the router accepted the complete message
for an online registered destination and committed both its metadata and
payload to the router's persistent queue before sending the success response.
A normal router-daemon restart after that response must not lose the message.

The exact storage-engine flush/fsync strategy is an implementation detail, but
it must satisfy the observable durability contract and the crash-recovery tests
in Section 28.

`0 OK` does **not** mean that the recipient has read the message.


### 15.5 Queue order and message deletion

**[BASELINE]**

For `OPEN #...;"ROUTER"` / `"0.0"`, pending messages are offered in stable
FIFO order by router commit sequence for the destination's immutable station
identity. Source-specific retrieval first resolves the current source designator
to an internal station identity, then uses FIFO order within that source subset.
The source name returned with a queued record is the commit-time name snapshot.
Retries of a partially delivered record do not move that record behind newer
eligible messages.


The router must not delete a queued message merely because transmission began.

Deletion or final delivery marking occurs only after:

- the Spectrum validates the complete message; and
- the close or acknowledgment exchange succeeds.

The initial release does not expire a successfully committed queued message by
wall-clock age automatically. It remains pending until successful delivery,
administrative removal, or an explicitly defined recovery action. Bounded queue
limits still apply to accepting **new** messages.


### 15.6 Object staging

**[PROPOSED]**

The router may stage an object while the sender uploads it. It should not expose
the object to the receiver until:

- every chunk is present;
- the final object checksum passes; and
- object metadata is valid.


### 15.7 Offline behavior

**[FROZEN]**

The initial release is not an offline mailbox for disconnected machines. A
queue commit requires the destination to be online at the authoritative router
check performed during the committing operation; there is no recent-online
grace period in the initial protocol.

Known but disconnected destination at new commit time:

  Station not available

If a destination disconnects **after** a successful commit, the already committed
message remains bound to that station's internal identity and is retained for
retrieval when that same station becomes available again, subject to the frozen
queue-retention/resource policy. This does not permit a new commit to an offline
destination.

Unknown destination:

  Station not found

---

## 16. Interface 1-style BASIC Front End


### 16.1 Frozen front-end rule

**[FROZEN]**

Only the Interface 1-style vocabulary is visible:

  FORMAT
  SAVE *
  LOAD *
  VERIFY *
  MERGE *
  OPEN #
  PRINT #
  INPUT #
  INKEY$ #
  CLOSE #
  MOVE

The design shall not add visible commands such as:

  NET
  MSG
  WHO
  FILES
  GET
  SEND
  SHARE


### 16.2 Device selector

**[FROZEN]**

The network device selector remains:

  "n"


### 16.3 Station designator

**[FROZEN]**

Every ordinary station-targeted network operation that names a station accepts
`N`:

```text
N = "x.y" or "AAAAAA"
```

`OPEN #` additionally accepts the reserved local-router endpoint `"ROUTER"` or
`"0.0"` for queued-message retrieval and router-local stream services defined
by this architecture. The reserved service name is not an assignable station
name.


### 16.4 Immediate and program use

**[BASELINE]**

The high-memory extension should support the Interface 1-like forms wherever
the ROM and extension architecture safely permit them.

Immediate command support is mandatory.

Stored BASIC program support is required for the normal Interface 1 forms used
by applications, but the exact tokenizer and execution-hook mechanism must be
validated during implementation.


### 16.5 No modern router leakage

**[FROZEN]**

The Spectrum user need not see:

- queue IDs;
- database IDs;
- router internal services;
- physical port numbers;
- Pi process names;
- transport packet types.

The visible abstraction remains:

  Spectrum objects
  BASIC streams
  stations
  normal completion or Interface 1-like reports

---

## 17. Command Semantics


### 17.1 FORMAT

Syntax:

  FORMAT "n";N

Examples:

  FORMAT "n";"MASTER"
  FORMAT "n";"1.1"

Purpose:

- establish or confirm local station identity;
- validate the local name or address;
- activate the router binding;
- store returned local identity in the resident stack.

Possible outcomes:

  0 OK
  Invalid station
  Missing station
  Station name in use
  Station not found, when a valid requested preconfigured identity does not exist
  Network not present
  Network timeout
  Network version error


### 17.2 SAVE *

Syntax:

  SAVE *"n";N
  SAVE *"n";N LINE line
  SAVE *"n";N DATA array()
  SAVE *"n";N DATA array$()
  SAVE *"n";N CODE address,length
  SAVE *"n";N SCREEN$

Purpose:

  Transfer a Spectrum object to the destination.

Completion:

  0 OK only after the router has accepted the complete, validated object under
  the current delivery semantics.

Possible outcomes:

  Station not set
  Station not found
  Station not available
  Station busy
  Network not present
  Network timeout
  Network data error
  Transfer rejected


### 17.3 LOAD *

Syntax:

  LOAD *"n";N
  LOAD *"n";N DATA array()
  LOAD *"n";N DATA array$()
  LOAD *"n";N CODE
  LOAD *"n";N CODE address
  LOAD *"n";N CODE address,length
  LOAD *"n";N SCREEN$

Purpose:

  Receive and load a Spectrum object supplied by the named source through the
  router.

The Spectrum explicitly enters receive mode.

Possible outcomes:

  Station not set
  Station not found
  Station not available
  Station busy
  Network not present
  Network timeout
  Network data error
  Transfer rejected


### 17.4 VERIFY *

Syntax:

  VERIFY *"n";N
  VERIFY *"n";N DATA array()
  VERIFY *"n";N DATA array$()
  VERIFY *"n";N CODE address,length
  VERIFY *"n";N SCREEN$

Purpose:

  Receive an object and compare it with memory.

Possible additional outcome:

  Verification has failed


### 17.5 MERGE *

Syntax:

  MERGE *"n";N

Purpose:

  Receive a BASIC program and merge it into the current program using the
  normal Spectrum MERGE concept.

Possible outcomes mirror LOAD *, plus BASIC merge-specific ROM errors where
applicable.


### 17.6 OPEN #

Syntax:

```text
OPEN #stream;"n";N
OPEN #stream;"n";ROUTER_ENDPOINT
```

Examples:

  OPEN #4;"n";"GAME02"
  OPEN #4;"n";"1.3"
  OPEN #4;"n";"ROUTER"
  OPEN #4;"n";"0.0"

Purpose:

  Create a network channel associated with a station or router endpoint.

Resolution and availability checks occur during `OPEN #`. The initial protocol is
half-duplex per Spectrum, but the legacy-style syntax does not itself encode
input versus output direction. The exact channel-direction rule, including
whether the first `PRINT #` / `INPUT #` / `INKEY$ #` operation fixes direction
and which mixed-direction sequences are rejected, must be frozen at the Phase-3
entry gate before stream implementation tickets are assigned.

Possible outcomes:

  0 OK
  Invalid station
  Missing station
  Station not set
  Station not found
  Station not available
  Station busy
  Network not present
  Network timeout
  Network version error


### 17.7 PRINT #

Syntax:

  PRINT #stream;expression

Purpose:

  Add formatted character data to an output network stream.

The data may be uploaded to provisional router-side stream storage when the local
buffer fills or at another frozen transport flush boundary, but no partially
written output stream is visible to the destination as a committed message.
`CLOSE #` is the semantic commit boundary.


### 17.8 INPUT #

Syntax:

  INPUT #stream;variable
  INPUT #stream;LINE string-variable

Purpose:

  Receive formatted input from the opened station or router endpoint.

INPUT # is blocking while waiting for the explicitly requested payload.


### 17.9 INKEY$ #

Syntax:

  LET A$=INKEY$ #stream

Purpose:

  Nonblocking retrieval of one available character from an opened network
  stream.

Return:

  "" if no character is currently available
  one character if available

Possible errors after a channel was opened:

- `Station not available`, if the established peer/channel is known to have
  become unavailable;
- `Network data error`, if a previously detected channel failure must be
  surfaced.

`INKEY$ #` itself does not wait and therefore does not create a new network
timeout merely because no character is available; absence of a character returns
`""`.


### 17.10 CLOSE #

Syntax:

  CLOSE #stream

Output-channel behavior:

- flush final buffered data;
- commit the message or stream;
- wait for router acceptance;
- return 0 OK only after successful commit.

Input-channel behavior:

- for a queued message, acknowledge delivery only after the complete message has
  been received and validated;
- preserve/requeue an unconsumed, interrupted, or failed queued delivery; and
- release local channel state after the close/acknowledgment exchange or defined
  failure path.

Generic non-message stream close behavior is frozen with the Phase-6 channel
compatibility specification.


### 17.11 MOVE

Syntax class:

  MOVE source TO destination

Purpose:

  Transfer data between supported Interface 1-style channel specifications or
  streams, with network channels allowed on either side.

**[OPEN]**

The exact accepted MOVE grammar and end-of-file behavior must be copied from the
chosen Interface 1 compatibility baseline and then mapped to the new router.

---

## 18. Success Reports and Error Reports


### 18.1 Success

**[FROZEN]**

Successful immediate operations use normal Spectrum completion presentation:

  0 OK, line:statement

Example:

  0 OK, 0:1

No modern prose success banner is added.


### 18.2 Error presentation

**[FROZEN]**

Network failures use Interface 1-like report presentation with:

- a report identifier;
- report text;
- line number;
- statement number.

The final report letters or numeric codes are not frozen.


### 18.3 Frozen semantic distinctions

**[FROZEN]**

Unknown identity:

  Station not found

Known identity, currently disconnected, disabled, or down:

  Station not available

These conditions must never be collapsed into one generic error.


### 18.4 Baseline error catalog

**[BASELINE]**

Invalid station
  The station operand is malformed or outside permitted ranges.

Missing station
  A required station operand is absent.

Station name in use
  The requested six-character local name is already assigned elsewhere.

Station not set
  The local Spectrum has not completed valid registration.

Station not found
  The router has no registered name or address matching N.

Station not available
  The station exists but is offline, disconnected, disabled, or otherwise down.

Station busy
  The station exists and is online but cannot accept the requested operation.

Network timeout
  An expected exchange did not complete within its permitted interval.

Network not present
  The resident stack cannot communicate with the local router.

Network data error
  Framing, sequence, checksum, or repeated link corruption prevented completion.

Network version error
  The Spectrum stack and router have no compatible protocol version.

Transfer rejected
  The destination or router explicitly refused the transfer.

Verification has failed
  VERIFY completed reception, but received data differs from memory.


### 18.5 Error precedence

**[BASELINE]**

When several conditions are possible, ordinary already-registered network
operations report the earliest authoritative failure in this order:

  1. syntax and missing operand errors;
  2. local station not set;
  3. local router not present;
  4. protocol version mismatch;
  5. station not found;
  6. station not available;
  7. station busy;
  8. transfer rejected;
  9. timeout;
 10. data error;
 11. verification failure.

`FORMAT "n";N` is the registration operation and is explicitly exempt from the
`Station not set` precondition; otherwise a station could never transition from
`NOT_SET` to registered state. Registration uses its command-specific outcomes
from Section 17.1.

The order above is the initial normative precedence for ordinary operations. Any
later change requires an architecture/protocol revision and corresponding
compatibility tests; individual handlers may not choose a different precedence.

---

## 19. Asynchronous Incoming Messages


### 19.1 Required behavior

**[FROZEN]**

The system must handle incoming messages that arrive while the destination
Spectrum is not executing INPUT # or another receive operation.


### 19.2 Queueing model

**[FROZEN]**

A sending station uses normal stream operations:

  OPEN #4;"n";"GAME02"
  PRINT #4;"READY TO START"
  CLOSE #4

The router accepts the completed stream, creates a queued message, and returns
success to the sender when the queue commit is complete.


### 19.3 Attention channel

**[FROZEN]**

The router signals "message waiting" through the destination EAR line using a
slow pattern that can be detected by a small resident handler.

The attention pattern carries no message payload.


### 19.4 Interrupt-time detector

**[BASELINE]**

The resident stack installs a small handler associated with the normal 50 Hz
interrupt path. Attention detection is available only while that resident hook
remains installed, its reserved RAM remains intact, and interrupts continue to
be serviced compatibly. The router queue is authoritative even when notification
is temporarily undetectable.

The handler:

- samples EAR state;
- advances a tiny attention-pattern state machine;
- sets a pending flag after a complete valid pattern;
- preserves required machine state;
- returns quickly;
- never performs full packet reception;
- never calls arbitrary BASIC code;
- never writes message text into the editor.


### 19.5 Example attention pattern

**[PROPOSED]**

An initial slow pattern may be:

  active for 4 frames
  idle for 2 frames
  active for 4 frames

At 50 Hz this lasts approximately 200 milliseconds.

**[OPEN]**

The final pattern must be selected to minimize false detection and avoid
interference with ordinary packet waveforms.


### 19.6 Router attention behavior

**[BASELINE]**

When a station's pending-message count changes from zero to nonzero:

- begin periodic attention signalling;
- stop attention during active packet transmission/reception and during
  router-generated tape/bootstrap waveform delivery;
- resume if pending messages remain after the port returns to network-idle;
- stop after the Spectrum acknowledges the pending condition or the queue
    becomes empty.


### 19.7 Pending flag

**[BASELINE]**

The resident stack exposes at least:

  NET_PENDING = 0
    no attention pattern has been accepted

  NET_PENDING = 1
    router indicates one or more queued messages

The flag is advisory. The router remains authoritative. Any foreground network
entry point may reconcile the flag with `PENDING_STATUS`, and software returning
from a period in which the resident interrupt hook was unavailable must poll the
router before assuming that no traffic is pending.


### 19.8 Message retrieval

**[FROZEN]**

No new BASIC command is added.

Retrieve from any sender through the router endpoint:

  OPEN #4;"n";"ROUTER"
  INPUT #4;S$,M$
  CLOSE #4

Equivalent numeric endpoint:

  OPEN #4;"n";"0.0"

Source-specific retrieval:

```text
OPEN #4;"n";"MASTER"
INPUT #4;M$
CLOSE #4
```

For the router endpoint, one queued message is exposed as two logical input
fields in this order: canonical six-character source name, then complete
message character payload. A source-specific input stream exposes only the
message payload. The exact Spectrum channel-byte encoding must preserve these
observable BASIC semantics and is frozen with the Phase-6 Interface 1
compatibility work.


### 19.9 Delivery acknowledgment

**[BASELINE]**

A queued message is not marked delivered until:

- the complete record is received;
- integrity validation passes; and
- the Spectrum completes the close or explicit acknowledgment path.


### 19.10 Safe notification presentation

**[BASELINE]**

The stack may signal pending traffic through:

- a short sound;
- a brief border indication; or
- another non-destructive visual indication.

It must not alter the current edit line or inject text.

**[OPEN]**

The exact notification presentation is not frozen.

---

## 20. Resident High-Memory Spectrum Stack


### 20.1 Delivery

**[FROZEN]**

The Spectrum stack is distributed as a tape-loadable program.


### 20.2 Residency

**[FROZEN]**

The stack remains in reserved high RAM while BASIC and compatible applications
are active. The installer lowers/protects the usable memory boundary so ordinary
BASIC allocation does not overwrite the resident region.

The architecture does not claim transparent coexistence with arbitrary software
that overwrites the reserved high-memory region, replaces the required interrupt
hook without chaining it, or disables interrupts indefinitely. Such software can
still run as Spectrum software, but resident-network notification/API availability
is not guaranteed until the stack is safely reinstalled or restored.

It is not a DOS-style TSR. It is a resident machine-code extension that runs
through hooks and explicit calls.


### 20.3 Core components

**[BASELINE]**

  installer and uninstaller
  BASIC error-path hook
  Interface 1 syntax parser
  channel manager
  object transfer manager
  packet encoder and decoder
  MIC transmitter
  EAR receiver
  attention detector
  message pending flag
  router registration and resolution client
  checksums and retries
  diagnostics
  buffers and state


### 20.4 BASIC hook model

**[FROZEN]**

The extension uses the normal 48K BASIC error path as the command interception
point.

This interception path is valid only for the Architecture-#3 environment in
which Interface 1 is absent/inactive. The initial release does not chain through
an Interface 1 shadow-ROM parser/error hook and does not arbitrate dual ownership
of the `"n"` syntax. WZSN enforces that precondition through `EAR_MIC` mode;
physical acceptance enforces it by test setup.

Conceptual flow:

  user enters Interface 1-like syntax
          |
          v
  ROM parser reaches unsupported syntax
          |
          v
  ROM error machinery invokes resident hook
          |
          v
  extension recognizes valid network form?
       yes -> parse, execute, and return correctly
       no  -> preserve normal ROM error behavior


### 20.5 Hook safety

**[BASELINE]**

The installer must:

- save every replaced pointer or stack destination;
- install the hook atomically;
- verify its reserved RAM range;
- detect duplicate installation;
- expose a clean remove/reset path that restores every hook/pointer and the
  saved memory-boundary state when restoration is safe;
- pass unrelated BASIC errors to the original handler;
- preserve line and statement reporting.


### 20.6 Tokenization and stored programs

**[OPEN]**

Immediate command interception is straightforward relative to complete stored
program support.

The implementation must determine exactly how to support Interface 1-style
network forms inside numbered BASIC lines while preserving:

- tokenization;
- syntax checking;
- listing;
- execution;
- error position;
- compatibility with ordinary 48K BASIC.

No claim is made here that the error hook alone solves every stored-program
case.


### 20.7 Protected memory and cooperative operation

**[FROZEN]**

While the resident stack is installed, any network receive operation that would
write into the reserved stack/workspace, corrupt the saved hook state, or violate
the active BASIC memory boundary must be rejected before destructive writes
occur. Explicit CODE destinations are range-checked; BASIC/array/MERGE operations
must honor the lowered memory boundary through normal allocation/error behavior.

A rejected unsafe destination maps to the frozen transfer/error policy rather
than sacrificing the resident stack.



**[FROZEN]**

Full network transmit and receive operations are blocking.

A BASIC program or other compatible machine-code program must call the stack at
safe times and preserve the resident region/API contract while using it.

The attention detector is the only background-like function, and it records a
flag only. Networking is not promised to remain resident through arbitrary
software that deliberately takes over the same RAM or interrupt machinery.

---

## 21. Tape Distribution and Installation


### 21.1 Tape structure

**[BASELINE]**

A release tape may contain:

  Block 1: BASIC bootstrap
  Block 2: high-memory core code
  Block 3: optional BASIC front-end code
  Block 4: optional diagnostics/help/demo


### 21.2 Bootstrap concept

**[PROPOSED]**

Example only:

  10 CLEAR 49151
  20 LOAD "" CODE 49152
  30 RANDOMIZE USR 49152

The final CLEAR value, load address, and entry address depend on the actual
binary.


### 21.3 Digital distribution

**[BASELINE]**

Provide:

  TAP
  TZX, if timing features exceed ordinary TAP representation
  WAV, for convenient playback and preservation


### 21.4 Router-generated loading

**[BASELINE]**

The Raspberry Pi router may also act as a tape source.

Startup flow:

1. The router port enters `BOOTSTRAP_TAPE` mode; attention/network downlink
   generation is suppressed.
2. User types `LOAD ""`.
3. Router emits the normal Spectrum tape waveform through EAR.
4. Bootstrap and stack load into RAM.
5. Stack installs hooks.
6. The bootstrap completes and the port returns to `NETWORK_IDLE`.
7. Stack contacts router through MIC/EAR network framing.
8. Router returns station identity and status.

A port may never interleave an attention pattern or routed packet waveform with
an active bootstrap tape waveform.


### 21.5 Network update bootstrap

**[PROPOSED]**

A small stable bootstrap may later load the current full stack from the router.
This is a future enhancement, not an initial-release acceptance requirement.

Cold start still requires:

- real tape;
- digital tape playback; or
- router-generated tape waveform.


### 21.6 Image header

**[PROPOSED]**

The binary image should contain:

  signature
  image format version
  network protocol version
  software version
  load address
  entry address
  required RAM
  feature flags
  binary length
  binary checksum

---

## 22. Internal Spectrum Stack API


### 22.1 Purpose

**[FROZEN]**

The BASIC front end and machine-code applications use one common networking
core.


### 22.2 Baseline entry points

**[BASELINE]**

  NET_INIT
  NET_REMOVE
  NET_REGISTER
  NET_GET_IDENTITY
  NET_RESOLVE
  NET_OPEN_STREAM
  NET_STREAM_WRITE
  NET_STREAM_READ
  NET_STREAM_POLL
  NET_CLOSE_STREAM
  NET_SEND_OBJECT
  NET_RECEIVE_OBJECT
  NET_VERIFY_OBJECT
  NET_POLL_ROUTER
  NET_GET_PENDING
  NET_GET_STATUS
  NET_ABORT

The Phase-2 entry gate freezes the exported names that are required by physical
diagnostics and the calling convention for those entries. Later additions may
be appended compatibly; existing entry-point semantics may not drift between the
BASIC front end and machine-code callers.


### 22.3 Calling convention

**[OPEN]**

The Z80 register convention, error return convention, buffer ownership, and
reentrancy rules must be defined before assembly implementation.


### 22.4 Reentrancy

**[BASELINE]**

The stack is not reentrant.

The attention interrupt handler may only touch dedicated interrupt-safe state.

Full link functions must disable or coordinate attention processing while they
own the EAR path.

---

## 23. Router Data Model


### 23.1 Stations table

**[BASELINE]**

Fields:

  station_id_internal
  name
  area
  node
  physical_port
  enabled
  online
  protocol_version
  stack_version
  last_seen
  created_at
  updated_at

Constraints:

  unique(name)
  unique(area,node)
  unique active physical_port binding
  station_id_internal is immutable and is never reused for a different station

If an administrator deletes/replaces a station and later reuses its visible name
or address, the replacement receives a different internal station ID.


### 23.2 Messages table

**[BASELINE]**

Fields:

  message_id
  commit_sequence
  source_station_id_internal
  source_area_snapshot
  source_node_snapshot
  source_name_snapshot
  destination_station_id_internal
  destination_area_snapshot
  destination_node_snapshot
  destination_name_snapshot
  payload_length
  payload_location or payload_blob
  payload_checksum
  created_at
  delivery_state
  delivery_attempts
  last_attempt_at


### 23.3 Object transfers table

**[BASELINE]**

Fields:

  transfer_id
  source internal station ID
  source address/name snapshot
  destination internal station ID
  destination address/name snapshot
  object type
  object metadata
  total length
  received length
  object checksum
  staging location
  state
  created_at
  updated_at


### 23.4 Port status table or in-memory state

**[BASELINE]**

Fields:

  port number
  configured station
  cable/activity state
  uplink state
  downlink state
  last edge time
  last valid frame time
  framing errors
  CRC errors
  retries
  queue depth
  controller health

---

## 24. State Machines


### 24.1 Station lifecycle

  UNCONFIGURED
      | administrator assigns port/name/address
      v
  CONFIGURED
      |
      | physical activity and valid registration
      v
  ONLINE
      |
      | disconnect, timeout, disable
      v
  UNAVAILABLE
      |
      | reconnect and register
      v
  ONLINE


### 24.2 Spectrum local registration

  NOT_SET
      |
      | FORMAT "n";N
      v
  REGISTERING
      |
      +--> SUCCESS -> SET
      |
      +--> NAME_CONFLICT
      |
      +--> NETWORK_NOT_PRESENT
      |
      +--> VERSION_ERROR
      |
      +--> TIMEOUT


### 24.3 Output stream

  CLOSED
      |
      | OPEN #
      v
  OPEN_OUTPUT
      |
      | PRINT #
      v
  BUFFERING
      |
      | CLOSE #
      v
  COMMITTING
      |
      +--> ROUTER_ACCEPTED -> CLOSED, 0 OK
      |
      +--> REJECTED -> CLOSED, error
      |
      +--> TIMEOUT -> COMMIT_UNKNOWN
                         | bounded idempotent retry of same commit transaction
                         +--> ACK -> CLOSED, 0 OK
                         +--> RETRIES_EXHAUSTED -> CLOSED, Network timeout


### 24.4 Queued message

  RECEIVING_FROM_SENDER
      |
      | complete, checksum valid
      v
  QUEUED
      |
      | destination notified
      v
  PENDING_DELIVERY
      |
      | destination opens input
      v
  DELIVERING
      |
      +--> ACKNOWLEDGED -> DELIVERED
      |
      +--> INTERRUPTED -> QUEUED


### 24.5 Object upload

  OFFER
      |
      +--> REJECTED
      |
      v
  ACCEPTED
      |
      v
  RECEIVING_CHUNKS
      |
      +--> ABORTED
      |
      +--> RETRY
      |
      v
  VERIFYING_OBJECT
      |
      +--> CHECKSUM_ERROR
      |
      v
  STAGED_COMPLETE


### 24.6 Attention detector

  IDLE
      |
      | expected active duration
      v
  ACTIVE_1
      |
      | expected idle duration
      v
  IDLE_GAP
      |
      | expected second active duration
      v
  ACTIVE_2
      |
      | complete
      v
  PENDING_FLAG_SET

Any invalid transition returns to IDLE.


### 24.7 Router port mode

```text
NETWORK_IDLE
    | router-generated cold/bootstrap load requested
    v
BOOTSTRAP_TAPE
    | tape waveform complete/aborted
    v
NETWORK_IDLE
    | full routed transfer owns EAR/MIC direction
    v
NETWORK_TRANSFER
    | complete/abort/timeout
    v
NETWORK_IDLE
```

Attention generation is permitted only in `NETWORK_IDLE`. Transport loss or
controller fault forces the hardware output to safe idle and the logical port to
a recoverable unavailable/fault state until resynchronized.

In WZSN `EAR_MIC` mode, this state machine is also the cassette-socket ownership
arbiter. The normal WZSN tape engine is not a second simultaneous EAR source or
MIC consumer. `BOOTSTRAP_TAPE` is the only in-mode tape-waveform ownership path
for installing the distributed stack while retaining Ear+Mic attachment.

---

## 25. Reliability and Recovery


### 25.1 Required mechanisms

**[BASELINE]**

  packet CRC
  object checksum
  sequence numbers
  duplicate suppression
  bounded retries
  deterministic timeouts
  explicit ACK/NAK
  resumable router-side message delivery
  safe abort
  queue persistence where promised


### 25.2 Stop-and-wait baseline

**[BASELINE]**

The first file-transfer implementation uses one outstanding chunk at a time.

Advantages:

- low RAM use;
- simple duplicate handling;
- simple recovery;
- easy oscilloscope correlation;
- easy simulator trace comparison.


### 25.3 Duplicate handling

**[BASELINE]**

If an acknowledgment is lost, a sender may repeat a packet or the same commit
transaction.

The receiver/router shall:

- recognize a duplicate sequence/transaction identifier within the frozen retry
  window;
- avoid applying payload or creating a committed queue record twice; and
- resend the previous acknowledgment/result when it is still authoritative.

A successful commit is therefore idempotent under protocol retries. If all
bounded retries are exhausted after a response is lost, the sender reports
`Network timeout`; remote completion may be uncertain. The protocol does not
claim exactly-once application semantics across an unrecoverable link failure.


### 25.4 Power or link loss

**[BASELINE]**

During message upload:

  Incomplete message is discarded or retained as incomplete, never delivered.

During message delivery:

  Message returns to queued state unless acknowledgment completed. Delivery is
  consequently at-least-once across an unrecoverable acknowledgment failure; a
  recipient/application must not assume exactly-once delivery after a timeout.

During object upload:

  Staged object remains incomplete and is not exposed.

During object download:

  The Spectrum reports failure. Resume support is optional for the first
  release.


### 25.5 Timeout policy

**[OPEN]**

Timeout values must be defined per operation:

  registration
  name resolution
  stream open
  stream close/commit
  object offer
  object chunk
  object completion
  message retrieval
  attention acknowledgment

---

## 26. Security and Trust Boundaries


### 26.1 Source authentication

**[FROZEN]**

Source identity is derived from the physical router port binding, not trusted
from Spectrum packet content. This is physical-port identification, not
cryptographic authentication: a person with physical access who can replace the
device/cable on a configured port inherits that port's identity unless an
administrator changes or disables the binding.


### 26.2 Input validation

**[BASELINE]**

Validate:

  protocol version
  packet type
  length
  destination
  station-name syntax
  address range
  sequence number
  CRC
  object metadata
  memory ranges requested by Spectrum operations


### 26.3 Denial-of-service controls

**[PROPOSED]**

Per station:

  bounded message queue
  bounded object staging
  bounded packet rate
  bounded retries
  bounded open channels
  administrative disable


### 26.4 No confidentiality claim

**[FROZEN]**

The Spectrum MIC/EAR network protocol provides no encryption and no protection
against a person with physical access to the router or cables. This statement
does not forbid authentication, TLS, or another protected transport on a separate
network-reachable **administrative** interface; such a management surface must
meet Section 10.3 regardless of the Spectrum-link confidentiality policy.

---

## 27. Diagnostics and Observability


### 27.1 Spectrum diagnostic program

**[BASELINE]**

The release tape distribution must include a diagnostic utility capable of:

- emitting known MIC pulse patterns;
- detecting known EAR patterns;
- displaying router presence;
- displaying assigned name and address;
- measuring successful frames;
- reporting CRC and timeout errors;
- testing attention notification;
- performing loopback through the router;
- sending a small test stream;
- transferring a small CODE block.


### 27.2 Router logs

**[BASELINE]**

Operational logs record at minimum:

  timestamp
  physical port
  authenticated station
  operation
  source
  destination
  result
  error code
  retry count
  byte count
  controller fault

Message contents should not be logged by default.


### 27.3 Timing traces

**[BASELINE]**

The timing controller/router diagnostic path supports a trace containing:

  port
  edge timestamp
  measured pulse width
  decoded bit
  frame boundary
  CRC result
  transmit schedule
  underrun or overrun


### 27.4 Administrative status

**[PROPOSED]**

A status view should show:

  port
  name
  address
  online state
  last seen
  queued messages
  active transfer
  errors
  protocol version

---

## 28. Testing Strategy


### 28.1 Test levels

**[BASELINE]**

  unit tests
  protocol codec tests
  router service tests
  timing-controller tests
  analog bench tests
  real Spectrum integration tests
  simulator integration tests
  endurance tests
  fault-injection tests


### 28.2 Spectrum-side unit tests

Test:

  station parser
  six-character validation
  x.y validation
  name normalization
  packet encoding
  packet decoding
  CRC
  sequence handling
  error mapping
  object metadata
  attention state machine
  hook installation and removal


### 28.3 Router tests

Test:

  unique name enforcement
  unique address enforcement
  port source authentication
  unknown versus unavailable distinction
  busy handling
  message commit semantics
  message redelivery after failed acknowledgment
  object staging integrity
  queue bounds
  protocol-version mismatch
  restart persistence
  process termination immediately after successful queue commit
  atomic metadata/payload visibility after recovery


### 28.4 Physical-link tests

Use at least:

  multiple Issue 2 boards
  multiple power supplies
  multiple cable lengths
  repeated plug insertion
  amplitude variation
  noise injection
  intentional pulse-width distortion
  lost edges
  duplicate frames
  corrupted CRC
  delayed acknowledgments
  controller/Pi transport loss during receive and transmit
  controller reset/watchdog safe-idle behavior


### 28.5 Asynchronous-message tests

Test:

  message arrives while BASIC prompt is idle
  message arrives while a line is being edited
  message arrives while BASIC program runs
  message is queued while arbitrary game code runs
  attention is detected while compatible game/demo code runs with resident RAM and
  interrupt hook preserved
  attention pattern during no traffic
  attention suppressed during packet transfer
  false-pattern rejection
  repeated attention until acknowledgment
  message retained after failed retrieval
  message removed only after successful close/acknowledgment
  message remains queued when resident interrupt notification is unavailable
  foreground poll reconciles pending state after interrupt-hook restoration


### 28.6 Front-end compatibility tests

Test every supported syntax in:

  immediate mode
  numbered BASIC program where supported
  success path
  each defined error path
  LIST behavior
  syntax error location
  line and statement report
  unrelated normal BASIC errors


### 28.7 Transfer tests

For each object type:

  zero or minimum legal size
  typical size
  maximum supported size
  exact checksum match
  one corrupted chunk
  lost acknowledgment
  duplicate chunk
  receiver cancellation
  router restart during staging


### 28.8 Long-duration tests

Run:

  repeated short messages
  repeated screen transfers
  multi-port concurrent transfers
  attention signalling over many hours
  queue fill and drain cycles
  router/controller reconnect cycles
  repeated daemon restarts after acknowledged message commits
  deterministic WZSN multi-machine virtual runs with master-tick trace comparison
  WZSN NONE / INTERFACE1 / EAR_MIC mutual-exclusion transitions
  EAR_MIC fidelity run after loading the distributed RAM stack
  proof that EAR_MIC mode cannot expose Interface 1/Microdrive/original ZX Net

---

## 29. Simulator and Emulator Integration


### 29.1 Purpose

**[FROZEN]**

The same architecture must support WZSN virtual Spectrums.


### 29.2 Emulated physical boundary

For WZSN, the emulator integration consumes the canonical timing contract of
`warajevo-zx-spectrum-next-architecture.md`. The boundary exposes:

- MIC output-level transitions with canonical emulated **master-tick**
  timestamps; and
- EAR input level queried/scheduled at canonical emulated master ticks.

Conceptual interfaces are therefore:

```text
OnMicLevelChanged(level, master_tick)
GetEarLevel(master_tick)
```

T-state values used by the physical modulation profile remain Spectrum-side
engineering units and are converted through the selected WZSN machine profile's
frozen master-tick/T-state ratio. This document does not create a second
emulated clock.

### 29.3 Virtual router port

**[BASELINE]**

A virtual port replaces analog conditioning and physical timing hardware but
preserves the same MIC/EAR pulse stream, routed-packet framing, and router
service behavior. The virtual proof uses the same router protocol/core logic,
not a second emulator-only network protocol.

The virtual port validates the digital Spectrum/ULA boundary. It does not claim
to model cable, comparator, amplitude, grounding, or other Issue-2 analog
behavior; real hardware and bench measurement remain authoritative for those
physical properties.


### 29.4 Determinism

**[BASELINE]**

Phase-1 virtual proof uses a deterministic headless test harness that can host
at least two independent WZSN machine contexts plus virtual router ports. This
is a test/integration requirement; it does not require the normal WZSN GUI to
become a multi-machine application.

The harness uses a shared virtual-time coordinator based on WZSN canonical
master ticks. No machine may run arbitrarily ahead of another when an
inter-machine pulse or router event can affect observable behavior. Event-order
rules must be deterministic and recorded in the virtual-router companion test
specification before Phase-1 exit.

Router-core timeout/scheduling code must consume an injected time source.
Physical-router operation uses its monotonic physical/controller time domain;
virtual proof uses the shared emulated-time coordinator. Host wall-clock
scheduling may not leak into deterministic virtual protocol outcomes.


### 29.5 No byte shortcut in fidelity mode

**[FROZEN]**

The primary simulator mode shall not replace port FE signalling with a direct
SendByte API.

An optional accelerated test mode may exist, but it must be clearly separate
from the physical-layer simulation and may not satisfy fidelity acceptance
criteria.


### 29.6 WZSN runtime-speed independence

**[FROZEN]**

WZSN runtime emulation-speed selection changes only host wall-clock pacing. The
virtual MIC/EAR router protocol remains expressed in canonical emulated time. A
400% or Unlimited WZSN run therefore executes the same Spectrum-time pulse
widths, turnaround intervals, retries, and event order as a 100% run; it merely
reaches those master ticks sooner in host time.

### 29.7 WZSN networking-mode integration

**[FROZEN]**

Architecture #3 consumes the WZSN core networking-mode contract:

```text
NONE
INTERFACE1
EAR_MIC
```

The virtual routed MIC/EAR network is attachable only in `EAR_MIC` mode and only
on the explicit WZSN **ZX Spectrum 48K Issue-2** profile/variant certified for
this architecture. In that mode WZSN guarantees that Interface 1, Microdrive,
and original ZX Net are not active. `NONE` and `INTERFACE1` disconnect the
Architecture-#3 virtual router from the machine. A 128K or uncertified 48K
profile cannot enter Architecture-#3 `EAR_MIC` mode.

`EAR_MIC` also gives Architecture #3 exclusive semantic ownership of the
emulated cassette EAR/MIC peer. The ordinary WZSN tape transport may not drive
EAR or consume/record MIC concurrently. Stack installation without leaving
`EAR_MIC` uses this architecture's `BOOTSTRAP_TAPE` router-port mode, which
emits the standard tape waveform while network attention and packet signalling
are suppressed.

Selecting `EAR_MIC` changes only the emulated hardware/environment connection.
It does not inject the Spectrum-resident stack, patch RAM, call ROM routines, or
page a special ROM. The same distributed stack intended for physical machines
must be loaded/installed through the tape/bootstrap path for fidelity tests.

Changing networking mode uses the **cold machine-reconfiguration** transition
owned by Architectures #1 and #2. It creates a fresh machine context rather
than applying an ordinary Spectrum Reset, so a previously installed Ear+Mic RAM
hook cannot survive into `INTERFACE1` mode. If WZSN is application-paused, the
new machine context remains paused after the transition.

The Phase-1 virtual proof and all later WZSN fidelity tests must explicitly use
the certified 48K Issue-2 profile/variant in `EAR_MIC` mode and must verify that
attempting to enable Interface 1 simultaneously is impossible through the
command registry or core state model.

Multiple ordinary WZSN application processes having distinct Telnet Control
Ports does **not** by itself make those emulator instances Ear+Mic network peers.
Architecture #3's deterministic virtual proof continues to require the shared
virtual-time/router coordination contract of Section 29.4. Any future direct
cross-process Ear+Mic transport between independently paced GUI processes
requires an explicit synchronization/transport architecture revision; host
wall-clock socket arrival must not become emulated network time.

---

## 30. Implementation Phases

Architecture #3 implementation begins only after the applicable completion and
acceptance gates of the WZSN core/system and UI architectures have passed. In
particular, the 48K PAL machine, port-FE EAR/MIC behavior, canonical
master-tick scheduler, deterministic test harness foundations, and application
orchestration boundaries must already be dependable.

### Phase 0 - Specification freeze

Freeze before implementation tickets that depend on them:

- exact upstream WZSN ZX Spectrum 48K Issue-2 profile/variant identity, ROM hash,
  and hardware assumptions used by the Architecture-#3 virtual proof;
- ordinary/reserved address ranges beyond frozen router endpoint `0.0`;
- registration policy beyond the preconfigured-binding prototype baseline;
- packet header and packet-type numeric assignments;
- channel direction/mixed-I/O semantics for the Phase-3 stream subset;
- physical/link frame preamble and sync representation;
- within-version protocol negotiation rules;
- CRC polynomial, initialization, reflection, and byte order;
- virtual-proof payload-size candidate;
- retry counts and timeout policy for Phase-1 operations;
- stream commit semantics, immutable-station binding, queue-order, and retention
  rules;
- stream/transaction identifier representation and packet-sequence wrap/duplicate
  window semantics;
- broadcast exclusion or complete broadcast semantics;
- object metadata format;
- maximum initial message size;
- Pi/controller framing requirements sufficient for simulation stubs;
- router daemon/module boundary and repository/source-tree placement;
- implementation languages/build systems for router daemon and virtual proof;
- initial error-code namespace (semantic report text remains as Section 18);
- versioning policy for Spectrum stack, router daemon, and timing controller.

**Exit gate:** the packet codec, router registry, and virtual Spectrum stack can
be ticketed without an implementer choosing a wire-format behavior locally.

### Phase 1 - Virtual proof

Implement:

- packet codec;
- router registry;
- virtual ports;
- deterministic shared-master-tick coordinator;
- two simulated WZSN 48K Spectrum machine contexts in `EAR_MIC` mode;
- loading/installation of the same distributed resident stack used by physical
  Spectrum acceptance, with no emulator-only networking ROM;
- direct message stream;
- persistent queue commit/restart test double or real store;
- success/error mapping.

**Exit gate:** two simulated Spectrums in `EAR_MIC` mode exchange a pulse-level
message through the virtual router deterministically in repeated runs, with no
direct byte shortcut, no Ear+Mic ROM paging, and no simultaneous Interface 1
state in fidelity mode.

### Phase 2 - Spectrum physical diagnostic

Before dependent hardware/assembly tickets, freeze:

- first diagnostic hardware port count (one is sufficient for this phase);
- Spectrum-stack assembler/build toolchain and machine-code calling convention;
- provisional high-memory load range used by diagnostics;
- minimum immediate-mode BASIC error-hook mechanism needed for Phase-3 commands;
- conservative pulse profile for bench testing;
- turnaround guard test procedure;
- timing-controller technology selected for the prototype;
- Pi/controller transport and digital framing selected for the prototype;
- analog prototype schematic and protection limits sufficient for supervised
  bench work.

Implement:

- MIC pattern transmitter;
- EAR pattern detector;
- one-port analog prototype;
- timing controller;
- loopback tests;
- trace correlation between WZSN virtual pulses and physical-controller traces.

**Exit gate:** one unmodified Issue 2 machine can transmit and receive the
known diagnostic patterns repeatedly without unsafe electrical behavior or
unexplained timing divergence.

### Phase 3 - Two-station physical network

Before implementation, freeze:

- exact turnaround guard;
- conservative physical pulse timings used by the initial protocol;
- final initial-release routed-packet payload size after physical evidence;
- integrated high-memory reservation/load range used by Phase-3 through Phase-5;
- immediate-mode BASIC error-hook installation/recovery contract;
- station online/unavailable liveness and disconnect-detection policy;
- final attention waveform and false-positive threshold for this release;
- Phase-3 retry/timeout values;
- maximum queued message size and end-to-end message integrity algorithm;
- final channel direction/mixed-I/O semantics used by message send/retrieval;
- router crash-recovery behavior for committed messages;
- administrative station-rebinding behavior for already queued data.

Implement:

- two independent ports;
- registration;
- `OPEN #` / `PRINT #` / `CLOSE #` message send;
- `INPUT #` retrieval;
- attention flag;
- FIFO message selection;
- unknown/unavailable errors;
- durable queue commit and process-restart recovery.

**Exit gate:** two real Issue 2 machines exchange queued messages bidirectionally
with correct attention, failure, retry, and durability behavior.

### Phase 4 - Object transfer

Before implementation, freeze:

- object-transfer staging strategy for the initial release;
- maximum object size;
- object checksum algorithm, staging/retrieval order, and metadata representation;
- Phase-4 retry/timeout values.

Implement:

- CODE;
- SCREEN$;
- BASIC program;
- numeric and character arrays;
- VERIFY;
- MERGE.

**Exit gate:** every required object class passes minimum/typical/maximum,
corruption, retry, abort, and end-to-end checksum tests on simulator and real
hardware.

### Phase 5 - Multi-port router

Before implementation, freeze:

- first multi-port reference hardware count (minimum four ports);
- production-intent port electronics topology for that reference board;
- Raspberry Pi deployment/OS/service baseline;
- administrative-interface mechanism and access-control policy;
- queue/staging/open-channel/packet-rate limits per station;
- router/controller watchdog and reconnect policy.

Implement:

- four or more ports;
- concurrent independent links;
- persistent registry;
- bounded queues/staging;
- local administration;
- diagnostics and status surfaces.

**Exit gate:** multi-port concurrency and isolation tests pass without
cross-port corruption, source-identity confusion, or unbounded resource use.

### Phase 6 - Full Interface 1-style integration

Before implementation, freeze:

- exact chosen Interface 1 compatibility baseline for `MOVE`;
- final ROM error-hook behavior for full immediate/stored-program compatibility,
  building on the Phase-3 immediate-mode contract;
- complete stored-BASIC tokenizer/list/execution strategy;
- final binary image-header format and tape packaging metadata;
- final Spectrum stack calling convention if not already frozen;
- release-final high-memory allocation/generated map, or explicit confirmation
  that the Phase-3 integrated map remains final;
- final report letters/numeric identifiers;
- user-visible pending-message notification presentation.

Implement and validate:

- immediate forms;
- stored-program forms;
- channel behavior;
- `MOVE`;
- line/statement reports;
- complete tape installer;
- router-endpoint two-field message retrieval semantics.

**Exit gate:** every supported Interface 1-style form passes immediate and
stored-program compatibility tests without changing unrelated 48K BASIC error
behavior.

### Phase 7 - Hardening

Before production PCB/release tickets, freeze:

- final isolation strategy and isolation ratings;
- connector/cable specification;
- production PCB port count/layout;
- final electrical component values and protection network;
- final conservative profile and any optional negotiated faster profile;
- controller firmware build/update toolchain and security/recovery policy;
- timing-controller buffer sizing/flow-control margins at the release profile;
- sudden-power-loss storage expectations and recovery tests.

Implement:

- production isolation;
- production PCB;
- watchdogs;
- controller firmware update;
- endurance/fault-injection tests;
- complete documentation and manufacturing/test records.

**Exit gate:** all Section-31 acceptance criteria pass on the release hardware,
and no Section-32 item remains open beyond a deliberately deferred future
feature.


## 31. Acceptance Criteria

Architecture #3 is implementation-complete only when all applicable criteria
below pass with no unexplained divergence.

1. An unmodified Issue 2 48K Spectrum can load the resident stack from a
   standard tape image or router-generated tape waveform.
2. After installation, unrelated normal 48K BASIC errors still follow their
   normal behavior and report path.
3. `FORMAT "n";N` establishes/confirms a valid local identity or returns a
   defined deterministic error.
4. Exactly six-character alphanumeric station names are enforced uniquely;
   canonical storage is uppercase.
5. Reserved service name `ROUTER` cannot be assigned to an ordinary station.
6. Reserved address `0.0` cannot be assigned to an ordinary station and selects
   the same local router endpoint as `ROUTER` where defined.
7. Both ordinary `"x.y"` and `"AAAAAA"` station designators work.
8. Unknown and known-but-unavailable stations produce different reports.
9. Source identity is derived from the authenticated physical/virtual port, not
   trusted from Spectrum payload content.
10. `OPEN #`, `PRINT #`, and `CLOSE #` can send a queued character message
    through the router.
11. `CLOSE #` returns `0 OK` only after the complete message and required
    metadata are persistently committed under Section 15.4.
12. A successful queued message survives a normal router-daemon restart before
    recipient retrieval.
13. The initial release rejects queue commits to stations that are offline at
    the authoritative commit check; no grace-period mailbox behavior occurs.
14. Router-endpoint retrieval is stable FIFO by commit sequence, while
    source-specific retrieval is FIFO within that source subset.
15. A message can arrive while the destination Spectrum is not receiving.
16. The destination detects an attention indication without receiving payload
    in the interrupt handler.
17. The attention handler does not inject text, call arbitrary BASIC, or perform
    full packet reception.
18. A queued message remains queued after failed or incomplete retrieval.
19. A message is marked delivered only after complete validated retrieval and
    the required close/acknowledgment succeeds.
20. `SAVE *` and `LOAD *` transfer every required Spectrum object class with
    end-to-end integrity.
21. `VERIFY *` reports `Verification has failed` for mismatched data.
22. `MERGE *` preserves normal Spectrum BASIC merge semantics for the supported
    compatibility baseline.
23. No Spectrum MIC outputs are electrically tied together; each station has an
    independent router RX/TX path.
24. No Raspberry Pi Linux user-space scheduler is relied upon to generate or
    measure active cassette pulse timing.
25. Every physical router port meets the release protection/isolation contract.
26. A WZSN virtual harness exercises the same pulse-level protocol using
    canonical master-tick timestamps.
27. The WZSN fidelity path contains no direct byte shortcut that bypasses port-FE
    MIC/EAR behavior.
28. Repeated identical virtual tests produce identical event order and results.
29. Virtual and physical timing traces for the diagnostic corpus have no
    unexplained protocol-significant divergence.
30. The visible Spectrum command vocabulary contains no new `NET`, `MSG`, `WHO`,
    `FILES`, `GET`, `SEND`, or `SHARE` command family.
31. Immediate-mode and supported stored-BASIC forms preserve listing, syntax,
    execution, and line/statement reporting requirements.
32. The resident stack is non-reentrant; interrupt attention state cannot race
    with full EAR receive ownership.
33. Duplicate packets do not apply payload twice and produce the required repeat
    acknowledgment behavior.
34. Packet corruption, lost acknowledgments, timeouts, cancellation, and router
    restart follow the Section-25 recovery contract.
35. Per-station queues, open channels, retries, and staging are bounded by frozen
    release limits.
36. Initial administration is local-only; any network-reachable management
    surface is authenticated and authorized.
37. Router logs omit message contents by default while retaining required
    operational diagnostics.
38. At least four ports operate concurrently in the multi-port reference build
    without cross-port identity, waveform, queue, or fault contamination.
39. Tape-distributed diagnostics can exercise MIC output, EAR input, attention,
    loopback, a small stream, and a small CODE transfer.
40. `FORMAT` can execute from local `NOT_SET` state; ordinary `Station not set`
    precedence never blocks registration.
41. `INKEY$ #` returns immediately and does not generate a timeout solely because
    no character is available.
42. Queued-message transport retries are idempotent and do not create duplicate
    committed records from a repeated commit transaction.
43. The protocol documentation makes no exactly-once guarantee after an
    unrecoverable acknowledgment timeout; retained/redelivered messages follow the
    documented at-least-once recovery rule.
44. Router-generated bootstrap tape, attention signalling, and routed packet
    delivery are mutually exclusive port modes and never interleave waveforms.
45. Loss/reset of the Pi/controller link forces the physical downlink driver to
    the frozen safe-idle state.
46. An installed resident stack rejects network receives that would overwrite its
    reserved RAM/hook state.
47. Network notification/API availability is not claimed for arbitrary software
    that destroys the resident RAM or interrupt contract; queued router data
    remains authoritative and can be reconciled after restoration.
48. One physical/link frame carries one routed packet in the initial protocol and
    uses the single frozen packet CRC definition; no undocumented second link CRC
    exists.
49. Queued messages and staged objects bind to immutable internal station
    identities; later display-name/address changes cannot silently retarget them.
50. A destination that disconnects after a successful commit retains that already
    committed message for its later return, while new commits to an offline
    destination remain rejected.
51. Initial committed messages do not silently expire by wall-clock age; bounded
    queue limits govern acceptance of new data.
52. Stream/object control has a frozen logical transaction identifier and packet
    sequence-wrap/duplicate-window rule sufficient for idempotent retries.
53. The virtual router uses the same router protocol/core behavior as the physical
    system and an injected deterministic time source.
54. WZSN emulation-speed changes do not change virtual Spectrum-time protocol
    timing or event ordering.
55. Router liveness rules distinguish an idle online station, a busy station, and
    an unavailable/disconnected station deterministically under the frozen
    Phase-3 policy.
56. `station_id_internal` is immutable/non-reused, so deleting and recreating a
    visible name/address cannot inherit old queued data accidentally.
57. Phase-3 integrated BASIC commands use a frozen immediate-mode hook and memory
    reservation; Phase 6 does not retroactively invent those foundations.
58. The selected Spectrum/router/controller build and deployment toolchains are
    recorded before their dependent implementation/release tickets.
59. All Phase-0 through Phase-7 exit gates applicable to the release are closed
    before that release is declared architecture-complete.
60. Physical Architecture-#3 acceptance is performed with Interface 1 absent or
    inactive while the Ear+Mic resident BASIC front end is installed.
61. WZSN Architecture-#3 fidelity tests run only in `EAR_MIC` networking mode,
    which structurally excludes Interface 1, Microdrive, and original ZX Net.
62. `EAR_MIC` mode does not page a networking ROM and does not auto-install or
    patch the resident Spectrum stack; fidelity tests load the distributed stack
    through the documented tape/bootstrap path.
63. WZSN networking-mode changes use the upstream cold machine-reconfiguration
    transition, so old resident RAM hooks/device state cannot survive the mode
    change and simultaneous `INTERFACE1`/`EAR_MIC` state cannot occur.
64. In WZSN `EAR_MIC` mode the routed virtual port exclusively owns cassette
    EAR/MIC semantics; ordinary tape transport cannot drive/consume the same
    signals concurrently, and stack installation uses `BOOTSTRAP_TAPE`.
65. WZSN Ear+Mic fidelity uses an explicitly certified ZX Spectrum 48K Issue-2
    profile/variant; 128K and uncertified 48K variants are rejected without
    machine mutation, and distinct Telnet Control Ports do not imply an
    unsynchronized cross-process virtual network.


## 32. Deferred Engineering Decisions and Gates

An item in this section is not an architecture gap if and only if its gate is
closed before the first dependent implementation ticket. A ticket may research
an item before its gate; it may not silently choose the production behavior.

| ID | Decision | Required gate |
|---|---|---|
| D01 | Final analog input/output schematics for supervised prototype | Phase 2 entry |
| D02 | Final production isolation strategy and ratings | Phase 7 entry |
| D03 | Final connector and cable specification | Phase 7 entry |
| D04 | Production/reference PCB port count beyond Phase-5 minimum four | Phase 5/7 entry as applicable |
| D05 | Conservative pulse timings and any later turbo profile | prototype profile: Phase 2 entry; initial release profile: Phase 3 entry; turbo: Phase 7 or later |
| D06 | Final physical/link frame preamble and sync | Phase 0 exit |
| D07 | CRC polynomial, initialization, reflection, and representation | Phase 0 exit |
| D08 | Packet payload size | virtual-proof candidate: Phase 0 exit; initial physical release value: Phase 3 entry |
| D09 | Retry counts and operation-specific timeout values | before each dependent phase; initial set at Phase 0 exit |
| D10 | Remaining reserved/broadcast address ranges beyond frozen `0.0` | Phase 0 exit |
| D11 | Production registration/reassignment policy beyond preconfigured port binding | Phase 0 exit; pending-data reassignment safety also requires D47 |
| D12 | Lowercase station-name handling | **closed:** normalize to uppercase before validation |
| D13 | Exact Interface 1 compatibility baseline for `MOVE` | Phase 6 entry |
| D14 | BASIC ROM error-hook implementation/recovery | minimum immediate-mode contract: Phase 2/3 entry; full stored-program compatibility: Phase 6 entry |
| D15 | Complete stored-BASIC tokenizer/list/execution strategy | Phase 6 entry |
| D16 | High-memory load address and RAM footprint | diagnostic provisional range: Phase 2 entry; integrated Phase-3 map: Phase 3 entry; release-final confirmation/map: Phase 6 entry |
| D17 | Exact attention waveform and false-positive threshold | Phase 3 entry |
| D18 | Pending-message presentation: border, sound, or combination | Phase 6 entry |
| D19 | Offline queue grace-period semantics | **closed for initial release:** destination must be online at commit check; no grace period |
| D20 | Broadcast in initial release | **closed:** excluded unless Phase 0 adds complete explicit semantics |
| D21 | Object-transfer staging strategy | Phase 4 entry |
| D22 | Maximum message and object sizes | message: Phase 3 entry; object: Phase 4 entry |
| D23 | Pi/controller physical transport selection | Phase 2 entry |
| D24 | Router restart/database recovery and sudden-power-loss policy | daemon restart: Phase 3 entry; sudden power loss: Phase 7 entry |
| D25 | Final report letters/numeric identifiers | Phase 6 entry |
| D26 | Uplink source-field policy | **closed:** source omitted on initial uplink; router derives it from port binding |
| D27 | Bit order within a byte | **closed:** most-significant bit first |
| D28 | Initial administrative mechanism and network reachability | Phase 5 entry; network-reachable administration requires authentication |
| D29 | Spectrum-stack Z80 calling convention and buffer ownership | Phase 2 entry |
| D30 | Virtual multi-machine event-order/coordinator contract | Phase 1 entry |
| D31 | Repository/source-tree placement for router daemon, timing firmware, Spectrum stack, and hardware design files | Phase 0 exit |
| D32 | Controller firmware-update security/recovery mechanism | Phase 7 entry |
| D33 | Exact request/response turnaround guard | Phase 3 entry |
| D34 | Baseline exported Spectrum-stack API names/semantics | Phase 2 entry |
| D35 | Router daemon/module decomposition beyond frozen logical responsibilities | Phase 0 exit |
| D36 | Routed-packet header field layout and packet-type numeric assignments | Phase 0 exit |
| D37 | Binary image-header/tape packaging format | Phase 6 entry |
| D38 | Concrete persistent schema/storage mapping for baseline router data model | Phase 1 for registry; Phase 3/4 for messages/objects |
| D39 | Per-station queue, staging, open-channel, packet-rate, and retry bounds | Phase 5 entry; smaller test bounds frozen earlier as needed |
| D40 | Stream channel direction and mixed `PRINT #`/`INPUT #`/`INKEY$ #` semantics | Phase 3 entry for message subset; Phase 6 final compatibility |
| D41 | Deterministic timing-controller technology for prototype | Phase 2 entry |
| D42 | Pi/controller digital framing, integrity, flow control, and reconnect protocol | Phase 2 entry |
| D43 | End-to-end message and object integrity algorithms | message: Phase 3 entry; object: Phase 4 entry |
| D44 | Controller buffering/flow-control capacity at each supported physical profile | Phase 2 prototype; Phase 7 release profile |
| D45 | Packet sequence wrap, duplicate-recognition window, and recovery lifetime | Phase 0 exit |
| D46 | Stream/message transaction-ID representation and idempotent commit key | Phase 0 exit; object transfer ID finalized at Phase 4 entry |
| D47 | Administrative station replacement/rebinding treatment of pending messages/objects | Phase 3 entry |
| D48 | Station online/unavailable liveness, cable/activity evidence, and disconnect timeout policy | Phase 3 entry |
| D49 | Spectrum Z80 assembler/build/tape-packaging toolchain for physical stack development | Phase 2 entry |
| D50 | Router daemon and virtual-proof implementation language/build system | Phase 0 exit |
| D51 | Raspberry Pi deployment OS/service baseline and release packaging | Phase 5 entry |

Future performance enhancements may remain deferred after Phase 7 only when
explicitly classified as future features and not required by Section 31.


## 33. Requirements Traceability Matrix

| ID | Requirement | Sections |
|---|---|---|
| R01 | Issue 2 48K physical target only | 2, 3, 6 |
| R02 | MIC uplink and EAR downlink | 1, 6, 12 |
| R03 | Signal and return on each mono socket | 1, 6 |
| R04 | Central active router | 1, 5, 10 |
| R05 | Raspberry Pi-class logical router | 1, 9, 10 |
| R06 | Deterministic pulse timing outside Linux user-space | 8, 9, 31 |
| R07 | Independent active-star ports for many Spectrums | 6, 8 |
| R08 | No tied Spectrum MIC outputs | 2, 6, 31 |
| R09 | Six-character unique alphanumeric station names | 4, 11, 31 |
| R10 | Two-byte area-plus-node addresses | 4, 11 |
| R11 | Ordinary station designator is `"x.y"` or `"AAAAAA"` | 4, 11, 16 |
| R12 | Reserved router service is `ROUTER` / `0.0` | 11, 17, 19, Appendix A |
| R13 | Router maintains name/address mapping | 10, 11 |
| R14 | Router authenticates source by physical/virtual port | 5, 11, 26 |
| R15 | Interface 1-style front end only | 16, 17, 31 |
| R16 | FORMAT | 17 |
| R17 | SAVE *, LOAD *, VERIFY *, MERGE * | 17 |
| R18 | OPEN #, PRINT #, INPUT #, INKEY$ #, CLOSE # | 17 |
| R19 | MOVE | 17, 32 |
| R20 | Normal `0 OK` feedback | 18 |
| R21 | Interface 1-like network errors | 18, Appendix C |
| R22 | Unknown versus unavailable distinction | 18, 31 |
| R23 | High-memory resident stack | 20 |
| R24 | Tape distribution | 21 |
| R25 | BASIC error-path hook preserves unrelated errors | 20, 31 |
| R26 | No payload reception in interrupt handler | 19, 20, 31 |
| R27 | Asynchronous router queueing | 15, 19 |
| R28 | Slow EAR attention indication | 19 |
| R29 | Explicit cooperative message retrieval | 19 |
| R30 | Message retained until validated acknowledgment | 15, 19, 25 |
| R31 | Stable FIFO queue selection | 15, 31 |
| R32 | Durable queue commit before `0 OK` | 15, 25, 31 |
| R33 | Offline destination rejected in initial release | 12, 15, 31 |
| R34 | Separate core API and BASIC front end | 5, 22 |
| R35 | Pulse-level WZSN integration uses canonical master ticks | 29, 31 |
| R36 | No direct SendByte shortcut in fidelity mode | 29, 31 |
| R37 | No modern NET/MSG/WHO/GET vocabulary | 16, 31 |
| R38 | Local-only initial administration; authenticated remote management | 10, 26, 31 |
| R39 | Bounded resource use | 25, 26, 30, 31 |
| R40 | Explicit deferred-decision gates | 30, 32 |
| R41 | Architecture #3 starts only after WZSN core/UI prerequisite completion | preamble, 30 |
| R42 | Detailed testing and acceptance criteria | 28, 31 |
| R43 | FORMAT registration exempt from `Station not set` precondition | 17, 18, 24, 31 |
| R44 | INKEY$ is genuinely nonblocking | 17, 31 |
| R45 | Bootstrap tape, attention, and packet transmission are exclusive port modes | 9, 19, 21, 24, 31 |
| R46 | Resident RAM/hook protection and cooperative-software limitation | 20, 31 |
| R47 | Idempotent retry plus documented at-least-once failure semantics | 15, 24, 25, 31 |
| R48 | One link frame carries one routed packet and one packet CRC definition | 14, Appendix B, 31 |
| R49 | Controller host-loss/reset forces safe idle | 9, 24, 25, 31 |
| R50 | Queued/staged data binds to immutable internal station identity | 15, 23, 31 |
| R51 | Online-at-commit rule plus retention after later disconnect | 12, 15, 31 |
| R52 | No automatic wall-clock expiry of initial committed messages | 15, 31 |
| R53 | Logical transaction IDs plus packet sequence/duplicate window | 14, 25, 30, 32 |
| R54 | Virtual router reuses router protocol/core with injected deterministic time | 29, 31 |
| R55 | WZSN speed changes host pacing, not virtual protocol timing | 29, 31 |
| R56 | Deterministic station liveness/unavailable policy | 12, 24, 30, 32, 31 |
| R57 | Internal station IDs are immutable and non-reused | 15, 23, 31 |
| R58 | Immediate-mode hook/memory contracts freeze before Phase-3 BASIC network use | 20, 30, 32 |
| R59 | Implementation/deployment toolchains are explicit pre-ticket gates | 30, 32 |
| R60 | Ear+Mic resident BASIC front end and active Interface 1 are mutually exclusive | 2, 20, 29, 31 |
| R61 | WZSN Architecture #3 uses the upstream NONE/INTERFACE1/EAR_MIC networking-mode arbiter | 2, 29, 31 |
| R62 | Ear+Mic uses distributed RAM stack; no networking-ROM paging or auto-install | 1, 2, 21, 29, 31 |
| R63 | WZSN Ear+Mic exclusively owns cassette EAR/MIC and uses BOOTSTRAP_TAPE for in-mode stack loading | 21, 24, 29, 31 |
| R64 | WZSN Ear+Mic requires certified 48K Issue-2 profile; Control Port multiplicity does not imply cross-process network timing | 2, 29, 31 |


## 34. Architecture Audit Record

The Markdown implementation authority is audited from the saved disk copy, not
from an in-memory draft.

Required audit classes:

1. Markdown/section structure and heading continuity;
2. ASCII-only encoding and malformed-fence detection;
3. frozen/baseline/proposed/open status consistency;
4. every `[OPEN]` body item mapped to a Section-32 decision gate;
5. command grammar versus examples and error semantics;
6. station-name/address/service-endpoint consistency;
7. message queue, durability, FIFO, acknowledgment, and offline semantics;
8. object-transfer lifecycle consistency;
9. Spectrum stack hook/reentrancy/memory-contract consistency;
10. electrical/timing-controller/Linux trust-boundary consistency;
11. WZSN cross-architecture timing and integration consistency;
12. Interface-1 versus Ear+Mic mutual exclusion and BASIC-hook ownership;
13. WZSN 48K Issue-2 availability gating and no networking-ROM paging;
14. cassette EAR/MIC ownership and `BOOTSTRAP_TAPE` exclusivity;
15. ordinary multi-process Control Ports versus deterministic virtual-router timing separation;
16. implementation-phase dependency and exit-gate coverage;
17. acceptance-criterion and traceability coverage; and
18. contradiction/obsolete-wording scan.

A clean audit is not allowed to certify electrical values or hardware behavior
that still require bench measurement. It certifies that every such uncertainty
is explicit and gated before dependent implementation.

Final convergence record:

- structural/contract audit: zero gaps;
- independent semantic/cross-architecture audit: zero gaps;
- the final saved Markdown bytes were then re-audited without modification by
  both audit methods in two successive cycles;
- success criterion: both methods report zero gaps on the identical saved file.

The audit result does not certify any electrical value, physical timing value,
or hardware behavior that Section 32 deliberately leaves to a later measurement
or decision gate.


## Appendix A. Command Grammar


### A.1 Lexical forms

```text
DEVICE ::= "n"
NAME ::= exactly six characters from A-Z and 0-9, excluding reserved service name ROUTER
AREA ::= decimal integer 0..255, subject to frozen reservation rules
NODE ::= decimal integer 0..255, subject to frozen reservation rules
ADDRESS ::= AREA "." NODE
N ::= quoted NAME or quoted ordinary-station ADDRESS
ROUTER_ENDPOINT ::= "ROUTER" | "0.0"
```

Examples:

```text
"MASTER"
"GAME02"
"1.3"
"12.42"
```


### A.2 Network forms

```text
FORMAT "n";N

SAVE *"n";N
SAVE *"n";N LINE line
SAVE *"n";N DATA array()
SAVE *"n";N DATA array$()
SAVE *"n";N CODE address,length
SAVE *"n";N SCREEN$

LOAD *"n";N
LOAD *"n";N DATA array()
LOAD *"n";N DATA array$()
LOAD *"n";N CODE
LOAD *"n";N CODE address
LOAD *"n";N CODE address,length
LOAD *"n";N SCREEN$

VERIFY *"n";N
VERIFY *"n";N DATA array()
VERIFY *"n";N DATA array$()
VERIFY *"n";N CODE address,length
VERIFY *"n";N SCREEN$

MERGE *"n";N

OPEN #stream;"n";N
OPEN #stream;"n";ROUTER_ENDPOINT

PRINT #stream;expression
INPUT #stream;variable
INPUT #stream;LINE string-variable
INKEY$ #stream
CLOSE #stream

MOVE source TO destination
```


### A.3 Router endpoint

```text
"ROUTER"
"0.0"
```

These identify the same local router service endpoint. Neither is assignable
to an ordinary station. `FORMAT "n";"ROUTER"` and `FORMAT "n";"0.0"` are
therefore invalid registration targets. `ROUTER` is accepted only in contexts that explicitly
allow the router endpoint; its lexical shape does not make it an ordinary
station name.

---

## Appendix B. Packet Definitions


### B.1 Initial uplink routed-packet header

```text
Offset  Size  Field
------  ----  ----------------
0       1     protocol version
1       1     packet type
2       1     destination area
3       1     destination node
4       2     sequence
6       2     payload length
8       N     payload
8+N     2     CRC-16
```

The source is omitted and derived by the router from the authenticated physical
or virtual port binding.


### B.2 Initial downlink routed-packet header

```text
Offset  Size  Field
------  ----  ----------------
0       1     protocol version
1       1     packet type
2       1     source area
3       1     source node
4       1     destination area
5       1     destination node
6       2     sequence
8       2     payload length
10      N     payload
10+N    2     CRC-16
```

The exact packet-type numbers, CRC definition, payload limit, and transaction-ID
encoding are frozen by the gates in Sections 30 and 32.


### B.3 Object metadata

  object type
  object length
  load address where relevant
  execution or autostart metadata where relevant
  array metadata where relevant
  complete-object checksum


### B.4 Logical transaction identity

Stream/message and object control payloads establish or carry a logical
`stream_id` / `transfer_id` separate from the common packet sequence field.
The exact width/encoding is frozen by D46. Packet sequence numbers order/retry
packets within the frozen recovery window; they are not a substitute for the
logical operation identifier.


### B.5 Message envelope

  message identifier
  source address
  source name snapshot
  payload length
  payload
  payload checksum

---

## Appendix C. Error Catalog

Code assignment is gated by D25. Semantic distinctions/text are baseline.

- Invalid station
- Missing station
- Station name in use
- Station not set
- Station not found
- Station not available
- Station busy
- Network timeout
- Network not present
- Network data error
- Network version error
- Transfer rejected
- Verification has failed

Required semantic examples:

  OPEN #4;"n";"BAD-01"
    Invalid station

  OPEN #4;"n";"NOEXST"
    Station not found

  OPEN #4;"n";"GAME02"
    Station not available
    when GAME02 exists but is disconnected

  FORMAT "n";"MASTER"
    Station name in use
    when MASTER belongs to another active station

  SAVE *"n";"GAME02" CODE 32768,4096
    Transfer rejected
    when the router or destination explicitly refuses the object

---

## Appendix D. Example Sessions


### D.1 Registration

  FORMAT "n";"MASTER"

  0 OK, 0:1


### D.2 Unknown station

  OPEN #4;"n";"NOEXST"

  s Station not found, 0:1

The report letter "s" is illustrative only.


### D.3 Known but unavailable station

  OPEN #4;"n";"GAME02"

  t Station not available, 0:1

The report letter "t" is illustrative only.


### D.4 Send asynchronous message

  OPEN #4;"n";"GAME02"
  PRINT #4;"READY TO START"
  CLOSE #4

  0 OK, 0:1

Meaning:

  The router accepted and queued the complete message for the online registered
  station GAME02. The recipient may not have read it yet.


### D.5 Receive next message from any sender

  OPEN #4;"n";"ROUTER"
  INPUT #4;S$,M$
  CLOSE #4

Possible values:

  S$ = "MASTER"
  M$ = "READY TO START"


### D.6 Receive from one source

  OPEN #4;"n";"MASTER"
  INPUT #4;M$
  CLOSE #4


### D.7 Send CODE

  SAVE *"n";"GAME02" CODE 32768,4096


### D.8 Receive CODE

  LOAD *"n";"MASTER" CODE 32768,4096


### D.9 Verify screen

  VERIFY *"n";"GAME02" SCREEN$


### D.10 Address form

  OPEN #4;"n";"1.3"
  PRINT #4;"HELLO"
  CLOSE #4

---

## Appendix E. Provisional Memory Map

This map is illustrative and is not a frozen allocation.

```text
C000-CFFF  physical link and packet code
D000-D7FF  router protocol and name resolution
D800-DFFF  stream/message and object services
E000-EFFF  BASIC parser and command dispatch
F000-F7FF  transmit and receive buffers
F800-FFFF  hook state, attention detector, workspace
```

The final map must be generated from the assembled binary and verified against:

  BASIC program space
  variables
  calculator stack
  machine stack
  UDGs
  system variables
  screen memory
  interrupt requirements

---

## Appendix F. Provisional Timing Profile

The following values are engineering starting points only.

```text
pilot half-pulse  2168 T-states
sync 1             667 T-states
sync 2             735 T-states
zero half-pulse    855 T-states
one half-pulse    1710 T-states
pilot count         64 half-pulses
inter-frame idle     3 milliseconds minimum
transfer mode        half-duplex
chunk window          1 packet
initial payload      32 or 64 bytes (Phase-0 selection required)
```

Every value in this appendix must be validated on real Issue 2 hardware before
it becomes normative for the physical release. In the WZSN virtual harness,
T-state values are represented on the core architecture's canonical master-tick
timeline using the selected 48K PAL machine profile; the router architecture
must not introduce an independent emulator-time base.
