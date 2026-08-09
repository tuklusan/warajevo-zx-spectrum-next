<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Warajevo ZX Spectrum Next - Core and System Architecture

This document is the canonical authority for emulation-core, machine-timing,
host-boundary, media-engine, build, regression, and Telnet transport/keyboard
semantics. User-interface structure, the shared application command registry,
toolbar/menu behavior, media-manager presentation, compatibility-tool surfaces,
and Telnet application-control commands are defined by the companion document
`design/warajevo-zx-spectrum-next-ui-architecture.md`. Neither document may
override the deterministic machine contracts owned by this document.

## 1. Architectural mission

Warajevo ZX Spectrum Next is a modern continuation of Warajevo, not a mechanical
source-language translation of Warajevo 2.50.

Its design goals are:

1. preserve the knowledge encoded in the original Warajevo source;
2. preserve user-visible and peripheral behavior where it is correct;
3. replace DOS/x86 host mechanisms with portable modern mechanisms;
4. implement the emulated Spectrum in portable, standards-defined C;
5. make machine state deterministic across CPU architectures and operating
   systems;
6. make the ULA, bus, contention, interrupt, I/O, and memory timing accurate
   enough for software that depends on exact raster position;
7. produce deterministic beeper and AY audio from emulated time;
8. compile the host layer, including Sokol, into the Warajevo executable;
9. produce one application binary per supported platform, subject only to
   normal operating-system libraries/frameworks;
10. make host presentation replaceable without modifying emulation code.

Accuracy is the canonical priority. Performance optimizations are acceptable
only when they are proven behaviorally equivalent.

---

## 2. Sources of truth

Warajevo ZX Spectrum Next project repository:

```text
https://github.com/tuklusan/warajevo-zx-spectrum-next.git
```

Warajevo ZX Spectrum Next has more than one reference source, and they have
different authority.

### 2.1 Original Warajevo 2.50

Upstream legacy source repository:

```text
https://github.com/tuklusan/warajevo-spectrum-2.50.git
```

### 2.1.1 Surviving-source provenance boundary

The official Warajevo download page states that the authors no longer had the
complete Warajevo 2.51 sources and had only the 2.50 sources. It further states
that some 2.51 GUI patches were reapplied to those 2.50 sources, while some
2.51 changes were lost.

The official revision history describes release 2.52 as behaviorally identical
to 2.51 except for the change to the GPL and publication of source. No complete
matching 2.51 or 2.52 source tree is presently known to this project.

Therefore:

- the preserved Warajevo 2.50 source is the canonical migration-source baseline;
- surviving 2.51/2.52 binaries, documentation, revision notes, sample files, and
  observable behavior may be used as secondary behavioral evidence;
- no migration ticket may assume that a complete 2.51 or 2.52 source routine is
  available;
- a behavior present in 2.51/2.52 but absent from the preserved 2.50 source must
  be treated as a reconstruction/research task, not as an ordinary source-port
  task;
- if a later source discovery is claimed, its provenance and hash must be
  verified before it is admitted as an implementation authority.

Historical source pages used for this provenance rule:

```text
https://worldofspectrum.net/warajevo/Download.html
https://worldofspectrum.net/warajevo/Revision.html
```

The preserved Warajevo 2.50 source is the primary reference for:

- existing Warajevo feature behavior;
- media handling;
- snapshot handling;
- tape behavior as implemented by Warajevo;
- Microdrive behavior as implemented by Warajevo;
- Interface 1 behavior as implemented by Warajevo;
- historical ZX Net behavior as implemented by Warajevo;
- monitor/debugger behavior;
- configuration semantics;
- existing user-facing workflows;
- historical compatibility workarounds;
- routine provenance.

The preserved source modules include, among others:

```text
WARAJEVO.PAS
SPECSIM.ASM
Z80.ASM
TAPE.ASM
MDRIVE.ASM
SPECMON.ASM
ZXPRINT.ASM
SPECLOGO.ASM
```

The original Pascal source and original x86 assembly are equally valid inputs
to the migration. Neither language is privileged in the new implementation.

### 2.2 Real ZX Spectrum hardware behavior

Where the original Warajevo timing model, workaround tables, or host-era
implementation differs from real Spectrum behavior, authentic hardware behavior
is authoritative for the new timing engine.

In particular, the following must not be treated as correct merely because the
old emulator did them:

- instruction-end-only timing;
- application-specific timing offsets;
- raster compatibility hacks;
- host-timer-dependent behavior;
- x86 instruction scheduling assumptions.

### 2.3 Documentation and hardware research

Machine timing constants, contention behavior, ULA bus activity, I/O behavior,
and peripheral behavior must be corroborated against reliable hardware
documentation and, when necessary, real-machine measurement.

### 2.4 Regression corpus

Known software and timing tests are evidence, not the definition of the
hardware.

Programs such as overscan demonstrations, rainbow/multicolor demonstrations,
NIRVANA/NIRVANA+, BIFROST-class software, and historically troublesome
Warajevo test programs are useful regression evidence. Difficult third-party
media used only for development may live solely in the private directory defined
by Section 36.2 and need not become public project artifacts.

The complete **Fuse Z80 unit-test suite** is a mandatory external CPU
conformance corpus. Its upstream revision must be pinned for each certification
baseline; Section 36.1 defines the execution and waiver policy.

The implementation must model the hardware mechanism that makes them work
rather than recognize individual programs.

### 2.5 Legacy Warajevo feature disposition

The modern project must not hide product scope behind phrases such as
"remaining Warajevo facilities." Every historically significant facility is
assigned an explicit disposition.

```text
Feature / facility                         Disposition for first architecture-complete milestone
-----------------------------------------  -----------------------------------------------------
ZX Spectrum 48K PAL                        REQUIRED - initial certified machine
ZX Spectrum 128K PAL                       REQUIRED - initial certified machine
ZX Spectrum +2                             LATER compatibility target; not an initial blocker
Timex Sinclair 2068                        LATER compatibility target; preserved source retained
DCK/Timex memory expansions                LATER with Timex support
Z80 CPU incl. documented/undocumented      REQUIRED
ULA/border/contention/floating bus          REQUIRED
48K keyboard matrix                        REQUIRED
Kempston joystick                          REQUIRED initial joystick interface
Beeper                                     REQUIRED
AY-3-8912 on 128K                          REQUIRED
Tape: standard TAP                         REQUIRED
Tape: Warajevo native TAP                  REQUIRED compatibility path
Tape: TZX                                  REQUIRED
Tape: WAV/audio input                      REQUIRED deterministic decode path
Live physical cassette capture              LATER; not an initial blocker
Snapshots: SNA 48K/128K                    REQUIRED
Snapshots: Z80                             REQUIRED
Interface 1                                REQUIRED
Microdrive / MDR                           REQUIRED
Original Sinclair/ZX Net behavior          REQUIRED
ZX Printer                                 REQUIRED legacy peripheral
Built-in monitor/debugger                  REQUIRED legacy workflow
Runtime speed control                      REQUIRED, redesigned as host pacing
128K MIDI interface                        LATER; not an initial blocker
128K extended keypad                       LATER; not an initial blocker
Historical RS-232 host redirection         LATER host-integration work; authentic IF1 state first
Historical external plug-in ABI            NOT an initial compatibility requirement
ZXCOMP executable-snapshot compiler        LATER/separate tool; not part of initial emulator binary
Historical database/shell/help system      REPLACE with modern UI where functionality is retained
Historical file-conversion utilities       LATER utility work unless required by a media test
DOS/BIOS/video/sound host mechanisms       REPLACE; never compatibility targets themselves
```

A change to a disposition is an explicit architecture change. A ticket may not
silently drop a REQUIRED item or promote a LATER item into a release blocker.

---

## 3. Language and implementation contract

### 3.1 Core language

The emulator core and normal application logic are written in portable C.

The emulator core and normal application logic target **ISO C11**.

The code must remain within behavior that is well-defined by ISO C11 and
supported by the target compilers. Later adoption of a newer C revision is an
explicit architecture/build change and must not alter canonical core results.

### 3.2 No source-language preservation requirement

Original Pascal code may become C.

Original x86 assembly may become C.

The goal is:

```text
preserve behavior and provenance
not
preserve implementation language or host tricks
```

### 3.3 Platform-specific compilation exception

The emulation core remains pure C.

A platform host translation unit may be compiled in the platform mode required
by Sokol or the operating system. For example, the Sokol implementation on
macOS may need to be compiled as Objective-C while exposing only a C interface
to the rest of Warajevo.

No Objective-C, Win32, X11, Metal, D3D, OpenGL, ALSA, CoreAudio, or WASAPI type
may appear in the emulation core.

---

## 4. Proposed source-tree architecture

```text
Warajevo-ZX-Spectrum-Next/
|
+-- LICENSE.txt
+-- NOTICE.md
+-- CMakeLists.txt
|
+-- cmake/
|   +-- toolchains/
|
+-- design/
|   +-- warajevo-zx-spectrum-next-architecture.md
|   +-- warajevo-zx-spectrum-next-ui-architecture.md
|   +-- zx48-mic-ear-router-network-architecture.md   (downstream Architecture #3)
|   +-- machine-timing-evidence.md
|   +-- media-format-support.md
|   +-- migration-ledger.md
|   +-- interface1-microdrive-zxnet.md
|
+-- src/
|   |
|   +-- core/
|   |   +-- wz_types.h
|   |   +-- wz_machine.h
|   |   +-- wz_machine.c
|   |   +-- wz_scheduler.h
|   |   +-- wz_scheduler.c
|   |
|   +-- cpu/
|   |   +-- wz_z80.h
|   |   +-- wz_z80.c
|   |   +-- wz_z80_flags.c
|   |   +-- wz_z80_decode.c
|   |
|   +-- bus/
|   |   +-- wz_bus.h
|   |   +-- wz_bus.c
|   |   +-- wz_memory.c
|   |   +-- wz_ports.c
|   |   +-- wz_contention.c
|   |
|   +-- video/
|   |   +-- wz_ula.h
|   |   +-- wz_ula.c
|   |   +-- wz_raster.h
|   |   +-- wz_raster.c
|   |   +-- wz_palette.c
|   |
|   +-- audio/
|   |   +-- wz_beeper.h
|   |   +-- wz_beeper.c
|   |   +-- wz_ay8912.h
|   |   +-- wz_ay8912.c
|   |   +-- wz_audio_mixer.h
|   |   +-- wz_audio_mixer.c
|   |
|   +-- media/
|   |   +-- wz_tape.c
|   |   +-- wz_snapshot.c
|   |   +-- wz_microdrive.c
|   |
|   +-- peripherals/
|   |   +-- wz_interface1.c
|   |   +-- wz_zxnet.c
|   |   +-- wz_keyboard.c
|   |   +-- wz_joystick.c
|   |   +-- wz_printer.c
|   |
|   +-- machines/
|   |   +-- wz_48k.c
|   |   +-- wz_128k.c
|   |   +-- wz_machine_profile.h
|   |
|   +-- host/
|   |   +-- wz_sokol_host.h
|   |   +-- wz_sokol_host.c
|   |   +-- wz_sokol_host_macos.m
|   |   +-- wz_socket.h
|   |   +-- wz_socket_posix.c
|   |   +-- wz_socket_win32.c
|   |
|   +-- diagnostics/
|   |   +-- wz_trace.h
|   |   +-- wz_trace.c
|   |   +-- wz_trace_file.h
|   |   +-- wz_trace_file.c
|   |
|   +-- features/
|   |   +-- telnet/
|   |       +-- wz_telnet_server.h
|   |       +-- wz_telnet_server.c
|   |       +-- wz_telnet_protocol.c
|   |
|   +-- app/
|       +-- wz_app.c
|       +-- wz_config.c
|       +-- wz_command_registry.h
|       +-- wz_command_registry.c
|       +-- wz_media_manager.c
|       +-- wz_screenshot_service.c
|
+-- third_party/
|   +-- sokol/
|
+-- tests/
|   +-- cpu/
|   +-- bus/
|   +-- ula/
|   +-- raster/
|   +-- audio/
|   +-- tape/
|   +-- microdrive/
|   +-- interface1/
|   +-- zxnet/
|   +-- snapshots/
|   +-- telnet/
|   +-- command_registry/
|   +-- ui/
|   +-- determinism/
|   +-- regression/
|   +-- corpus/
|       +-- cpu/
|       +-- timing/
|       +-- media/
|
+-- reference/
    +-- original-warajevo/
```

File names are provisional. Module boundaries and dependency direction are
architectural requirements.

---

## 5. Dependency direction

The deterministic Spectrum core has **no host dependency**.

Application/orchestration code owns the relationship between the core and host
services:

```text
                         application/orchestrator
                         /                    \
                        v                      v
+================================+   +===============================+
|   DETERMINISTIC SPECTRUM CORE  |   |          HOST SERVICES        |
|                                |   |                               |
| Z80 -> bus -> memory/ports     |   | Sokol app/gfx/audio/time     |
|          |                     |   | native socket wrapper         |
|          +-> ULA/raster        |   | local keyboard event source   |
|          +-> tape              |   | Telnet keyboard event source  |
|          +-> IF1/MDV/ZX Net    |   | files/window/UI               |
|          +-> beeper/AY         |   +===============================+
|                                |                 |
| consumes normalized input      |                 |
| produces canonical output      |                 |
+================================+                 |
                        ^                          |
                        |                          |
                        +------- orchestrator -----+
```

The core consumes only deterministic, normalized machine inputs such as:

```text
keyboard matrix transition at master tick N
joystick transition at master tick N
media state change at a defined emulated boundary
```

The core produces only machine-domain results such as:

```text
canonical raster
canonical audio
machine state
timing trace
```

The application/orchestrator is responsible for:

- converting local and Telnet host events into normalized input events;
- applying source ownership/merging before those events enter the core;
- scheduling normalized input at an emulated master tick;
- sending canonical raster/audio to Sokol;
- host pacing;
- file dialogs and application UI;
- network listener lifecycle.

Forbidden core dependencies include:

```text
Z80        -> Sokol
bus        -> Sokol
ULA        -> Sokol
raster     -> Sokol
tape       -> Sokol
AY         -> Sokol
Microdrive -> Sokol
ZX Net     -> Sokol
keyboard   -> Telnet
keyboard   -> local host key codes
core       -> socket API
core       -> host clock API
```

This separation is an architectural requirement, not merely a source-tree
preference.

---

## 6. Host-independence and determinism contract

Given the same:

- machine model;
- ROM bytes;
- initial machine state;
- media bytes;
- configuration affecting emulation;
- deterministic input-event trace;

the core must produce the same machine state at the same canonical emulated
master tick on
every supported host.

The result must not depend on:

- x86-64 versus AArch64;
- Windows versus Linux versus macOS;
- compiler register allocation;
- host pointer width;
- host byte order;
- host memory alignment tolerance;
- signed `char` policy;
- enum representation;
- structure padding;
- host floating-point optimizations;
- host CPU flags;
- host instruction execution time;
- GPU backend;
- monitor refresh rate;
- DPI;
- audio device sample rate;
- audio callback size;
- host thread scheduling;
- Sokol frame timing.

Cross-platform divergence is a defect unless the differing state is explicitly
classified as non-emulated host state.

---

## 7. Defined C behavior contract

Portable C creates different hazards from the original Pascal/x86 code. The
core must actively avoid undefined, implementation-defined, or
architecture-sensitive behavior where it can affect machine results.

### 7.1 Explicit-width integer types

Use `<stdint.h>` types for machine state:

```c
uint8_t
uint16_t
uint32_t
uint64_t
int8_t
int16_t
int32_t
int64_t
```

Examples:

```c
typedef uint8_t  wz_byte_t;
typedef uint16_t wz_addr_t;
typedef uint64_t wz_master_tick_t;
```

Do not use plain `int`, `long`, pointers, or enums as storage formats for Z80
registers, Spectrum addresses, or persistent machine state unless their
representation is irrelevant and proven so.

### 7.2 Signed overflow

Signed integer overflow must never be used to implement Z80 wraparound.

Use explicitly widened unsigned arithmetic.

Example:

```c
uint16_t sum = (uint16_t)a + (uint16_t)b;
uint8_t result = (uint8_t)sum;
```

### 7.3 Shifts

Do not depend on:

- shifting by a width equal to or greater than the operand width;
- implementation-defined right shift of negative signed values;
- implicit integer promotion producing unintended signed arithmetic.

### 7.4 Aliasing and type punning

Do not depend on strict-aliasing violations or pointer casts to reinterpret
machine bytes as wider host values.

### 7.5 Alignment

Do not assume unaligned 16-bit, 32-bit, or 64-bit host memory access is legal.

### 7.6 Endianness

The Z80's little-endian memory semantics are implemented explicitly.

Example:

```c
uint16_t wz_read16(wz_machine_t* m, uint16_t address) {
    uint8_t lo = wz_bus_read8(m, address);
    uint8_t hi = wz_bus_read8(m, (uint16_t)(address + 1));
    return (uint16_t)lo | ((uint16_t)hi << 8);
}
```

The host's byte order must never define Spectrum behavior.

### 7.7 Structure layout

C structures are convenient in memory but their raw byte layout is not a file
format, hash format, or network format.

### 7.8 Booleans and enums

Boolean and enum representations must not be serialized or hashed by raw
memory-copy of structures.

### 7.9 Sanitizer policy

Development builds must support compiler sanitizers where the selected compiler provides them,
including undefined-behavior and memory-safety checking.

A sanitizer failure in core code is treated as a correctness defect.

---

## 8. Machine state and serialization

### 8.1 Machine state

Machine state contains only emulated data and deterministic scheduler state.

Conceptually:

```c
typedef struct {
    wz_z80_t       cpu;
    wz_bus_t       bus;
    wz_ula_t       ula;
    wz_audio_t     audio;
    wz_tape_t      tape;
    wz_networking_mode_t networking_mode;
    wz_if1_t       interface1;
    wz_microdrive_t microdrive;
    wz_zxnet_t       zxnet;
    wz_master_tick_t master_tick;
} wz_machine_t;
```

The exact layout is not frozen.

### 8.2 No raw structure serialization

Never implement a portable save state with:

```c
fwrite(&machine, sizeof(machine), 1, file);
```

Persistent state must use explicitly defined fields, widths, byte order,
versions, and checksums.

### 8.3 Canonical state hashing

Cross-platform determinism hashes must be produced from a canonical
serialization of emulated state, not `sizeof(struct)` bytes.

### 8.4 Host state excluded

Window size, window position, GPU resources, audio queue fill, file-dialog
state, and similar host details are not included in deterministic machine
state.

---

## 9. Machine profiles

### 9.1 Certified machine scope

The first architecture-complete milestone certifies exactly these emulated
machines:

```text
Machine                         Initial status   Certification phase
------------------------------  ---------------  -------------------
ZX Spectrum 48K PAL             REQUIRED         Phase 4 / Gate 4A
ZX Spectrum 128K PAL            REQUIRED         Phase 9
ZX Spectrum +2                  LATER            separate later profile
Timex Sinclair 2068             LATER            separate later profile
16K / +2A / +3 / clones         FUTURE RESEARCH  not initial commitments
```

The initial 48K and 128K profiles must explicitly identify the ROM hash and the
hardware/ULA assumptions used by each certified regression run. Where Issue,
ULA revision, memory, or other model differences are hardware-observable, they
must be represented by explicit profile data or a documented profile variant;
they must not be approximated by host randomness.

A later machine may reuse common devices, but certification of one profile does
not imply certification of another.

### 9.2 Profile structure

Machine-specific timing and hardware differences are data or explicit
machine-profile behavior rather than scattered constants.

Each profile defines one canonical integer master timing domain and the exact
relationships from that domain to the CPU, ULA, AY, and other synchronous
hardware clocks.

Conceptual profile:

```c
typedef struct {
    uint64_t master_hz_num;
    uint64_t master_hz_den;

    uint32_t master_ticks_per_cpu_tstate;

    uint16_t tstates_per_line;
    uint16_t lines_per_frame;
    uint32_t tstates_per_frame;

    uint16_t raster_clocks_per_line;
    uint16_t active_width;
    uint16_t active_height;

    /* ULA/AY divisors or rational clock relationships */
    /* contention, interrupt, memory-map and device parameters */
} wz_machine_profile_t;
```

The master frequency is represented exactly as a rational quantity. CPU
frequency and other derived frequencies are calculated from the master clock
relationship rather than stored as independent competing timing authorities.

Other profile relationships may use integer divisors, rational ratios, or
validated tables as appropriate. No profile may require floating-point time to
define machine behavior.

Examples for the classic PAL models include:

```text
48K:
    224 CPU T-states per line
    312 lines per frame
    69888 CPU T-states per frame
    448 ULA pixel-clock positions per line

128K:
    228 CPU T-states per line
    311 lines per frame
    70908 CPU T-states per frame
    456 ULA pixel-clock positions per line
```

The master-tick frequency and clock-ratio fields for each machine profile must
be frozen from validated hardware evidence before that profile is certified.
The evidence, source citations, measured values, uncertainties, and final frozen
constants are recorded in `design/machine-timing-evidence.md`.

Exact raster origins, contention tables, interrupt windows, blanking regions,
same-edge event ordering, and model-specific ULA behavior must likewise be
validated before being frozen.

---

## 10. Global emulated time

### 10.1 One authoritative master timeline

Every emulated device shares one monotonically increasing integer master-tick
timeline.

```c
typedef uint64_t wz_master_tick_t;
```

A master tick represents one tick of the selected machine profile's canonical
master timing domain.

The CPU T-state is not the universal machine clock.

### 10.2 Master tick is the canonical scheduler timestamp

The canonical timestamp stored by the deterministic scheduler is:

```text
master_tick
```

not:

```text
CPU T-state only
```

The machine profile defines the conversion between master ticks and CPU
T-states.

Where the ratio is integral:

```text
cpu_tstate = master_tick / master_ticks_per_cpu_tstate
cpu_phase  = master_tick % master_ticks_per_cpu_tstate
```

If a later supported machine requires a more general rational relationship,
the profile must still provide an exact integer/rational mapping without
floating-point machine time.

### 10.3 T-state + phase is a derived view

Z80-oriented code, traces, and debuggers may display:

```text
T-state N, phase P
```

This is a human/CPU-oriented coordinate derived from the canonical master
tick.

ULA-oriented diagnostics may derive:

```text
frame
scanline
raster clock
```

from the same master tick.

All are coordinate systems over one canonical timeline.

### 10.4 Same-time ordering

Using a master tick prevents events from being forced onto the same timestamp
merely because a CPU T-state was too coarse.

Where two real hardware effects genuinely occur on the same master-clock edge,
the machine profile must define their ordering/visibility according to hardware
evidence.

C function-call order must never decide same-edge hardware behavior.

### 10.5 Host clock is not machine clock

Host time is used for:

- pacing;
- sleeping;
- performance measurement;
- UI responsiveness statistics.

Host time is not used to decide:

- when a Z80 memory write occurs;
- when an I/O write occurs;
- when the ULA fetches a byte;
- when an interrupt is sampled;
- when contention delays a bus cycle;
- when a border color changes;
- when the beeper changes;
- when an AY register changes;
- when a tape edge changes.

### 10.6 Presentation may be dropped; emulation may not

If the host cannot present every generated frame, the host may drop display
presentations.

It must not skip emulated ULA frames or change machine timing merely to maintain
host frame rate.

### 10.7 Runtime speed control

Warajevo ZX Spectrum Next exposes a runtime speed control in the application menu.

The speed control is a host-pacing multiplier. It is not an emulated hardware
clock modification.

Baseline choices include:

```text
25%
50%
100%   authentic real-time pacing
200%
400%
800%
Unlimited
```

At every setting, the internal machine clock relationships remain unchanged.

For a 48K PAL machine:

```text
224 CPU T-states per line
312 lines per frame
69888 CPU T-states per frame
```

remain true at 50%, 100%, 200%, or Unlimited host execution speed.

A 200% setting means that the host attempts to let the same amount of emulated
master-tick progress occur in approximately half the wall-clock time used at
100%.

### 10.8 Runtime speed changes

A speed setting may be changed while the emulator is running.

The change affects only the mapping between future emulated progress and host
wall-clock pacing.

No discontinuity may be introduced into the canonical master-tick timeline.

Changing speed must not reset:

- master tick;
- CPU T-state/phase;
- frame/raster position;
- CPU state;
- tape position;
- AY phase;
- beeper state;
- peripheral timers.

### 10.9 Unlimited mode

Unlimited mode removes normal wall-clock throttling and runs the deterministic
core as quickly as the host permits.

All machine frames and timed device events are still emulated.

The host may omit presentation of intermediate completed frames to reduce GPU
work, but it may not skip their ULA execution or other machine effects.

### 10.10 Runtime-speed audio policy

Host sound output is enabled only when the selected runtime speed is within:

```text
0.5x <= speed <= 2.0x
```

inclusive.

Therefore:

```text
speed < 0.5x       host sound muted
0.5x .. 2.0x       host sound enabled and follows runtime speed
speed > 2.0x       host sound muted
Unlimited          host sound muted
```

Muting is purely a host-presentation policy.

Even while host sound is muted:

- beeper state continues to advance;
- AY state continues to advance;
- canonical audio state remains deterministic;
- audio-related machine timing remains exact.

Crossing the 0.5x or 2.0x boundary at runtime must not reset or perturb
emulated audio hardware state.

Within the enabled range, host delivery must follow the selected speed rather
than allowing audio-buffer pressure to force the emulator back toward 1.0x.

Within the enabled 0.5x..2.0x range, audio follows emulator speed naturally:
duration and pitch change with the selected runtime speed. The architecture does not require pitch-preserving time stretching.

The exact resampling implementation is a host-presentation detail, but it must
not alter canonical core state.

### 10.11 Speed control versus emulated overclocking

Runtime emulator speed and emulated hardware clock modification are different
features.

```text
runtime speed control:
    same Spectrum hardware timing
    different wall-clock pacing

emulated overclock:
    altered relationship among CPU and hardware clocks
    different emulated machine
```

This architecture defines only runtime speed control.

Any future emulated overclock option must be implemented as an explicit machine
configuration/profile and must not reuse the host-pacing control.

---

## 11. Z80 execution architecture

### 11.1 Behavioral requirements

The Z80 core must model:

- primary opcode set;
- CB-prefixed instructions;
- ED-prefixed instructions;
- DD/FD-prefixed instructions;
- DDCB/FDCB forms;
- documented flag behavior;
- compatibility-relevant undocumented flag behavior;
- alternate register set;
- I and R registers;
- HALT;
- EI/DI timing;
- interrupt modes;
- interrupt acceptance;
- NMI;
- refresh behavior;
- memory accesses;
- I/O accesses.

### 11.2 No instruction-end-only external effects

This is forbidden as the final timing model:

```text
execute complete instruction
mutate all memory/ports
add total instruction T-states
```

For timing-sensitive emulation, externally visible operations occur at the
correct point within the instruction.

### 11.3 Bus-cycle decomposition

Instruction execution must be capable of representing:

```text
M1 opcode fetch
memory read
memory write
I/O read
I/O write
interrupt acknowledge
internal CPU cycles
refresh-related behavior
```

Each external bus operation is associated with an exact master tick.

### 11.4 Implementation freedom

The CPU implementation may use:

- explicit opcode handlers;
- decode tables;
- generated tables;
- micro-operations;
- another traceable approach.

Accuracy and auditability are mandatory. Dispatch cleverness is not a design
goal.

---

## 12. Bus architecture

The CPU does not directly mutate arbitrary machine state.

Externally observable accesses cross the machine bus:

```text
Z80
 |
 +--> memory read
 +--> memory write
 +--> I/O read
 +--> I/O write
 +--> interrupt acknowledge
 |
 v
BUS
 |
 +--> contention
 +--> memory map
 +--> ULA-visible RAM
 +--> paging
 +--> keyboard
 +--> tape/EAR
 +--> ULA/port FE
 +--> AY
 +--> Interface 1
 +--> peripherals
```

Conceptual interfaces:

```c
uint8_t wz_bus_mem_read(wz_machine_t* m, uint16_t address);
void    wz_bus_mem_write(wz_machine_t* m, uint16_t address, uint8_t value);

uint8_t wz_bus_io_read(wz_machine_t* m, uint16_t port);
void    wz_bus_io_write(wz_machine_t* m, uint16_t port, uint8_t value);
```

Actual APIs may include cycle descriptors and explicit timing information.

---

## 13. Contention and arbitration

Contention belongs to the bus/machine timing model, not as arbitrary delays
sprinkled through opcode handlers.

The contention engine receives enough information to determine behavior from:

- machine model;
- current master tick;
- bus-cycle class;
- memory address or I/O port;
- ULA phase;
- paging state where relevant.

CPU progress is delayed by the hardware contention result before the affected
bus operation completes.

The architecture must support:

- contended RAM;
- uncontended RAM where applicable;
- contended I/O;
- machine-model differences;
- future correction of contention tables without rewriting the CPU.

---

## 14. Same-edge event ordering

Exact raster software can depend on whether near-simultaneous events observe
old or new state.

The master-tick architecture distinguishes events that occur on different
master-clock edges even when both belong to the same CPU T-state.

For events that genuinely occur on the same master-clock edge, the scheduler
and machine profile require an explicit, tested hardware-ordering contract for
cases such as:

```text
ULA memory fetch
CPU memory write
CPU I/O write
interrupt edge/state
audio output change
```

Ordering must be derived from actual bus/hardware behavior for each supported
machine model.

It must never be an accidental consequence of C function-call order.

---

## 15. ULA authenticity contract

### 15.1 The ULA is a timed raster device

The ULA does not fundamentally see a modern framebuffer.

Its canonical state consists of:

```text
canonical master tick
raster position
memory-fetch state
bitmap/attribute pipeline state
border state
FLASH phase
contention/bus interaction
interrupt-generation state
other model-specific ULA state
```

### 15.2 Full raster-time coordinate system

For the 48K PAL model:

```text
224 CPU T-states per line
312 lines per frame
2 ULA pixel clocks per CPU T-state
448 ULA pixel-clock positions per line
```

The complete timing grid is therefore logically:

```text
448 x 312 raster-clock positions
```

For the 128K PAL model the corresponding line width follows its 228-T-state
line timing.

The timing grid is not equivalent to "448 x 312 visible pixels." It contains
active display, border, blanking, and synchronization periods as defined by the
machine model.

### 15.3 Active display

The ordinary bitmap region is:

```text
256 x 192 pixels
```

That is one region inside the full ULA raster timeline.

### 15.4 Border

Border generation is emulated by the ULA/raster model.

An `OUT` changing the border is visible at the hardware-correct raster point.

The renderer must never reconstruct the border from one final border value per
frame.

### 15.5 ULA memory fetches

The ULA fetches bitmap and attribute information according to the real machine
schedule.

A CPU write becomes visible to the ULA according to the relative timing of:

```text
CPU write bus cycle
ULA fetch bus cycle
```

This is essential for multicolor and NIRVANA-class software.

### 15.6 Floating-bus and bus-visible ULA behavior

Where a supported Spectrum model exposes ULA bus activity through floating-bus
or related behavior, the result must derive from the actual timed ULA fetch
model rather than a frame-level approximation.

### 15.7 No program-specific raster hacks

The final architecture must not contain rules such as:

```text
if program is known demo X:
    add compatibility delay Y
```

Historical Warajevo compatibility tables may be retained as regression evidence
during development but are not the target architecture.

---

## 16. Raster output representation

### 16.1 Canonical truth

The canonical video truth is:

```text
master-timed ULA state + raster events
```

A raster buffer is a derived artifact.

### 16.2 Native raster buffer

For debugging, regression testing, and straightforward presentation, the
implementation may materialize one logical sample per native raster-clock
position.

For the 48K timing grid:

```c
uint8_t raster[312][448];
```

would consume 139,776 bytes at one byte per sample.

This is the required initial implementation
because it exposes the complete raster explicitly.

Machine profiles must determine the actual dimensions rather than hard-coding
48K dimensions into generic code.

### 16.3 Raster sample meaning

A raster sample must retain Spectrum-level meaning.

Possible states include:

- logical Spectrum palette index;
- border color index;
- blanking;
- optional diagnostic state in debug builds.

The canonical raster must not store host-specific Metal, D3D, OpenGL, RGB,
BGRA, or texture-format values.

---

## 17. Spectrum color semantics

### 17.1 Bitmap colors

The normal Spectrum display provides:

- 8 INK/PAPER color codes;
- BRIGHT;
- FLASH.

A convenient logical representation is:

```text
0..7   normal palette
8..15  bright palette
```

Normal black and bright black are visually the same, producing 15 distinct
visible active-display colors.

### 17.2 Border colors

The classic border uses the 3-bit border color selection.

BRIGHT does not create an additional bright border palette.

### 17.3 FLASH

FLASH is behavior, not an extra color.

Its phase belongs to emulated machine time. The ULA applies the corresponding
INK/PAPER interpretation.

### 17.4 Host RGB palette

Mapping logical Spectrum colors to host RGB values belongs to presentation.

Different optional display profiles may later emulate:

- nominal digital colors;
- measured display palettes;
- PAL/RF appearance;
- CRT characteristics.

None may alter core ULA state.

---

## 18. Overscan, rainbow, multicolor, NIRVANA and BIFROST requirement

Support for timing-sensitive display techniques is a first-class architectural
requirement, not a later rendering feature.

The core must naturally support software that relies on:

- border changes within a scan line;
- border changes between scan lines;
- writes to attribute RAM immediately before ULA fetches;
- writes to bitmap RAM immediately before ULA fetches;
- precise interrupt entry timing;
- HALT-based synchronization;
- delay loops calibrated in T-states;
- contended memory timing;
- contended I/O timing;
- floating-bus behavior where applicable;
- complete border/overscan raster output;
- deliberate raster racing.

Classes of target software include:

```text
overscan effects
rainbow effects
multicolor effects
NIRVANA
NIRVANA+
BIFROST-family effects
historical Warajevo multicolor/overscan regression cases
```

This list is illustrative, not exhaustive. The architecture must not impose a
known timing or raster-model ceiling below the real supported Spectrum
hardware. Software that relies only on behavior of the emulated hardware
should be representable without adding program-specific exceptions.

The acceptance criterion is not merely that selected demonstrations look
plausible. Their correctness must emerge from the hardware model.

---

## 19. Video presentation through Sokol

### 19.1 Presentation path

```text
ULA / raster core
       |
       v
native logical raster
       |
       v
host-side palette conversion or indexed-texture path
       |
       v
Sokol gfx
       |
       v
native GPU backend
       |
       v
window
```

### 19.2 Sokol role

Sokol may:

- create/manage the application window;
- receive host input events;
- create graphics resources;
- upload the completed native raster or converted host texture;
- draw the raster to the window;
- scale and letterbox;
- present fullscreen;
- support optional presentation shaders.

Sokol may not:

- decide ULA raster position;
- decide which value the ULA fetched;
- determine contention;
- determine border timing;
- determine FLASH phase;
- decide when CPU memory writes become visible;
- drive emulated frame progression.

### 19.3 GPU backend independence

Whether the host presentation path uses Metal, D3D11, OpenGL, Vulkan, or
another Sokol backend must have no effect on core machine state.

### 19.4 Default presentation

The default presentation mode must preserve native aspect and pixel structure
without hiding border timing.

Scaling is a host concern.

### 19.5 Optional CRT and analog effects

Optional shaders may provide:

- scanlines;
- phosphor appearance;
- mask patterns;
- curvature;
- blur;
- PAL/RF-style artifacts.

These operate after canonical raster generation.

A shader failure or shader choice may change appearance, never emulation.


### 19.6 Screenshot capture boundary

WZSN provides one shared screenshot-capture service for GUI and remote-control
front ends.

The capture source is the completed host-visible Spectrum raster produced from
the canonical logical raster **before** menus, toolbars, status overlays,
debugger windows, mouse cursors, or other application chrome are composited.
The selected host-visible crop/border presentation is honored. A screenshot is
therefore a picture of the emulated Spectrum display, not a desktop/window
capture.

The screenshot service must not read back an arbitrary platform GPU framebuffer
when that would make output dependent on backend-specific scaling, shaders,
window occlusion, or desktop composition. It must encode from the stable
presentation raster or an equivalently defined host-side image.

Creating a screenshot:

- does not advance canonical master time;
- does not mutate Spectrum state;
- does not enter deterministic machine-input recordings;
- may use host wall-clock time for host-only filenames and logging;
- is valid while the machine is running or paused.

The GUI destination-selection workflow and the Telnet temporary-file workflow
are defined by `design/warajevo-zx-spectrum-next-ui-architecture.md`; both must
use this shared capture service.

---

## 20. Audio authenticity contract

### 20.1 Core ownership

Warajevo emulates sound.

Sokol delivers sound.

The core owns:

- Spectrum beeper state;
- MIC/speaker-related output state where machine behavior requires it;
- AY-3-8912 state on applicable models;
- AY tone generators;
- AY noise generator;
- AY envelope generator;
- AY mixer/register state;
- event timestamps;
- mixing into canonical internal samples.

### 20.2 Beeper and ULA cassette/audio output state

The core must preserve the machine-model-specific ULA cassette/audio output
state from the relevant port-FE output bits. In particular, implementations
must not prematurely collapse distinct ULA output-bit combinations into one
generic Boolean "beeper" value when the supported hardware exposes
distinguishable output levels or those combinations affect cassette/audio
behavior.

The same underlying timed ULA output state may feed more than one derived
behavior, such as speaker audio and MIC/cassette output. Machine profiles
define the hardware-specific interpretation.

Beeper/output changes are timestamped on the canonical master-tick timeline.

Conceptually:

```text
T=10000   beeper level changes
T=11241   beeper level changes
T=12705   beeper level changes
```

Audio generation integrates these transitions over sample intervals.

Sampling the current beeper bit once per host audio callback is not acceptable.

### 20.3 AY register timing

AY writes are master-timestamped bus events.

The AY chip advances from emulated time and its defined clock relationship to
the Spectrum model.

It does not advance because the host requests more audio.

### 20.4 Canonical mixer

The preferred canonical mixer uses deterministic integer or fixed-point
arithmetic.

Conceptual pipeline:

```text
timed beeper events ---> beeper model ---+
                                         |
timed AY writes -------> AY model -------+--> fixed-point mixer
                                         |
                                         v
                                 canonical PCM
                                         |
                                         v
                              float32 conversion
                                         |
                                         v
                                  Sokol audio
```

Using floating point at the Sokol boundary is acceptable.

Core state and canonical regression audio must not depend on platform-specific
floating-point variation.

### 20.5 Internal sample timeline

The internal PCM rate, or equivalent rational sampling scheme, must be fixed
and documented.

Master-tick-to-sample conversion must use deterministic integer/rational
accumulation.

The host audio device may run at a different physical rate.

---

## 21. Sokol audio contract

The baseline Sokol audio integration **uses the push model**. Pull/callback-driven CPU execution is not an alternative.

Conceptually:

```text
emulation advances
      |
      v
core generates deterministic canonical audio
      |
      v
application applies runtime-speed host-audio policy
      |
      +--> outside 0.5x..2.0x: discard/mute host delivery
      |
      +--> inside 0.5x..2.0x:
               convert/resample as required for selected wall-clock speed
      |
      v
saudio_push(...)
      |
      v
platform audio backend
```

A Sokol audio callback must not become the clock that drives CPU execution.

Audio queue fullness may be used only to prevent host-side overflow/latency.
It must never force the deterministic emulator to abandon the selected runtime
speed or alter machine event ordering.

If the host cannot sustain correct audio delivery at a selected speed within
the enabled 0.5x..2.0x range, the host must degrade audio presentation
explicitly rather than silently change emulation speed.

At speeds outside the enabled range, the host must not accumulate an ever-
growing queue of canonical samples. Core audio hardware state continues
advancing, but host delivery is muted/discarded according to Section 10.10.

When sound is muted, implementations may suppress materialization of PCM
samples that are not required by a regression/recording task, provided beeper,
AY, mixer phase/accumulator state, and every Spectrum-observable audio-related
state advance exactly as if audio presentation were active.

---

## 22. Input architecture

### 22.1 Host input sources

Host-side input sources include:

- Sokol local keyboard events;
- controller state;
- Telnet keyboard commands;
- future approved host-control sources.

These sources do not enter the Spectrum core directly.

### 22.2 Host input arbiter

The application owns an input arbiter that:

1. tracks source ownership independently;
2. maps host inputs to physical Spectrum keys;
3. OR-merges simultaneously held sources;
4. emits only effective matrix transitions;
5. assigns each transition a canonical emulated master tick.

Conceptually:

```text
local keyboard ----+
                   |
                   v
             input arbiter
                   |
Telnet ------------+
                   |
          source ownership / OR
                   |
                   v
      normalized matrix transition
                   |
                   v
        Spectrum keyboard core
```

The Spectrum keyboard core never receives `LOCAL`, `TELNET`, TCP, Sokol key
codes, or any other host-source identity.

### 22.3 Spectrum input

The core models:

- the Spectrum keyboard matrix;
- joystick interfaces;
- other supported emulated input hardware.

The keyboard core receives only physical matrix switch transitions at canonical
master ticks.

### 22.4 Deterministic input timestamping

For deterministic replay, normalized input transitions are recorded at exact
master ticks.

A recorded input trace must be replayable headlessly without Sokol, Telnet, or
any other live host input source.

### 22.5 Focus and UI events

Host focus loss or window-management events must not silently alter arbitrary
Spectrum state.

Any policy such as releasing locally held keys must be implemented in the host
input arbiter and must preserve other sources' ownership.

### 22.6 Initial Kempston joystick contract

Kempston joystick emulation is a REQUIRED initial input device and remains
separate from the Spectrum keyboard matrix. Host keyboard/controller mappings
may drive it, but the core receives normalized joystick state rather than
synthetic Spectrum key presses.

The emulated state contains the five logical controls:

```text
RIGHT
LEFT
DOWN
UP
FIRE
```

Transitions are scheduled on the same canonical master-tick input path used by
other normalized host input. The exact Kempston I/O decode and bit values must
be frozen from hardware/documented evidence before the Phase-5 input ticket is
accepted. Reads must be deterministic and independent of which host device
produced the joystick state.

Automated tests must cover each direction/fire independently, legal
combinations, host remapping, and direct machine-code port reads.

---

## 23. Tape architecture

Tape remains an emulated peripheral/media subsystem, independent of Sokol.

### 23.1 Default loading mode: normal cassette emulation

The default cassette loading mode is **Normal**.

Normal mode means that the Spectrum executes its real loader code against the
emulated cassette waveform/edge stream.

For ROM loading, the normal path is conceptually:

```text
tape image / timed tape source
        |
        v
timed EAR edges
        |
        v
ULA / port input state
        |
        v
Spectrum ROM loader executes on Z80
        |
        +--> real instruction timing
        +--> real interrupt behavior
        +--> real border/loading effects
        +--> real checksum/error behavior
        |
        v
program/data loaded by the emulated Spectrum itself
```

Normal loading is the authenticity baseline and must not depend on a trap or
ROM-loader interception.

This applies to ordinary TAP/TZX-style logical media after conversion to the
appropriate timed pulse/edge representation, and to WAV/audio tape media after
deterministic decoding into the emulator's tape timeline.

### 23.2 Tape timing is expressed in emulated time

Cassette pulses and edges are scheduled on the same authoritative master-tick
timeline used by the Z80, ULA, interrupts, beeper, AY, and peripherals.

A pulse that is 2168 T-states long remains 2168 emulated T-states long
regardless of the selected runtime speed.

The runtime speed control changes only how quickly those T-states elapse in
wall-clock time.

Therefore, for emulator-controlled tape media:

```text
emulator speed      cassette playback wall-clock speed
--------------      -----------------------------------
0.5x                0.5x normal
1.0x                authentic real-time rate
2.0x                2.0x normal
4.0x                4.0x normal
N x                 N x normal
Unlimited           as fast as deterministic execution permits
```

The internal relative timing among CPU execution, EAR edges, MIC output, ULA
raster timing, interrupts, border changes, and audio remains unchanged.

### 23.3 Runtime speed changes during tape playback

Changing emulator speed while a tape is playing must not rescale, rewrite, or
re-encode the tape pulse sequence.

The current tape position remains on the canonical master-tick timeline.

Only future host pacing changes.

For example, changing from 100% to 400% while a pilot tone is in progress means
that the remaining emulated T-states of the current pulse are consumed at the
new wall-clock pace. The pulse itself retains its exact emulated duration.

### 23.4 SAVE / MIC output follows the same rule

Cassette SAVE output and MIC-related output are generated from the same
canonical master-tick timeline.

At 200% runtime speed, a normally generated SAVE waveform is produced twice as
fast in wall-clock time while retaining the exact same T-state pulse structure.

At 50%, it is produced at half wall-clock speed.

### 23.4.1 Cassette-socket ownership in `EAR_MIC` networking mode

The Spectrum has one emulated cassette EAR input and one MIC output. They may
not be simultaneously owned by the ordinary WZSN tape transport and the
Architecture-#3 routed MIC/EAR virtual port.

When networking mode is `EAR_MIC`, Architecture #3 owns the cassette-socket
peer. Ordinary mounted tape media may remain known to the application, but the
normal tape transport must not drive EAR or consume/record MIC while the routed
network owns those signals.

Installation of the distributed Ear+Mic stack while remaining in `EAR_MIC`
fidelity mode uses Architecture #3's explicit `BOOTSTRAP_TAPE` path, in which
the virtual router temporarily emits the standard tape waveform and suppresses
network attention/packet signalling. This is still pulse-level loading through
port FE; it is not RAM injection or ROM-loader trapping.

To use the ordinary WZSN tape transport for unrelated cassette media, the user
must leave `EAR_MIC` mode through the defined cold networking-mode reconfiguration transition.
No implementation may electrically/logically OR tape and routed-network EAR
sources together or feed one MIC waveform concurrently into both semantic
transports.

### 23.5 Optional loading mode: trap-based instant load

Warajevo ZX Spectrum Next may also provide an explicitly selectable
**Instant/Trap Load** mode for supported tape operations.

Instant loading is an acceleration feature. It is not the default and is not
the reference path for tape correctness.

Conceptually:

```text
Normal:
    waveform -> EAR -> real loader code -> loaded result

Instant:
    recognized loader operation -> validated tape block
                                -> accelerated equivalent result
```

The menu/UI must clearly distinguish the two modes.

Suggested semantics:

```text
Cassette Loading
    Normal          [default]
    Instant/Trap
```

The exact menu wording is not frozen, but the default is.

### 23.6 Instant-load equivalence contract

For every supported successful trapped load, the instant path must hand control
back to the emulated Spectrum in the same Spectrum-observable post-load state
that a successful Normal load would have produced from the same initial state
and the same tape data.

This is a canonical machine-state requirement, not merely a loaded-bytes
requirement.

The equivalence target includes, as applicable:

- loaded RAM bytes;
- destination range;
- checksum/result status;
- Z80 register values expected at loader return;
- flags expected at loader return;
- stack state expected at loader return;
- PC/return destination;
- relevant ROM/system variables;
- tape block/file position;
- machine model and paging state;
- loader success/error semantics.

A trap implementation must not merely copy payload bytes and assume that this
is sufficient.

### 23.7 Explicitly transient differences in instant mode

Normal loading has a real elapsed loading interval. During it, the machine may
produce:

- border stripes;
- beeper/MIC transitions;
- intermediate ULA frames;
- intermediate interrupt activity;
- transient CPU register values;
- transient RAM accesses;
- audible tape/loading sound.

A truly instant trap necessarily omits or accelerates some or all of that
transient history.

Therefore equivalence is defined at the **post-load handoff state**, not as an
identical trace of the skipped loading interval.

Any post-load state component that would normally depend on elapsed emulated
time must be reconciled so that the Spectrum-observable handoff state matches
the Normal-load reference. This includes, where relevant:

- canonical master-tick/frame phase;
- ULA raster position;
- ULA FLASH phase;
- interrupt phase;
- peripheral timers;
- tape position;
- paging/device state;
- any system variable or latch changed by the normal loader path.

The trap is free to omit the transient loading history from host presentation,
but it is not free to leave a different Spectrum-visible endpoint merely
because the skipped path would have consumed time.

One valid implementation strategy is to advance the emulated machine timeline
and affected timed devices to the Normal-load handoff point while bypassing the
expensive byte-by-byte loader execution. Another strategy is acceptable only if
it produces the same canonical post-load machine state.

If a trap cannot meet this endpoint-equivalence contract for a loader path, that
path is not eligible for Instant/Trap loading and must use Normal mode.

### 23.8 Preferred acceleration when exact transient equivalence is required

When the user wants faster loading without changing the Spectrum-visible
execution path, the preferred mechanism is to run **Normal** tape emulation at a
higher runtime speed, including Unlimited mode.

That path still executes:

- the real loader code;
- every EAR edge;
- every checksum operation;
- every border change;
- every interrupt;
- every ULA frame.

Only host wall-clock pacing is accelerated.

This is distinct from Instant/Trap Load.

### 23.9 Trap eligibility

Instant loading must be conservative.

A trap may engage only when the emulator can positively identify a supported
loader path and supported media/block semantics.

If eligibility is uncertain, the emulator falls back to Normal loading.

Custom loaders, copy protection, unusual pulse encodings, timing-dependent
software, and unsupported ROM variants must not be silently forced through the
trap path.

### 23.10 Trap failure semantics

If an instant-load attempt cannot preserve the defined result semantics, it
must fail safely or fall back to Normal mode.

It must not leave partially injected data or silently altered machine state.

### 23.11 Live physical cassette input

Live physical cassette input is tied to wall-clock time because the external
device produces edges in real time.

Therefore a 400% emulator speed setting cannot simply make a real cassette deck
play four times faster.

For live external input, the implementation must either:

- synchronize emulated execution to the arriving real-time waveform; or
- capture/buffer the signal first and replay the resulting deterministic tape
  timeline under emulator control.

Live physical cassette capture is explicitly **LATER** in Section 2.5 and is
not part of the first architecture-complete milestone. Its real-time versus
capture/buffer policy must be frozen by an explicit later design change before
implementation tickets are issued.

### 23.12 Initial tape-format support matrix

The initial media contract is:

```text
Format / variant               Read / load   Write / save   Initial requirement
-----------------------------  ------------  -------------  -------------------
Standard Spectrum TAP          REQUIRED      REQUIRED       logical blocks + Normal pulse expansion
Warajevo native TAP            REQUIRED      REQUIRED       signature-detected compatibility path
TZX                            REQUIRED      NOT REQUIRED   read/timing compatibility is mandatory
WAV / sampled audio tape       REQUIRED      NO            deterministic decode into tape timeline
VOC and other legacy audio     LATER         NO            not an initial blocker
```

Before Phase 7 implementation tickets are assigned,
`design/media-format-support.md` must freeze a block/variant-level matrix. For
TZX this means every block type relevant to loading must be explicitly marked
**SUPPORTED**, **IGNORED AS NON-MACHINE METADATA**, or **UNSUPPORTED WITH
CONTROLLED ERROR**. No TZX block may acquire behavior merely because a parser
author happened to implement it first.

For both standard and Warajevo-native `.TAP`, file identification rules, block
semantics, truncation handling, checksum/error behavior, and write format must
be specified in that companion document. For WAV input, accepted PCM/container
forms and deterministic edge-decoding rules must be specified there.

### 23.13 Tape regression requirements

Tape regression must cover both loading modes.

For Normal mode:

- compare edge timing;
- execute real ROM loader paths;
- verify success/error cases;
- verify border/loading behavior;
- verify behavior at multiple runtime speed multipliers.

For Instant mode:

- compare post-load canonical machine state against Normal mode;
- compare loaded data;
- compare loader return state;
- compare tape position;
- validate supported error cases;
- verify unsupported/custom loaders fall back safely.

Normal mode remains the reference result for trap-equivalence testing.

---

## 24. Networking-mode arbitration, Interface 1, Microdrive, and original ZX Net

Interface 1, Microdrive, and original ZX Net remain core emulated devices.
Architecture #3 later adds a separate routed MIC/EAR networking environment.
The two networking environments are mutually exclusive by machine
configuration; they are not independent toggles.

The preserved `MDRIVE.ASM` implementation is a migration reference for:

- register behavior;
- Microdrive state;
- Interface 1 state;
- original Warajevo ZX Net behavior;
- historical network state-machine behavior.

Their new C implementations must be independent of the Sokol layer.

Any separate new MIC/EAR routed networking system is a different feature and
must not be conflated with authentic Interface 1 ZX Net emulation.

### 24.1 Networking-mode arbitration

WZSN has exactly one networking-mode value:

```text
NONE
INTERFACE1
EAR_MIC
```

The implementation must represent this as one enum/state field or an equivalent
single mutually exclusive state. It must not use independently enableable
`interface1` and `ear_mic` booleans that can produce an illegal combined state.

The modes mean:

```text
NONE
    no Interface 1 ROM/device
    no Microdrive
    no original ZX Net
    no Architecture-#3 virtual MIC/EAR router attachment

INTERFACE1
    authentic Interface 1 ROM/device active
    Microdrive available
    original ZX Net available
    Architecture-#3 virtual MIC/EAR router attachment disabled

EAR_MIC
    Interface 1 ROM/device absent
    Microdrive unavailable
    original ZX Net unavailable
    Architecture-#3 virtual MIC/EAR router attachment may be active when that
    downstream feature has been implemented and enabled
```

`EAR_MIC` does **not** page an EAR/MIC networking ROM. Architecture #3 retains
its distributed tape-loaded high-memory software stack and pulse-level port-FE
MIC/EAR behavior. Selecting `EAR_MIC` connects the emulated cassette interface
to that virtual routed-network environment; it does not inject or install the
resident Spectrum software automatically.

Before Architecture #3 is implemented, the `EAR_MIC` state exists as a reserved
downstream mode but must not falsely report the routed network as available.
The companion UI may show it as unavailable with a machine-readable reason
until the Architecture-#3 integration gate has passed.

`EAR_MIC` is additionally available only for a machine profile/variant certified
for Architecture #3's initial physical target: **ZX Spectrum 48K Issue 2**. A
128K machine or a 48K variant whose Issue-2-observable assumptions are not
certified for the routed-network integration cannot enter `EAR_MIC`; the
request fails without mutating the current machine. Architecture #3 may add
later target profiles only through an explicit architecture revision.

Changing networking mode is a **cold machine-reconfiguration transition**, not
an ordinary Spectrum Reset. It is serialized through the
application/orchestrator and destroys/recreates the live machine context using
the same machine model and the newly selected networking mode. Existing RAM,
resident hooks, paged-ROM state, Interface-1 state, and Ear+Mic resident-stack
state do not survive the transition. This is mandatory so a previously loaded
Ear+Mic BASIC hook cannot remain in RAM after `INTERFACE1` becomes active.

The application pause/running state is preserved across the reconfiguration,
but Spectrum RAM state is not. Selecting the already-active networking mode is
idempotent and does not cold-reconfigure the machine.

When leaving `INTERFACE1`, dirty writable Microdrive media must be resolved
before the old machine context is destroyed. A local transition must
successfully flush/commit dirty media through the normal application-controlled
media path or be cancelled; it may not silently discard writes. A remote
`machine.networking.set` request is unavailable while local dirty-media
resolution/confirmation is required. After a successful transition away from
`INTERFACE1`, Microdrive slots are logically detached from the new machine
context and require normal remounting if Interface-1 is selected again.

The transition never hot-adds or hot-removes Interface 1 or the routed MIC/EAR
environment from a running Spectrum.

The initial mode on a fresh application launch is `NONE` unless an explicitly
versioned future configuration policy changes that default. The dynamically
selected Telnet Control Port is unrelated host state and never changes this
machine networking mode.

### 24.2 Device ownership and timing

Interface 1, Microdrive, original ZX Net, and the selected networking-mode
state are deterministic machine configuration/state. Register/latch changes,
cartridge position, network-device state, and all Spectrum-visible timing are
driven by canonical emulated time. Host file or network timing may not directly
mutate these devices.

### 24.3 Microdrive contract

The Microdrive implementation must support, at minimum:

- insertion/ejection of an MDR cartridge image;
- deterministic cartridge position and motor state;
- deterministic sector/header/data visibility;
- write-protect and write behavior where represented by the media format;
- explicit malformed/truncated image errors;
- save/flush through application-controlled media operations rather than hidden
  writes from arbitrary core code;
- canonical state hashing sufficient to reproduce a run at the same master tick.

### 24.4 Interface 1 contract

The Interface 1 implementation owns its ROM-facing paging/latch/device behavior
and exposes Microdrive, serial/network, and related machine-visible behavior
through the machine bus. It must not expose host file handles, sockets, or host
printer objects to the core. Old/new Interface 1 ROM behavioral differences
used by compatibility software must be represented explicitly when evidence
requires them.

### 24.5 Original ZX Net contract

Original ZX Net emulation must preserve the Spectrum-visible state machine and
timing independently of any host transport used to connect emulator instances.
A deterministic single-process/loopback harness must be sufficient to test the
core network state machine without relying on host packet timing.

If a later multi-instance host transport is added, received host data is
normalized and scheduled through the application/orchestrator before it affects
emulated ZX Net state.

### 24.6 Phase-10 specification gate

Before Phase 10 implementation tickets are assigned,
`design/interface1-microdrive-zxnet.md` must freeze:

- I/O port decode and paging rules;
- ROM identity requirements;
- device registers/latches and reset values;
- Microdrive bit/byte/sector timing and MDR interpretation;
- old/new Interface 1 ROM compatibility cases;
- original ZX Net state transitions and timing;
- networking-mode arbitration/cold-reconfiguration behavior for `NONE` and `INTERFACE1`, with
  the reserved downstream `EAR_MIC` mode kept mutually exclusive;
- canonical serialization/hash fields;
- differential tests against preserved Warajevo behavior;
- hardware/reference tests used when Warajevo and hardware differ.

No unresolved item in that companion specification may be decided implicitly
inside a Phase-10 implementation ticket.

---

## 25. Snapshot architecture

Snapshot loading and saving must be portable across supported host platforms.

Requirements include:

- explicit file-format parsing;
- explicit endianness;
- bounds checking;
- no raw structure casts;
- deterministic machine restoration;
- validation of unsupported or malformed variants;
- model selection based on defined snapshot semantics;
- regression comparison against original Warajevo behavior where applicable.

Internally generated Warajevo ZX Spectrum Next save states must be versioned and
portable between x86-64 and AArch64 builds. The native internal state format
must record `networking_mode` and the active deterministic peripheral state
needed to restore that configuration. Historical SNA/Z80 behavior regarding
external Interface-1 state or an active `EAR_MIC` environment must be frozen in
`design/media-format-support.md`; an import/export path may not silently create
simultaneous Interface-1 and Ear+Mic state or invent a networking ROM.

### 25.1 Initial snapshot-format matrix

```text
Format / variant                 Load          Save          Initial requirement
-------------------------------  ------------  ------------  -------------------
SNA 48K                          REQUIRED      REQUIRED      exact 48K state restore
SNA 128K                         REQUIRED      REQUIRED      exact paging/state restore
Z80 v1-class                     REQUIRED      NO            legacy input compatibility
Z80 v2-class                     REQUIRED      REQUIRED      canonical initial Z80 save form
Z80 v3-class                     REQUIRED      NO            later-input compatibility
Warajevo Timex Z80 extension     LATER         LATER         with TS2068 profile
Other historical snapshots       LATER         LATER/NO      explicit future disposition
```

The canonical initial `.Z80` writer targets the v2-class representation used by
the historical Warajevo save path where the selected machine state is
representable. If a state cannot be represented without loss, the application
must either select another explicitly supported save format or report a
controlled error; it must not silently discard machine state.

### 25.2 Atomic load contract

Snapshot parsing occurs into validated temporary state. The live machine state
is replaced only after the entire snapshot has passed structural, range, model,
and semantic validation. A failed load leaves the pre-load machine state
unchanged.

Snapshot auto-detection may use extension as a hint but must validate the actual
file structure before committing state.

### 25.3 Phase-8 specification gate

Before Phase 8 implementation tickets are assigned,
`design/media-format-support.md` must freeze the byte-level variant matrix for
SNA and Z80, including compression, paging, PC encoding, hardware-mode fields,
unsupported variants, malformed-input behavior, and exact canonical save form.

---

## 26. Application/orchestrator and host services

Sokol is the selected initial presentation/input/audio implementation, but the
deterministic core does not call Sokol or any generic host API.

Application/orchestration code owns both sides:

```text
deterministic core
    consumes:
        normalized timed input
        explicit media/control requests

    produces:
        canonical raster
        canonical audio
        state/trace results

application/orchestrator
    owns/maps:
        shared application command registry and serialized command dispatch
        GUI/Telnet/test command front ends -> one semantic command handler path
        host inputs -> normalized timed core inputs
        core raster -> screenshot service and Sokol presentation
        core audio  -> runtime-speed audio policy -> Sokol
        host clock  -> pacing decisions only
```

Host-facing project interfaces may exist for code organization, but they are
not dependencies of the Spectrum core.

A headless orchestrator/test harness is required for automated testing and
deterministic replay.

---

## 27. Sokol integration contract

### 27.1 Sokol is compiled into the program

The project vendors a pinned Sokol revision.

Sokol implementation code is compiled as part of the Warajevo target rather
than distributed as a Warajevo-supplied shared library.

Typical integration is one controlled implementation translation unit
containing the required Sokol implementation defines and headers.

### 27.2 Required Sokol surface

The baseline host adapter is expected to use:

```text
sokol_app.h
sokol_gfx.h
sokol_audio.h
sokol_time.h
sokol_glue.h where appropriate
sokol_log.h optionally
```

`sokol_framebuffer.h` may be used as a convenience layer if it simplifies
presentation without leaking presentation details into the core.

Other Sokol headers must not be pulled in without a concrete requirement.

### 27.3 Pinned revision

The exact Sokol commit or release used by a Warajevo release is recorded in the
build manifest.

Canonical builds must not silently use "whatever Sokol happens to be installed
on the host."

### 27.4 Platform backend selection

Backend selection is compile-time per release target.

Initial intended desktop paths are:

```text
Windows:
    Sokol app
    D3D11 graphics
    WASAPI audio

macOS:
    Sokol app
    Metal graphics
    CoreAudio audio

Linux:
    Sokol app
    X11-compatible desktop windowing in the current baseline
    graphics backend selected by the pinned Sokol/build profile
    ALSA audio in the baseline Sokol audio path
```

The initial Linux host contract is therefore an X11-compatible desktop path.
Native Wayland is not an initial requirement. It may be added later if the
selected/pinned Sokol revision and project requirements justify it. The exact
Linux graphics backend is frozen only after the pinned Sokol revision and
target distribution matrix are tested.

### 27.5 macOS compilation

The Sokol implementation translation unit may be compiled as Objective-C on
macOS as required by the platform integration.

The C core remains unaware of Objective-C.

### 27.6 Sokol upgrades

A Sokol upgrade requires:

- successful build on every supported target;
- deterministic core tests;
- presentation tests;
- audio delivery tests;
- input tests;
- package dependency inspection.

A host-layer upgrade must not change canonical core hashes.

---

## 28. Single-binary distribution contract

### 28.1 Meaning of "single binary"

For Warajevo ZX Spectrum Next, the requirement means:

> Each supported target is distributed with one Warajevo program executable
> containing the Warajevo code and required third-party application code such
> as Sokol. The release does not require a project-supplied Sokol DLL, audio
> DLL, graphics DLL, or similar companion multimedia runtime library.

Normal operating-system libraries, drivers, and frameworks are permitted.

### 28.2 Not a promise to statically embed the operating system

The requirement does not mean that the executable contains:

- Windows system DLLs;
- Apple system frameworks;
- Linux kernel interfaces;
- GPU drivers;
- system graphics loaders;
- X11/Wayland system components;
- system audio services;
- libc in every target configuration.

If a future requirement becomes "zero dynamic system-library dependencies,"
that is a different packaging constraint and may require a different Linux host
strategy.

### 28.3 Windows target

Target shape:

```text
Warajevo-ZX-Spectrum-Next.exe
```

Sokol and Warajevo code are linked into the executable.

Windows-provided APIs such as the windowing, D3D, and audio systems remain
system dependencies.

### 28.4 macOS target

The program binary contains Warajevo and Sokol code.

For normal GUI distribution it may live inside a signed/notarized:

```text
Warajevo ZX Spectrum Next.app
```

The `.app` is the platform distribution container. The executable itself must
not depend on project-shipped third-party `.dylib` files.

Apple system frameworks remain system dependencies.

### 28.5 Linux x86-64 and ARM64 targets

The target is one Warajevo ELF executable with Sokol compiled into it.

System windowing, graphics, libc, and audio libraries may remain dynamic system
dependencies.

The release must not require a Warajevo-supplied third-party `.so` beside the
program.

### 28.6 Configuration and generated data

"Single binary distribution" does not prohibit the executable from creating
normal user configuration, cache, screenshots, snapshots, or media files after
launch.

Default configuration should be compiled into the executable where practical.

### 28.7 Multi-instance host-data safety

Multiple WZSN processes may run concurrently. Shared host data must therefore
remain valid under interprocess access.

At minimum:

- preference/configuration writes use interprocess serialization and atomic
  replacement so one process cannot truncate or partially overwrite another's
  configuration file;
- the dynamically selected Control Port is never persisted as a preference;
- a writable media image may not be exposed to two WZSN processes as independent
  writers at the same time; the application must obtain an exclusive write
  claim based on resolved host file identity where practical, not merely a
  user-supplied path string, or reject the second writable mount; explicit
  read-only fallback is allowed only when that workflow is deliberately
  selected;
- every WZSN-managed persistent output write, including snapshots, converted
  media, exported reports, and other generated files, must either obtain an
  interprocess exclusive claim for the destination or use an atomic
  write-to-new-file/replace protocol that cannot expose a partial file; two
  concurrent writers to the same logical destination may serialize or one may
  fail cleanly, but they may not interleave/truncate each other's output;
- Telnet temporary screenshots use atomic exclusive file creation before any
  collision suffix retry, so two processes cannot both pass a non-atomic
  existence check and overwrite one another;
- host-only temp/output files created by background conversion or media work use
  unique or exclusively created names and atomic commit where replacement is
  promised.

These are host-safety rules. They do not become Spectrum state and do not alter
deterministic machine timing.

---

## 29. ROM and firmware distribution constraint

The single-binary technical architecture does not itself grant redistribution
rights for Sinclair ROMs, Interface 1 ROMs, or other firmware.

Therefore:

- ROM bytes may be embedded only when redistribution rights have been
  established for the intended release;
- otherwise a release may need an external user-supplied ROM;
- requiring an external ROM is a legal/distribution exception to the
  single-file objective, not an emulator architecture failure;
- ROM identity must be hashable and recorded in deterministic tests.

The build system must keep ROM licensing status separate from emulator source
licensing.

---

## 30. UI and application-command authority boundary

The detailed WZSN user-interface architecture is intentionally separated from
this core/system document.

The canonical UI authority is:

```text
design/warajevo-zx-spectrum-next-ui-architecture.md
```

That companion document owns:

- the top-level GUI menu tree and toolbar;
- the Machine/Media status presentation;
- media-manager user workflows;
- debugger/monitor presentation;
- settings and compatibility-tool presentation;
- the shared application command registry and stable command IDs;
- GUI, toolbar, Telnet, automated-test, and future CLI projections of that
  registry;
- Telnet application-control commands such as `RESET`, `SPEED`, `SCREENSHOT`,
  `MENU`, `DESCRIBE`, and `DO`;
- remote-command permission classes and UI-visible remote-control policy;
- screenshot naming/destination/reporting behavior;
- UI workflow, cancellation, accessibility, error, and acceptance contracts.

This document continues to own the machine semantics beneath those controls.
In particular:

- machine reset semantics belong to the machine/core architecture;
- runtime speed semantics belong to Sections 10, 20-21, 23, and 32;
- local/Telnet keyboard normalization belongs to Section 22 and Section 55;
- screenshot raster provenance belongs to Section 19.6;
- Telnet socket/framing, connection, threading, and keyboard-injection behavior
  belong to Section 55;
- the UI and Telnet command registry may request machine operations but may not
  redefine their effects.

The UI is not part of the deterministic emulation core. No GUI toolkit,
application menu, file chooser, command registry, Telnet control command, or
host screenshot operation may become a source of canonical Spectrum time.

The exact C UI toolkit remains a Phase-12 freeze-gate decision. Whatever toolkit
is selected must be statically incorporable into the application binary, remain
outside the emulation core, and remain optional to headless core tests.

Phase 12 may not exit until the UI companion document's initial-workflow and
acceptance contracts pass on disk-built application binaries without core-state
divergence.

---

## 31. Core threading model

The baseline core is single-threaded and deterministic.

One emulation thread owns mutable machine state.

Sokol or the operating system may use internal threads for presentation or
audio delivery, but those threads do not mutate Spectrum state directly.

Cross-thread communication, if required, uses bounded queues containing:

- completed presentation data;
- completed audio data;
- host input events;
- host control requests.

Thread scheduling must not affect canonical machine results.

---

## 32. Pacing and audio buffering

The emulation scheduler determines what machine work happens.

The host determines when the already-determined work is presented.

A normal run loop is conceptually:

```text
collect host input
      |
convert to scheduled emulated input events
      |
run deterministic core to next scheduling boundary
      |
produce raster/audio blocks
      |
submit presentation/audio to host
      |
compare emulated progress to host monotonic time
      |
sleep/yield when ahead
```

Audio queue fullness may be observed to avoid host-side overflow or excessive
latency, but it may not alter emulated event ordering, hardware timing, or the
user-selected runtime speed. Section 10.10 defines when host sound is muted.

---

## 33. Build architecture

The build system must produce a target-specific executable from:

```text
portable Warajevo C core
+
target host adapter
+
pinned Sokol headers/implementation
+
normal system SDKs
```

The project build generator is **CMake**. The root `CMakeLists.txt` is the
canonical build entry point. Platform toolchain details may live under
`cmake/toolchains/`, but platform build files must not create a second
independent source of project architecture.

The build must expose at least these logical targets:

```text
wz_core                 deterministic portable C11 core
wz_headless             headless orchestrator/test executable
wz_tests                unit/regression test aggregate
warajevo-zx-spectrum-next GUI distribution application
```

Sanitizer-enabled and high-warning developer configurations must be supported
where the selected compiler provides them.

Every build records:

- Warajevo source commit;
- compiler identity and version;
- target OS;
- target CPU;
- selected C standard;
- optimization flags;
- sanitizer/debug flags where relevant;
- Sokol commit/release;
- graphics backend;
- audio backend;
- system SDK version where available.

No runtime language environment is required by the architecture.

---

## 34. Initial supported platform matrix

The initial architecture targets:

```text
Target                    CPU       Primary host intent
------------------------  --------  -----------------------------------
Windows                   x86-64    Sokol + D3D11 + WASAPI
Linux desktop             x86-64    Sokol + X11 path + system graphics + ALSA
Linux desktop             AArch64   Sokol + X11 path + system graphics + ALSA
macOS                     AArch64   Sokol + Metal + CoreAudio
macOS                     x86-64    Secondary while worth maintaining
```

A platform is not "supported" because it merely compiles.

Support requires:

- successful release build;
- successful deterministic core test suite;
- raster tests;
- audio tests;
- input tests;
- media tests;
- package dependency inspection;
- practical launch/run testing on the real host architecture.

Additional platforms such as Windows ARM64, Android, iOS, and WebAssembly may
be considered later. They are not initial commitments.

---

## 35. Cross-compiler matrix

Pure C gives the project a valuable correctness test: compile the same core
with materially different compilers.

The required CI/compiler matrix for the initial supported targets is:

```text
Windows x86-64:
    MSVC
    Clang/clang-cl

Linux x86-64:
    GCC
    Clang

Linux AArch64:
    GCC
    Clang

macOS AArch64:
    Apple Clang

macOS x86-64:
    Apple Clang while that secondary target is maintained
```

The first four target groups above are mandatory for the architecture-complete
milestone. macOS x86-64 is secondary and is required only while the project
continues to advertise it as a maintained target. If a hosted CI service cannot
provide a required CPU architecture, the corresponding release gate must run on
project-controlled or self-hosted hardware; lack of a convenient runner is not
a reason to omit the test.

Compiler disagreement in deterministic core state is investigated as a defect,
with C undefined/implementation-defined behavior among the first suspects.

---

## 36. Deterministic regression protocol

Each deterministic test specifies:

```text
machine model
ROM hash
initial RAM/state
media hashes
emulation configuration
input-event trace
checkpoint master ticks
```

At each master-tick checkpoint, canonical hashes are generated for relevant state:

```text
CPU state
RAM/pages
paging state
ULA state
bus state
interrupt state
tape state
Interface 1 state
Microdrive state
ZX Net state
beeper state
AY state
canonical raster or raster-event state
canonical audio state
```

The hashes must match across supported hosts and compilers.

Host state is excluded.

### 36.1 Mandatory complete Fuse Z80 test suite

The **complete Fuse Z80 unit-test suite** is a mandatory CPU-conformance corpus
for Warajevo ZX Spectrum Next. The exact upstream Fuse revision used for a
release must be pinned and recorded with the test artifacts.

All applicable Fuse Z80 tests must pass for the certified Z80 core. A failing
or skipped case may not be waived merely because ordinary Spectrum software
appears to work. Any intentionally inapplicable case requires a documented,
reviewed reason tied to the test's assumptions rather than to convenience.

The Fuse suite is complementary to, not a replacement for:

- project-owned instruction-level tests;
- timing/bus-cycle tests;
- contention and interrupt tests;
- real-hardware validation;
- full-machine compatibility artifacts.

Upstream project reference:

```text
https://sourceforge.net/projects/fuse-emulator/
```

### 36.2 Private difficult-media regression directory

Difficult real-world media used only for development compatibility testing lives
under the project root in a dedicated directory whose private contents are
ignored by Git and excluded from every source or binary distribution. The
architecture names one optional private development location:

```text
WZSN-PRIVATE-TEST-MEDIA/
```

The directory is deliberately flat and unmanaged by the architecture. Developers
may place any useful difficult TAP, TZX, SNA, Z80, MDR, program, or other test
artifact directly in it. The architecture imposes **no** subdirectory structure,
file naming convention, COMPAT identifier, manifest, hash, provenance record,
copyright/license record, source URL, media-type grouping, or mandatory per-file
description.

The directory path may be supplied by the local development/test environment,
for example through:

```text
WZSN_PRIVATE_TEST_MEDIA=./WZSN-PRIVATE-TEST-MEDIA
```

No developer-specific absolute path is committed to the public repository. The
directory name and its public guidance files may be committed, but the private
media files inside it are never committed, packaged, copied into release
artifacts, or required to be redistributable. Public repository validation and
metadata-only automation must work when the private media files are absent;
private difficult-media tests are then reported as unavailable/skipped rather
than as product failures.

When the private directory is present, private test code may use its files in any
way useful to development. The architecture does not require a manifest or a
one-test-per-file mapping, and it does not require the harness to recurse into
subdirectories. Unrecognized files may be ignored.

A first architecture-complete release may not claim full difficult-media or
hardware-hack compatibility until the project's private development regression
run has completed with no unexplained failures. This is a release-validation
requirement; it does not turn the private media into a public project artifact.

---

## 37. Video regression protocol

Canonical video tests compare emulation output before Sokol presentation.

Depending on the test, compare:

- full logical raster;
- visible active 256 x 192 image;
- border timeline;
- blanking map;
- ULA fetch/event trace;
- raster hash;
- exact pixel/color transitions around critical master ticks.

Host screenshots after GPU scaling or CRT shaders are presentation tests, not
core video correctness tests.

Timing-sensitive regression cases must include deliberately difficult raster
software, not only ordinary games.

---

## 38. Audio regression protocol

Canonical audio tests compare data before Sokol conversion/output.

Tests may compare:

- timestamped beeper transitions;
- timestamped AY register writes;
- AY internal state;
- fixed-point mixer output;
- canonical PCM hash;
- sample-boundary accumulator state.

Audio captured from WASAPI, CoreAudio, or ALSA is a host-delivery test and must
not replace deterministic core audio tests.

---

## 39. Differential testing against Warajevo 2.50

Original Warajevo remains valuable as a differential reference.

The preserved 2.50 source is the source-level authority. Where executable
2.51/2.52 behavior, official documentation, or official sample media expose a
behavior absent from the surviving source, those later artifacts may be used
as behavioral evidence only; they do not become imaginary source code.

For compatible scenarios, compare:

- Z80 registers;
- memory;
- snapshots;
- tape state;
- Microdrive state;
- Interface 1 state;
- ordinary screen output;
- debugger/monitor semantics;
- user-visible errors and workflows.

A mismatch is investigated, not automatically classified as a new-emulator
bug.

If the mismatch is caused by a known or proven Warajevo 2.50 timing
approximation and the new result agrees with real hardware, the new hardware
behavior wins and the difference is documented.

---

## 40. Real-hardware validation

Timing-sensitive behavior requires real-hardware or independently established
hardware-reference validation.

Before the first architecture-complete milestone, certification validation must
cover:

- frame timing;
- line timing;
- interrupt timing;
- contention;
- floating bus;
- port FE timing;
- border transition timing;
- bitmap/attribute fetch timing;
- tape EAR/MIC behavior;
- model differences;
- AY clock/register behavior;
- Interface 1 timing where required.

Measurements and test ROMs used to establish behavior must be preserved as
project artifacts where licensing permits.

---

## 41. Migration method for original Pascal and assembly

Migration is performed routine by routine. The canonical migration inventory is
`design/migration-ledger.md`.

For every original source routine or logically inseparable block, create a
migration record containing:

```text
upstream repository + commit/blob identity
source file
source routine/label
original purpose
original copyright holder / notice
license classification
derivation clearance: TRANSLATE / REIMPLEMENT / RESEARCH / EXCLUDE
inputs
outputs
emulated state read
emulated state written
host state read/written
master-timing consequences
external calls
error behavior
new C destination
regression test
provenance comment/reference in new code
migration status
```

Then:

```text
read original routine completely
        |
identify emulated behavior
        |
identify DOS/x86/Turbo-Pascal host mechanism
        |
separate the two
        |
implement behavior in the appropriate C subsystem
        |
write focused tests
        |
compare with original Warajevo
        |
compare with hardware behavior when timing-sensitive
        |
mark migration record complete
```

No source file is considered fully migrated until every relevant routine is
accounted for. No routine may be translated merely because it exists in the
archive: `derivation clearance` must be resolved first. Third-party or unclear
legacy material defaults to **REIMPLEMENT** or **RESEARCH**, not TRANSLATE.

---

## 42. Historical host mechanism replacement map

```text
Historical mechanism          Warajevo ZX Spectrum Next
----------------------------  --------------------------------------------
Turbo Pascal application      portable C application logic
Turbo Vision UI               new host/application UI
x86 Z80 register mapping      explicit portable Z80 state
x86 opcode handlers           portable C Z80 handlers/micro-operations
DOS segmented memory          explicit flat C data structures
BIOS keyboard                 Sokol host input -> Spectrum matrix
CGA/EGA/VGA/Hercules output   ULA raster -> Sokol gfx
PC speaker                    emulated beeper -> canonical mixer
SoundBlaster                  canonical PCM -> Sokol audio
DOS PIT/timer                 Sokol/host monotonic time for pacing only
DOS file I/O                  portable C file/media layer
LPT host output               host abstraction where retained
CLI/STI host critical code    modern synchronization where actually needed
Warajevo raster hacks         accurate bus/ULA timing model
```

A historical mechanism is retained only when it represents actual Spectrum
behavior rather than the DOS host.

---

## 43. Performance policy

Performance is not allowed to weaken correctness.

Permitted optimizations include:

- event batching when no externally observable event occurs;
- precomputed flag tables whose results are exhaustively verified;
- precomputed contention tables derived from the authoritative timing model;
- generated opcode decode tables;
- contiguous memory layouts;
- presentation-frame dropping;
- SIMD in host-only color conversion after canonical raster generation.

Forbidden optimizations include:

- delaying memory writes until instruction end;
- collapsing multiple timed I/O writes into one final value;
- rendering only the final border color;
- skipping ULA fetches in a way that breaks floating-bus behavior;
- using host audio callback timing as AY timing;
- host-specific arithmetic that changes flags or overflow behavior.

Every core optimization requires regression evidence.

---

## 44. Debugging and observability

Accuracy work requires first-class deterministic diagnostics.

Debug builds should be able to trace:

```text
global master tick
same-tick event sequence/order
derived CPU T-state/phase
CPU instruction boundary, PC, opcode/prefix bytes, and register snapshot
CPU M-cycle / externally visible bus phase
memory read/write address and value
I/O read/write address and value
contention request and inserted delay
interrupt/NMI edge, sample, acceptance, and acknowledge
ULA fetch address/value and fetch kind
raster coordinate / frame position
floating-bus source/value
border change
beeper change
AY register write
tape EAR/MIC edge
Interface 1/Microdrive/ZX Net event
networking-mode/cold-reconfiguration marker
explicit developer/test trace marker
```

Trace generation must be optional because it is expensive.

Trace timestamps are canonical emulated master ticks, not host timestamps.

A trace produced from the same deterministic test must be semantically
identical across hosts.

### 44.1 Per-instance circular timing trace file

A binary circular timing trace file is required **before Phase-2 CPU/timing
implementation begins** so timing failures can be debugged from the first
instruction-level work onward.

Each running developer/debug WZSN process owns one independent trace file. The
file is host diagnostic state and is never part of canonical Spectrum state.
The initial file contract is:

```text
maximum total file size: 16 MiB = 16,777,216 bytes, including file header
organization:            fixed header + circular binary record area
writer ownership:        exactly one WZSN process
record ordering:         monotonically increasing trace-event sequence
machine timing:          canonical master tick + same-tick event order
wrap behavior:           overwrite oldest complete records only
release behavior:        developer/debug diagnostic; not required to be enabled by default
```

The trace backend must preallocate or otherwise enforce the 16 MiB maximum. It
must never grow beyond that limit. A wrap may not leave a record that can be
misread as complete; the format therefore requires explicit record lengths and
a recognizable wrap/generation boundary.

The binary format is versioned and self-describing enough for a standalone
trace-dump tool to determine at minimum:

```text
format version
record-area bounds
current write position / wrap generation
process/session identity sufficient to distinguish concurrent WZSN instances
machine profile and ROM identity at trace start
trace feature/event mask
first/last recoverable record sequence
```

Host wall-clock time, PID, random/session nonce, and path information may appear
in the **file header or filename** for developer convenience, but never as event
timestamps and never in canonical machine hashes. File creation uses an
exclusive/no-clobber name such as `wzsn-trace-<pid>-<session>.wztrace`; concurrent
instances must never share one trace file. The selected path is reported to the
developer through startup diagnostics and may later be surfaced by debugger
tools without becoming machine state.

### 44.2 Timing-full event depth

The `TIMING_FULL` trace level is the correctness/debugging baseline. It must be
deep enough to reconstruct the causal ordering of timing failures rather than
merely report final state. In particular it records instruction boundaries and
register snapshots together with every externally visible CPU bus operation,
contention delay, interrupt decision, ULA fetch, floating-bus source, raster/
border transition, and relevant peripheral edge listed above.

Records use compact binary encoding, including delta-master-tick encoding where
appropriate, so the fixed 16 MiB ring retains a useful pre-failure window. Before
Phase 2 exits, measurement on the 48K PAL timing-torture trace must demonstrate
that `TIMING_FULL` retains **at least eight complete Spectrum frames** immediately
preceding the current write position. If the initial encoding fails that floor,
the encoding must be compacted; increasing the 16 MiB file limit is not the
default remedy.

A periodic absolute master-tick/instruction-state synchronization record is
required so a valid portion of a wrapped trace can be decoded without needing
records that have already been overwritten.

### 44.3 Trace isolation, freeze, and post-mortem behavior

Trace instrumentation is observational only. Enabling, disabling, wrapping,
flushing, or failing the trace file may change host wall-clock performance but
may not change:

```text
master-tick progression or event ordering
CPU/bus/ULA results
contention
interrupt acceptance
canonical state hashes
raster/audio/tape/peripheral semantics
```

Core code emits structured trace events through a narrow diagnostic sink; it
does not perform host file I/O directly. The circular-file backend lives outside
the deterministic core boundary.

The trace service provides a **freeze** operation. Test mismatches, debugger
assertions, and controlled fatal-error paths may freeze the ring before further
records overwrite the pre-failure history. Fatal/crash handling performs only a
best-effort cursor/header flush that is safe for the host failure context; the
architecture does not require unsafe complex recovery work inside a signal or
exception handler. A standalone reader must recover every complete record that
was durably written before an abrupt process failure and must reject incomplete
trailing records cleanly.

Trace-file creation or write failure is visibly reported in a developer build
but does not mutate or terminate the emulated machine merely because diagnostic
storage is unavailable.

---

## 45. Headless core

The deterministic emulator core must build and run without Sokol.

This is required for:

- automated unit tests;
- CI;
- fuzzing;
- differential CPU tests;
- snapshot tests;
- media parser tests;
- deterministic replay;
- state hashing;
- timing traces.

The distribution application uses Sokol. The test architecture does not depend
on a window, GPU, keyboard, or audio device.

This is also the strongest proof that Sokol has not leaked into the machine
model.

---

## 46. Failure and error handling

Malformed external data must not compromise host process memory.

Media and snapshot parsers require:

- length checks;
- integer-overflow checks;
- allocation checks;
- format validation;
- explicit unsupported-format errors;
- deterministic failure state.

Core assertions are appropriate for internal invariants in development builds.
External malformed input must produce controlled errors rather than rely on
assertions.

Host errors such as failure to create a GPU device or audio device are
application failures, not Spectrum state.

---

## 47. Dependency policy

The default runtime dependency policy is intentionally narrow.

Required application-level third-party code:

```text
Sokol
```

Special Feature 1 uses native operating-system socket APIs through project-owned
C wrappers and therefore introduces no additional third-party runtime
dependency.

Additional third-party code requires explicit architectural approval.

A proposed dependency must justify:

- why the standard library or existing code is insufficient;
- static incorporation into release binaries;
- supported-platform coverage;
- license compatibility;
- maintenance burden;
- deterministic-core isolation.

Dependency convenience is not by itself sufficient.

---

## 48. Version-pinning and provenance

For every third-party source dependency:

- pin an exact revision or release;
- preserve license information;
- record source origin;
- record local modifications;
- never silently track a moving branch in release builds.

For every original Warajevo routine migrated:

- record the upstream repository, commit and source/blob identity;
- record the original copyright/notice and license classification;
- record whether the new work is a translation/derivative, independent
  reimplementation, research-only reference, or excluded material;
- obtain explicit migration clearance before implementation begins;
- retain source provenance in migration documentation;
- retain enough mapping information to audit the C implementation back to the
  preserved source;
- preserve required attribution/license notices in the new source and release
  materials.

The top-level `LICENSE.txt` and `NOTICE.md` are part of the repository
architecture and must remain consistent with migration records and bundled
third-party material.

---

## 49. Initial implementation sequence

The implementation order is:

```text
Phase 0
    freeze `design/machine-timing-evidence.md` and source authority
    freeze 48K profile master-clock/raster/interrupt/contention/floating-bus data
    freeze same-edge event-order table
    freeze ROM hashes used by certification tests
    integrate the complete Fuse Z80 test suite into the test plan
    freeze early timing-smoke cases and canonical trace/hash schema
    freeze the versioned 16 MiB circular timing-trace file/record contract
    create `design/migration-ledger.md` with licensing/provenance clearance fields

Phase 1
    portable C type/state foundation
    canonical master-tick timeline
    machine clock/profile relationships
    canonical serialization/hash framework
    implement per-process circular trace backend and freeze/post-mortem reader
    wire observational structured trace hooks before Phase-2 CPU work begins

Phase 2
    Z80 decoder and exact bus-cycle-capable execution
    T-state/phase derived from master tick

Phase 3
    memory bus, I/O bus, contention, interrupt scheduling

Phase 4
    48K ULA fetch/raster model
    full native raster
    border and floating-bus behavior

Phase 4A - mandatory early timing validation gate
    run timing-smoke corpus before host/UI work expands
    instruction/bus-cycle timing
    interrupt entry/edge timing
    contention
    border transition timing
    ULA fetch versus CPU write ordering
    floating bus
    at least one early overscan/raster-racing case
    at least one early multicolor timing case

Phase 5
    Sokol host shell
    raster presentation
    keyboard mapping
    host input arbiter
    Kempston joystick emulation and host mapping
    runtime speed control
    pacing

Phase 6
    beeper core
    canonical audio mixer
    runtime-speed audio policy
    Sokol audio push delivery

Phase 7
    tape subsystem
    Normal cassette loading as default
    runtime-speed-coupled tape playback/SAVE
    optional Instant/Trap loading with post-load equivalence tests

Phase 8
    snapshot subsystem

Phase 9
    128K machine profile
    paging
    128K ULA timing
    AY-3-8912

Phase 10
    networking-mode arbitration for NONE / INTERFACE1
    reserved downstream EAR_MIC mode with no false availability before Architecture #3
    Microdrive
    Interface 1
    original ZX Net

Phase 11
    monitor/debugger
    ZX Printer
    any other REQUIRED legacy item explicitly listed in Section 2.5 and not
    already completed in an earlier phase

Phase 12
    implement `design/warajevo-zx-spectrum-next-ui-architecture.md`
    shared application command registry
    GUI menus/toolbar/status and media-manager workflows
    GUI/Telnet/test command-path equivalence

Phase 13
    full cross-platform binary packaging

Phase 14
    timing torture suite:
        overscan
        rainbow
        multicolor
        NIRVANA/NIRVANA+
        BIFROST-class
        historical Warajevo problem cases

Phase 15
    Special Feature 1:
        single-client Telnet keyboard/control server per WZSN process
        first-free Control Port probe across 30740-32787 / wildcard bind
        multi-process exclusive bind/race tests
        local + remote source ownership
        keyboard-matrix equivalence tests
        application-control projection through the shared command registry
        UI-architecture Telnet control/security acceptance contract

Phase 16
    optimization only after correctness baselines are locked
```

This phase order may be changed only by an explicit architecture update. Exact
bus/raster timing remains foundational and may not be postponed behind host/UI
work.

### 49.1 Mandatory phase exit gates

A later phase may not treat an earlier subsystem as dependable merely because
it compiles. The following are minimum exit gates:

```text
Phase    Minimum exit gate
-------  -----------------------------------------------------------------------
0        timing evidence files frozen; ROM hashes recorded; source/provenance
         rules frozen; test strategy and early smoke corpus review-approved
1        canonical state/serialization/hash round-trips; C11 UB/sanitizer smoke
         tests pass on at least two materially different compilers; per-process
         16 MiB circular trace file, wrap/freeze/recovery, and trace-on/off
         equivalence tests pass before Phase-2 implementation begins
2        complete Fuse Z80 suite passes; project opcode/flag/interrupt tests pass;
         exact bus-cycle trace capability demonstrated; TIMING_FULL retention
         measures at least eight complete 48K frames in the fixed 16 MiB ring
3        memory/I/O/contention/interrupt tests pass against frozen evidence tables
4        48K raster/fetch/border/floating-bus unit tests pass
4A       early timing-smoke corpus passes with no unexplained trace divergence
5        headless hashes unchanged by Sokol host; keyboard/Kempston/pacing/
         presentation smoke tests pass on at least Windows and one Unix-like host
6        deterministic beeper/mixer tests pass; host audio cannot alter core hash
7        media-format-support tape matrix frozen; Normal and trap-equivalence
         tape tests pass; malformed media is memory-safe
8        SNA/Z80 byte-level matrix frozen; atomic load/save tests pass across hosts;
         native state records networking_mode without illegal IF1/Ear+Mic combinations
9        128K paging/ULA/AY profile passes deterministic and timing tests
10       Interface1/Microdrive/ZXNet companion spec frozen; NONE/INTERFACE1
         arbitration and cold-reconfiguration tests pass; deterministic device/MDR
         tests and round-trips pass
11       monitor/debugger and ZX Printer required workflows have regression tests
12       UI companion command-registry/menu/workflow/cancel/error/accessibility
         acceptance passes without core divergence
13       release binaries pass dependency inspection and launch tests per platform
14       full timing torture suite and private difficult-media regression run pass
15       complete Section 55 transport/keyboard acceptance and UI companion
         Telnet control/permission acceptance contracts pass, including concurrent
         process first-free Control Port allocation and exhaustion behavior
16       every optimization has before/after canonical regression evidence
```

Failure of an exit gate blocks dependent tickets unless the architecture is
explicitly amended.

### 49.2 Development-ticket derivation contract

An **implementation** ticket derived from this architecture must contain, at
minimum:

```text
architecture section(s) and implementation phase
exact machine profile(s) affected
source/migration-ledger record if legacy behavior is involved
license/provenance clearance when legacy or third-party material is referenced
explicit dependencies and prior phase gates
inputs/state owned and outputs/state produced
forbidden dependency/boundary notes relevant to the ticket
public/synthetic fixture identities or other test inputs appropriate to the ticket
positive acceptance tests
negative/error-path acceptance tests
determinism/cross-host acceptance where applicable
trace/hash evidence required for completion
```

If a required behavioral constant, format rule, hardware fact, or interface
contract is still deferred at the ticket's phase gate, the work item is a
**research/specification ticket**, not an implementation ticket. The result of
that research must update the appropriate companion design document before the
implementation ticket is opened.

A ticket may not resolve an architectural uncertainty merely by choosing the
first implementation that compiles.

Before Phase 11 implementation tickets are issued, the migration ledger must
contain a complete function-level inventory for the monitor/debugger and ZX
Printer, with each historical function classified REQUIRED, LATER, REPLACE, or
DROP and with its regression strategy recorded.

---

## 50. Acceptance criteria

Warajevo ZX Spectrum Next reaches its first architecture-complete milestone only
when all of the following are true:

1. the emulation core is portable C;
2. the core runs headlessly without Sokol;
3. the deterministic core has no host/Sokol/socket dependency;
4. application/orchestration code bridges core output/input to host services;
5. Sokol code is compiled into release program binaries;
6. there is no project-supplied multimedia shared library required at runtime;
7. supported platform builds use the same deterministic core;
8. canonical emulated time is an integer model-specific master tick;
9. CPU T-state/phase is derived from master time rather than defining universal
   machine time;
10. deterministic state hashes match across supported CPU architectures;
11. deterministic state hashes match across supported compilers;
12. Z80 memory and I/O operations can occur at exact intra-instruction master
    ticks;
13. contention is applied by the bus/machine timing model;
14. same-master-edge hardware ordering is explicit and evidence-based;
15. the ULA fetches memory on an explicit raster schedule;
16. CPU writes become visible according to real bus ordering;
17. full border/raster timing is represented;
18. the 256 x 192 bitmap is not treated as the entire display;
19. FLASH and BRIGHT are emulated as Spectrum semantics;
20. floating-bus behavior is derived from the timed ULA model where supported;
21. overscan/rainbow/multicolor effects do not require application-specific
    hacks;
22. NIRVANA/NIRVANA+ class timing can be represented by the core;
23. BIFROST-class timing can be represented by the core;
24. beeper transitions are master-timestamped;
25. AY state advances from emulated master time;
26. canonical audio is deterministic before Sokol conversion;
27. Sokol audio does not drive CPU execution;
28. host sound is enabled only from 0.5x through 2.0x inclusive and is muted
    outside that range without stopping emulated beeper/AY progression;
29. audio buffering cannot silently force the emulator away from the selected
    runtime speed;
30. runtime speed control changes host pacing without changing internal
    Spectrum timing relationships;
31. runtime speed can change while running without discontinuity in the
    master-tick timeline;
32. emulator-controlled cassette playback and SAVE output automatically follow
    the runtime speed multiplier in wall-clock time while retaining canonical
    emulated timing;
33. Normal cassette loading is the default loading mode;
34. Normal loading executes the real emulated loader against timed EAR edges;
35. Instant/Trap loading is optional and explicitly selectable;
36. a supported Instant/Trap load reaches the same Spectrum-observable
    post-load canonical machine state as the corresponding Normal load from the
    same initial state and tape data;
37. unsupported, uncertain, or non-equivalent trap cases fall back safely to
    Normal loading;
38. local and Telnet source ownership is resolved outside the Spectrum core;
39. the Spectrum keyboard core receives only normalized physical matrix
    transitions, never source identity;
40. simultaneous row selection follows authentic matrix-selection behavior;
41. the complete Telnet transport/keyboard acceptance contract in Section 55.20
    passes and the companion UI Telnet-control acceptance contract passes;
42. snapshots are portable across x86-64 and AArch64;
43. original Warajevo media/peripheral behavior is regression-tested;
44. known differences from Warajevo 2.50, and from observable 2.51/2.52
    behavior where used as evidence, are documented when hardware correctness
    intentionally supersedes historical approximation;
45. Windows x86-64 passes the release test suite;
46. Linux x86-64 passes the release test suite;
47. Linux AArch64 passes the release test suite;
48. macOS Apple Silicon passes the release test suite;
49. release dependency inspection confirms the single-program-binary policy;
50. ROM/firmware redistribution status is explicit and legally separated from
    emulator architecture.
51. the complete pinned Fuse Z80 unit-test suite passes with no unexplained
    skipped or failing applicable cases;
52. Phase 0 machine-timing evidence and ROM-hash baselines are frozen before
    dependent timing implementation is accepted;
53. the mandatory Phase 4A early timing-smoke gate passes before host/UI work is
    allowed to mask foundational timing defects;
54. the machine support matrix in Section 9.1 and legacy-feature disposition in
    Section 2.5 are reflected in the backlog with no unspecified "remaining"
    compatibility work;
55. `design/media-format-support.md` is frozen to the required block/variant
    detail before Phase 7/8 media tickets are accepted;
56. the private development-only `WZSN-PRIVATE-TEST-MEDIA` regression run has
    completed with no unexplained failures before full difficult-media or
    hardware-hack compatibility is claimed; the private files themselves are not
    repository or distribution artifacts;
57. every migrated legacy routine has provenance, copyright/license
    classification, derivation clearance, C destination, and regression evidence;
58. the Phase-10 Interface1/Microdrive/ZXNet companion specification is frozen
    before those implementation tickets are accepted;
59. every implementation phase satisfies its Section 49.1 exit gate;
60. Telnet Control-Port probing/bind-family policy, Telnet framing/negotiation,
    keyboard command vocabulary, hold interval, scheduling rule, and second-client behavior
    conform to Section 55, while application-control grammar and command-registry
    projection conform to the UI architecture;
61. implementation tickets satisfy the Section 49.2 derivation contract and do
    not contain unresolved architecture decisions;
62. real-hardware/reference certification covers every item required by Section
    40 before the architecture-complete milestone;
63. every mandatory initial target/compiler group in Section 35 executes the
    required deterministic regression suite;
64. Kempston joystick emulation uses normalized joystick state rather than
    keyboard shortcuts inside the core, and direct port-read tests pass;
65. every required UI workflow, stable command ID, GUI/Telnet/test projection,
    remote-permission rule, screenshot workflow, and menu-state rule defined by
    `design/warajevo-zx-spectrum-next-ui-architecture.md` passes its companion
    acceptance contract;
66. concurrent WZSN processes allocate Control Ports by the Section-55.2
    first-free 30740-32787 contract without duplicate numeric ownership, and full
    range exhaustion is nonfatal;
67. the core networking-mode state is exactly `NONE`, `INTERFACE1`, or `EAR_MIC`
    and cannot represent simultaneous Interface-1/Ear+Mic activation;
68. `NONE`/`INTERFACE1` cold-reconfiguration destroys prior RAM/hook/device state,
    preserves only application run/pause state, and resolves dirty Microdrive
    media without silent data loss before leaving Interface-1;
69. before Architecture #3 is implemented, `EAR_MIC` cannot falsely expose a
    working routed network; when implemented it is selectable only on an
    Architecture-#3-certified ZX Spectrum 48K Issue-2 profile/variant, and its
    actual fidelity/stack/bootstrap acceptance is owned by that downstream
    document;
70. concurrent WZSN processes satisfy the Section-28.7 host-data safety rules for
    settings, writable media, snapshots/exports/conversions, temporary files, and
    Telnet screenshots;
71. every developer/debug WZSN process can own an independent binary circular
    timing trace file whose total size never exceeds 16 MiB;
72. the trace records canonical master ticks, same-tick ordering, CPU/bus/ULA/
    contention/interrupt state deeply enough to reconstruct timing failures and
    `TIMING_FULL` retains at least eight complete 48K frames under the Phase-2
    retention measurement;
73. trace wrap, freeze, file-I/O failure, and trace enable/disable do not alter
    canonical machine state, hashes, or deterministic event ordering;
74. concurrent WZSN processes cannot share or overwrite one another's trace files,
    and a standalone reader can recover complete records from a wrapped or
    abruptly terminated trace while rejecting incomplete trailing data;
75. the circular trace backend is implemented and tested during Phase 1 and is
    available before Phase-2 CPU/timing implementation begins.

---

## 51. Deferred decisions and mandatory freeze gates

The following decisions may remain deferred only until the named gate. They
are not permission for implementation code to choose silently.

```text
Decision                                             Must be frozen before
---------------------------------------------------  ---------------------------
exact 48K master-tick frequency/ratios               Phase 0 exit
exact 48K same-edge CPU/ULA ordering                  Phase 0 exit
exact 48K contention tables                          Phase 0 exit
exact 48K floating-bus model                         Phase 0 exit
versioned circular trace binary record/header layout Phase 0 exit
logical raster sample encoding                       Phase 4 implementation
exact Sokol revision                                  Phase 5 implementation
exact Linux graphics backend within the X11 baseline Phase 5 implementation
default host-visible crop/border                      Phase 5 exit
canonical internal audio sample rate                 Phase 6 implementation
fixed-point mixer representation                     Phase 6 implementation
AY analog mixing model                               Phase 6 implementation
host resampling algorithm for 0.5x..2.0x audio       Phase 6 exit
Interface 1 ROM test/redistribution handling          Phase 10 implementation
exact UI toolkit                                     Phase 12 implementation
Linux package formats                                Phase 13 implementation
keyboard ghosting/electrical conclusion beyond       Phase 5 exit
multiple-row selection
```

Already frozen by this architecture:

- ISO C11 is the core/application C language baseline;
- CMake is the project build generator;
- initial certified machine profiles are 48K PAL and 128K PAL;
- native Wayland is not an initial requirement;
- Windows ARM64 is not an initial target;
- macOS x86-64 is secondary and may be dropped without changing initial
  architecture-complete acceptance;
- CPU micro-operation representation and scheduler container/data structure are
  implementation freedoms, not architecture decisions, provided all behavioral
  contracts and tests pass;
- exact in-memory C structure layout and exact internal C API signatures are
  implementation freedoms unless a field/signature is explicitly frozen by a
  serialization, ABI, test, or subsystem contract;
- the semantic GUI menu hierarchy, toolbar roles, stable application command IDs,
  and Telnet application-control surface are frozen by the UI architecture; minor
  typography/localization wording remains presentation-level freedom;
- Telnet socket/framing, connection, keyboard vocabulary, hold duration, and
  scheduling behavior are frozen in Section 55; Telnet application-control
  grammar and permission policy are frozen by the UI architecture.

A deferred item must be recorded as an explicit design decision before its gate
closes. It must not be decided implicitly by whichever host platform or
developer happens to implement it first.

---

## 52. Non-goals for the initial architecture

The following are not required for the first implementation baseline:

- embedding every operating-system library into the executable;
- native Wayland before the Linux host matrix is frozen;
- mobile platform support;
- WebAssembly support;
- shader-based CRT simulation;
- JIT compilation;
- multithreaded Spectrum core execution;
- hardware-specific SIMD in the deterministic core;
- application-specific timing hacks;
- exact reproduction of historical Warajevo bugs where they disagree with
  authenticated Spectrum behavior.

---

## 53. Final architectural contract

The architecture can be summarized by five non-negotiable boundaries.

### Boundary 1 - Original source versus new implementation

```text
Warajevo Pascal + x86 ASM
        |
        | behavior/provenance reference
        v
portable C implementation
```

### Boundary 2 - CPU versus machine bus

```text
Z80
 |
 | exact timed bus operations
 v
machine bus
 |
 +--> memory
 +--> contention
 +--> ULA
 +--> ports
 +--> peripherals
```

### Boundary 3 - emulated time versus host time

```text
Spectrum master-tick timeline
        |
        v
all machine behavior

host monotonic time
        |
        v
pacing only
```

### Boundary 4 - canonical output versus presentation

```text
ULA -> logical raster -----+
                           +--> host boundary -> Sokol -> display/audio
audio core -> canonical PCM+
```

### Boundary 5 - project binary versus operating system

```text
one Warajevo executable
    contains:
        Warajevo
        Sokol
        approved statically incorporated application code

    uses:
        normal operating-system APIs/frameworks/drivers
```

### Boundary 6 - user front ends versus semantic commands versus machine state

```text
GUI / toolbar / Telnet / tests / future CLI
                    |
                    v
          shared command registry
                    |
                    v
        application/orchestrator
             |            |
             v            v
      deterministic core  host services
```

No front end receives a private implementation of reset, speed, media, input,
screenshot, or debugger semantics when a shared semantic command exists.

The resulting objective is:

> A single-program-binary, cross-platform Warajevo ZX Spectrum Next whose
> Spectrum behavior is deterministic and hardware-timed, whose complex raster
> and audio behavior comes from the emulated machine rather than host
> heuristics, and whose host presentation layer can be replaced without
> changing the emulator core.

---

## 54. Architecture baseline

The principal decisions now frozen for further refinement are:

```text
Implementation core:
    portable C
    no host dependency

Primary host layer:
    Sokol
    connected by application/orchestration code

Distribution objective:
    one program binary per supported platform
    Sokol and approved project third-party code compiled into that binary
    normal system libraries/frameworks permitted

Timing architecture:
    model-specific integer master tick is canonical
    CPU T-state + phase is derived
    exact externally observable bus operations from the beginning

Video architecture:
    timed ULA/raster model
    full raster/border semantics
    native logical raster before host presentation

Audio architecture:
    master-timestamped beeper and AY emulation
    deterministic core mixer
    Sokol push delivery only
    host sound enabled only from 0.5x through 2.0x inclusive
    host sound muted outside that range while emulated audio state continues

Cassette architecture:
    Normal pulse-accurate loading is the default
    runtime speed automatically changes tape wall-clock playback rate
    Instant/Trap loading is optional
    trap result must match Normal post-load Spectrum-visible state for
    supported loader paths

Networking configuration:
    one mutually exclusive mode: NONE / INTERFACE1 / EAR_MIC
    fresh launch defaults to NONE
    INTERFACE1 owns Interface 1 / Microdrive / original ZX Net
    EAR_MIC excludes Interface 1 and is reserved for downstream Architecture #3
    EAR_MIC is selectable only on an Architecture-#3-certified 48K Issue-2 variant
    EAR_MIC pages no networking ROM and auto-installs no resident stack
    networking-mode changes use cold machine reconfiguration, not ordinary Reset
    dirty Interface-1 Microdrive media must be resolved before leaving that mode
    in EAR_MIC, Architecture #3 exclusively owns cassette EAR/MIC semantics

Special Feature 1:
    single-client Telnet keyboard/control server per WZSN process
    base Control Port 30740; first-free probe through 32787 inclusive
    listens on all host interfaces on the selected Control Port
    listener starts automatically
    multi-instance bind selection is race-safe and exclusive
    full 2048-port-range exhaustion is nonfatal and visibly reported
    selected Control Port is session state and is not persisted
    concurrent processes use interprocess-safe config/media/output file handling
    local and Telnet keyboard paths remain simultaneously active
    host input arbiter owns source identity
    Spectrum core receives only normalized matrix transitions
    independent source ownership prevents incorrect key releases
    application-control commands project the shared UI command registry
    host-control commands never bypass application/orchestrator boundaries

UI architecture:
    companion design document owns menus, toolbar, managers, command registry,
    Telnet control commands, screenshot workflow, and remote permission policy

Correctness authority:
    real Spectrum hardware for machine timing/electrical behavior
    preserved Warajevo 2.50 for source migration and provenance
    2.51/2.52 observable behavior as secondary evidence where source is lost
    documented differences where these diverge

Build baseline:
    ISO C11
    CMake

Initial certified machines:
    ZX Spectrum 48K PAL
    ZX Spectrum 128K PAL

Test baseline:
    complete pinned Fuse Z80 unit-test suite is mandatory
    early timing-smoke gate before host/UI expansion
    full timing torture suite before architecture-complete release
    private development-only difficult-media regression run required before
    claiming full difficult-media/hardware-hack compatibility

Provenance baseline:
    LICENSE.txt and NOTICE.md are top-level architecture files
    every migrated routine requires license/provenance classification and
    derivation clearance
```

---

## 55. Special feature 1 - single-client Telnet keyboard/control transport

### 55.1 Purpose

Warajevo ZX Spectrum Next includes a single-client Telnet service that
provides both hardware-level remote keyboard input and an application-control
front end.

This section owns the TCP/Telnet transport, connection lifecycle, bounded
parsing, keyboard-source ownership, deterministic keyboard scheduling, and the
boundary by which non-keyboard commands are handed to the shared application
command registry. The complete application-control command surface, including
menu projection, reset/speed controls, screenshot, permissions, responses, and
stable command IDs, is owned by
`design/warajevo-zx-spectrum-next-ui-architecture.md`.

The keyboard part of the feature exists to let a remote client operate the
emulated Spectrum at the same hardware keyboard-matrix level as a person using
the host computer's physical keyboard.

The defining keyboard requirement is:

> Once normalized into an emulated key transition, a local physical key event
> and an equivalent Telnet key event are indistinguishable to the Spectrum
> hardware model.

The feature must not inject BASIC characters, edit the ROM input buffer,
simulate BASIC tokens, call ROM routines, or write directly into Spectrum RAM.

It drives the emulated keyboard matrix.

### 55.2 Network endpoint and multi-instance Control Port allocation

The Telnet keyboard/control service is a per-process host service. Multiple WZSN
processes may run concurrently on the same host. Each process independently
selects one numeric **Control Port** by probing upward from the canonical base
port.

The frozen allocation contract is:

```text
Transport:          TCP
Protocol style:     Telnet-compatible line-oriented control
Base Control Port:  30740
Probe count:        2048 numeric ports total
Probe range:        30740 through 32787 inclusive
Probe order:        strictly ascending
Selection:          first bindable candidate under the family-ownership rule
Bind address:       all available host interfaces
Clients:            one active client per WZSN process
Startup:            automatic with the Warajevo application
Persistence:        selected Control Port is session state and is not persisted
```

For each candidate numeric port `P`, the process attempts wildcard reachability
for every supported address family. Conceptually:

```text
IPv4:  0.0.0.0:P
IPv6:  [::]:P
```

The implementation may use one dual-stack IPv6 listener or separate IPv4 and
IPv6 listeners according to host socket semantics, but one WZSN process owns
one numeric Control Port across the supported families. A candidate is rejected
and all partial listeners for it are closed if any supported family reports
that the numeric port/address is already in use by another process or service.
This prevents two WZSN instances from both reporting the same Control Port while
owning different address families.

An unsupported address family does not reject a candidate. If one family has a
non-conflict failure while another family successfully binds, the candidate may
be accepted in a visibly reported **degraded-family** state. The application
must report which address families are active.

The bind itself is the interprocess reservation. Listener socket options must
not permit two WZSN processes to share the same wildcard address/port as
independent servers. Where the operating system provides an explicit exclusive
listener option, the host wrapper should use it. A check-then-bind probe without
an atomic/exclusive bind is insufficient because simultaneous process startup
must not race into duplicate ownership.

The feature must not be restricted to loopback.

If a candidate is rejected because it is already in use, the process advances
to the next numeric port. It must not skip a lower bindable candidate merely to
preserve a prior session's port. Example:

```text
first instance:   30740
second instance:  30741
third instance:   30742
```

if those ports are free in that order. If `30741` is occupied by another
service, the next WZSN instance may select `30742`.

If no candidate in `30740..32787` can establish any wildcard listener:

- Warajevo continues running;
- local keyboard input remains fully available;
- Control Port status is visibly reported as unavailable;
- no port outside the frozen 2048-port probe range is selected automatically.

The selected Control Port is host session state, not Spectrum state. It does not
enter deterministic hashes, snapshots, or replay. A later explicit user
configuration that changes the base/range policy requires an architecture
change.

### 55.3 One-client rule

Exactly one Telnet client may own the active remote-keyboard session at a time.

If a second client connects while another client is active, the server sends:

```text
BUSY\r\n
```

and immediately closes the second session. It does not parse keyboard commands
from that connection.

The first active client's connection remains unaffected.

The listening socket remains available after a client disconnects so that a
new client may connect.

### 55.4 Local keyboard remains permanently available

The Telnet feature must never disable the normal local keyboard path.

These input paths remain simultaneously available:

```text
host physical keyboard ----+
                            |
                            v
                      input arbiter
                            |
Telnet client --------------+
                            |
                 normalized effective
                  matrix transitions
                            |
                            v
                 Spectrum keyboard matrix
```

A connected Telnet client is an additional keyboard source, not an exclusive
keyboard mode.

Connecting, disconnecting, or using the Telnet client must not require the
user to switch Warajevo out of normal local-keyboard operation.

### 55.5 Hardware-level keyboard injection

Local and Telnet input converge **before** the Spectrum core.

Source identity exists only in the host-side input arbiter.

Conceptually:

```text
local physical keyboard ----+
                            |
                            v
                      input arbiter
                            |
Telnet client --------------+
                            |
                 source ownership / OR
                            |
                            v
               normalized matrix transition
                            |
                            v
                 Spectrum keyboard core
```

The Spectrum core API receives only a physical matrix transition and its
effective master tick.

Conceptually:

```c
void wz_keyboard_set_matrix_key(
    wz_machine_t* machine,
    wz_spectrum_key_t key,
    bool pressed,
    wz_master_tick_t effective_tick
);
```

The exact API is not frozen.

There is deliberately no `LOCAL` or `TELNET` argument in the core keyboard API.

This makes local-versus-Telnet indistinguishability structural rather than
dependent on convention.

### 55.6 Independent source ownership

Local and Telnet key state must be stored independently by the host input
arbiter.

For each emulated Spectrum key:

```text
effective_pressed =
    local_pressed
    OR
    telnet_pressed
```

This is mandatory.

Example:

```text
1. local user presses A
2. Telnet client presses A
3. local user releases A
4. A must still be pressed because Telnet still holds it
5. Telnet client releases A
6. A is now released
```

A release event from one source must never cancel another source's still-held
key.

This rule also applies to modifier keys such as CAPS SHIFT and SYMBOL SHIFT.

### 55.7 Spectrum matrix semantics

Remote keys must map to the same physical 48K Spectrum keyboard positions as
local keys.

For the classic 8 x 5 active-low matrix, the implementation must preserve the
actual row/column selection model used by the machine core.

Examples include:

```text
CAPS SHIFT
SYMBOL SHIFT
ENTER
SPACE
A..Z
0..9
```

The keyboard port model must correctly handle software selecting more than one
keyboard row simultaneously through the address lines. It must not assume that
exactly one row is selected for every read.

Any additional ghosting or electrical interaction that can occur in the real
matrix when multiple switches/rows are involved must be resolved from hardware
evidence before that behavior is either implemented or explicitly ruled out.

A host character such as an uppercase letter or punctuation mark may require
multiple physical Spectrum keys.

The host-side mapping layer may generate the required combination. The core
still receives only physical matrix switch states.

### 55.8 No ROM-level shortcut

The following are explicitly forbidden for this feature:

```text
writing characters into BASIC workspace
calling ROM keyboard routines
injecting tokens
editing system variables to simulate input
writing directly to the ROM input buffer
patching PC to bypass keyboard scanning
```

Programs that scan the keyboard matrix directly must see Telnet input exactly
as they see equivalent local input.

This includes games and machine-code programs that never call the ROM keyboard
routines.

### 55.9 Input timing

Socket arrival time is host time and therefore cannot itself become canonical
Spectrum timing.

Both local and Telnet input are normalized into the same deterministic
emulated-input scheduling path.

Conceptually:

```text
host event arrives
        |
        v
normalize to Spectrum key transition
        |
        v
assign effective emulated master tick
        |
        v
deterministic input queue
        |
        v
keyboard matrix state changes
```

The effective-tick policy is frozen as follows:

1. the application/orchestrator collects host input into a FIFO carrying a
   monotonically increasing host-event sequence number;
2. before the emulation thread advances the core for its next run slice, it
   drains the pending host-input FIFO;
3. source ownership is resolved by the input arbiter;
4. every resulting effective Spectrum matrix transition in that drain is
   timestamped at the core's **current master tick**, before further machine
   execution;
5. transitions sharing that master tick are applied in host-event FIFO order;
6. only the normalized effective transition and master tick are recorded for
   deterministic replay.

The policy is identical for equivalent local and Telnet events. Host arrival
time determines which orchestrator drain observes a live event, but it never
becomes a Spectrum timestamp.

### 55.10 Runtime-speed interaction

The Telnet server remains available at every emulator runtime speed.

Changing Warajevo from 100% to 400%, 50%, or Unlimited changes host pacing but
does not change keyboard hardware semantics.

A Telnet key transition is inserted at its assigned emulated master tick in exactly
the same way as a local key transition.

The server itself remains responsive in host wall-clock time and must not wait
for a particular Spectrum frame before accepting network data.

### 55.11 Telnet protocol and command-dispatch boundary

The network service implements the minimal Telnet framing required for ordinary
clients. Telnet IAC negotiation bytes must never be interpreted as Spectrum key
commands or application-control commands.

The initial server does not require any Telnet option. It must consume escaped
`IAC IAC` correctly, consume `WILL`/`WONT`/`DO`/`DONT` negotiation sequences,
reject unsupported requested options with the corresponding `DONT` or `WONT`,
and consume subnegotiation through `IAC SE` without passing those bytes to the
application command parser. It sends no unsolicited application banner after a
successful connection.

The application protocol is line-oriented. The complete user-visible grammar is
defined by `design/warajevo-zx-spectrum-next-ui-architecture.md` and is parsed
outside the Spectrum core.

This core/system document freezes the hardware-keyboard command family:

```text
KEY DOWN <key>
KEY UP <key>
KEY PRESS <key>
RELEASE ALL
```

The canonical physical Spectrum key-name vocabulary is exactly:

```text
CAPS_SHIFT  Z  X  C  V
A           S  D  F  G
Q           W  E  R  T
1           2  3  4  5
0           9  8  7  6
P           O  I  U  Y
ENTER       L  K  J  H
SPACE       SYMBOL_SHIFT  M  N  B
```

These names identify physical matrix keys. Host characters such as `!`, `"`,
uppercase letters, or BASIC tokens are not protocol key names; clients express
such input by issuing the required physical-key transitions.

`KEY DOWN`, `KEY UP`, `KEY PRESS`, and `RELEASE ALL` are routed through the
host input arbiter described by Sections 55.5-55.9. They never enter the shared
application command registry as character/text injection operations.

`KEY PRESS` is a convenience command. For a key not already Telnet-held and not
already awaiting an automatic Telnet release, it schedules:

```text
matrix key down at master tick N
matrix key up   at master tick N + (2 * machine_frame_master_ticks)
```

Thus the deterministic hold duration is exactly **two emulated frame periods of
the active machine profile**, with `machine_frame_master_ticks` evaluated when
the command is accepted. Runtime speed changes only its wall-clock duration.
If the Telnet source already holds that key, or an automatic release for that
key is already pending, the command is rejected without altering the existing
hold.

`RELEASE ALL` releases every key owned by the Telnet source and leaves local
source ownership unchanged.

All non-keyboard application-control lines recognized by the UI protocol are
handed to the application/orchestrator's shared command registry. The Telnet
networking code must not implement private reset, speed, screenshot, media,
debugger, menu, or settings semantics.

In particular:

- `RESET` must reach the same machine-reset command as the GUI/toolbar;
- `SPEED` must reach the same runtime-speed command as the GUI/toolbar;
- `SCREENSHOT` must reach the shared screenshot service defined by Section 19.6;
- `MENU`, `DESCRIBE`, and `DO` must query/dispatch the same command registry used
  by the GUI and automated application tests.

The command parser uses bounded storage. Exact line encoding, command grammar,
response framing, special-command aliases, screenshot filename contract, and
remote permission classes are frozen by the UI architecture.

The initial transport has **no application-level authentication or encryption**.
Telnet is plaintext and the listener is intentionally network-visible according
to Section 55.2. The UI architecture therefore defaults privileged host-file,
destructive-media, and local-only application commands to non-executable over
Telnet while keeping the command tree discoverable.

### 55.12 Telnet disconnect behavior

When the active Telnet client disconnects unexpectedly or normally:

1. all keys owned by the Telnet source are released;
2. local-source key states are preserved;
3. the Spectrum sees the resulting normal matrix transitions;
4. the server returns to listening for the next client.

This prevents a disconnected remote client from leaving a Spectrum key stuck.

### 55.13 Emulator reset behavior

Resetting the emulated Spectrum does not inherently terminate the Telnet
server.

The server is a host feature and may remain connected across a machine reset.

The initial reset policy does **not** clear local/Telnet arbiter ownership or
pending deterministic `KEY PRESS` releases. Keys still held by either source
therefore remain held across an emulated machine reset and pending automatic
releases still occur at their already assigned master ticks. Changing that
policy requires an explicit later architecture change applied consistently to
all host input sources.

### 55.14 Deterministic recording and replay

External network arrival timing is nondeterministic host input.

For deterministic tests and replay, Warajevo records the normalized Spectrum
matrix transitions and their assigned emulated master ticks, not raw TCP packet
timing.

A replay must therefore work headlessly without a Telnet server.

Equivalent local and Telnet traces must become identical normalized
keyboard-event traces once source ownership is resolved.

### 55.15 Host/network implementation boundary

Sokol does not provide the networking function for this feature.

The Telnet server uses a very small platform socket abstraction implemented
with operating-system networking APIs:

```text
Windows:
    Winsock

Linux/macOS:
    POSIX/BSD sockets
```

No third-party networking runtime is required.

This preserves the single-program-binary distribution objective.

The socket implementation is host code. It may not enter the emulation core.

### 55.16 Threading

The Telnet server must not mutate machine state directly from a networking
thread.

Acceptable designs include:

```text
network thread / nonblocking socket poll
        |
        v
host-side command queue
        |
        v
main emulation thread
        |
        v
normalized deterministic key event
        |
        v
keyboard matrix
```

or a completely nonblocking single-threaded host poll.

Whichever implementation is chosen, one emulation thread remains the owner of
mutable Spectrum state.

### 55.17 Backpressure and malformed input

The Telnet parser must use bounded buffers.

A remote client must not be able to cause unbounded memory growth by sending
an unterminated or excessively long command.

Malformed commands must:

- leave Spectrum state unchanged;
- produce a controlled protocol error or be rejected;
- not terminate the emulator;
- not desynchronize subsequent valid commands.

### 55.18 Security boundary

Binding to all interfaces on the dynamically selected Control Port intentionally exposes a remote
keyboard-and-application-control surface to networks reachable by the host.

Telnet traffic is plaintext.

This architecture therefore requires the application to make the listener
state visible to the user and to document that host firewall/network policy
controls reachability.

The initial baseline intentionally has no application-level authentication or
encryption. The companion UI architecture therefore freezes conservative
remote permission classes and default-denial rules for host-file, destructive,
and local-only operations. Any later authentication/access-control feature is
an explicit architecture change and must not alter the keyboard-matrix
equivalence contract.

### 55.19 Testing requirements

The transport/keyboard layer requires automated tests for at least:

```text
local A down/up
Telnet A down/up
identical effective matrix state

local + Telnet same key held concurrently
one source releases while other still holds

CAPS SHIFT / SYMBOL SHIFT combinations
multi-key combinations
direct keyboard-matrix scanning software

Telnet disconnect with keys held
local keys preserved after Telnet disconnect

second-client rejection
new client accepted after first disconnect

IAC IAC, WILL/WONT/DO/DONT, and subnegotiation handling
unsupported Telnet options rejected without reaching command parser
no unsolicited application banner on successful connection
bounded line buffering and malformed-input recovery

KEY PRESS produces exactly two emulated frame periods of hold
KEY PRESS on already-held/pending key is rejected without changing the hold
multiple keyboard rows selected simultaneously
matrix electrical/ghosting tests as required by hardware evidence

automatic listener startup
first-free probing from 30740 through 32787 inclusive
simultaneous multi-process startup cannot produce duplicate Control Port ownership
IPv4/IPv6 wildcard-family reporting and degraded-family behavior
candidate rejection when any supported family reports address-in-use
2048-port-range exhaustion is nonfatal and visibly reported
selected Control Port is not persisted and no port outside the range is chosen
second client receives BUSY then closes

runtime speed changes while Telnet client is active
Normal/Unlimited emulator speed operation

host-input FIFO order and current-master-tick scheduling rule
headless deterministic replay from normalized key trace

non-keyboard control line dispatch reaches shared command registry
Telnet networking layer contains no private reset/speed/screenshot semantics
```

For a given normalized event and assigned master tick, the core must produce the
same keyboard-matrix result regardless of whether the event originated from the
local host keyboard or Telnet.

The companion UI architecture separately requires command-registry projection,
`RESET`, `SPEED`, `SCREENSHOT`, menu-tree discovery, response, and remote-policy
tests. Phase 15 requires both sets.

### 55.20 Acceptance contract

The Telnet transport/keyboard portion of Special Feature 1 is complete only
when:

1. the Telnet listener starts automatically with each Warajevo application process;
2. each process probes candidate Control Ports in strict ascending order from
   30740 through 32787 inclusive and selects the first candidate satisfying the
   Section-55.2 family-ownership rule;
3. simultaneous WZSN process startup cannot result in two processes owning or
   reporting the same Control Port through split IPv4/IPv6 or shared-listener
   semantics;
4. exhaustion of all 2048 candidates is nonfatal, visibly reported, and never
   causes automatic selection outside the frozen range;
5. the selected Control Port is session state and is not persisted across
   application launches;
6. active/degraded IPv4/IPv6 family state is visibly reported for the selected
   Control Port;
7. one active Telnet client per WZSN process is supported;
8. a second simultaneous client receives `BUSY` and is immediately closed
   without disturbing the first;
9. local keyboard input remains continuously available;
10. local/Telnet source identity and ownership are resolved outside the Spectrum
    core;
11. the Spectrum core receives only normalized physical matrix transitions and
    master ticks;
12. local and Telnet equivalent effective transitions are
    Spectrum-indistinguishable;
13. releases from one source cannot cancel a key held by the other;
14. direct keyboard-matrix-scanning software sees Telnet keys correctly;
15. simultaneous selection of multiple keyboard rows follows the authentic
    matrix-selection model;
16. any real matrix ghosting/electrical behavior is validated from hardware
    evidence before being implemented or ruled out;
17. no ROM/BASIC input shortcut is used;
18. `KEY DOWN` and `KEY UP` provide explicit physical-key state control;
19. `KEY PRESS` holds for exactly two active-machine emulated frame periods and
    refuses to override an existing Telnet hold;
20. Telnet disconnect releases only Telnet-held keys;
21. Telnet negotiation bytes cannot become keyboard or application commands;
22. malformed network input cannot corrupt Spectrum state;
23. deterministic keyboard replay does not require the network server;
24. the feature works at every runtime speed;
25. no third-party networking shared library is introduced;
26. the socket/network layer remains outside the deterministic Spectrum core;
27. normalized live input uses the Section 55.9 FIFO/current-master-tick
    scheduling rule for both local and Telnet sources;
28. bounded command parsing recovers deterministically after malformed/overlong
    input;
29. non-keyboard application-control requests are dispatched through the shared
    application command registry rather than implemented privately in network
    code;
30. the initial transport contains no application-level authentication or
    encryption and the UI/documentation visibly states that security boundary;
31. every Telnet application-control, menu-projection, screenshot, response, and
    remote-permission requirement in
    `design/warajevo-zx-spectrum-next-ui-architecture.md` also passes before
    Phase 15 exits.
