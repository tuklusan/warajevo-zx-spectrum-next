<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms, warranty disclaimer, termination,
patent, trademark, and governing-law provisions. -->

# Warajevo ZX Spectrum Next — Complete T-State and Machine-Time Description

This document is the timing contract for the Architecture #1 core and the
Architecture #2 user-facing emulator. It is subordinate to the canonical
architecture documents `01-*` and `02-*`, and is organized for the execution
steps in `04-*`. The `03-*` architecture is explicitly out of scope.

The purpose of this document is not to provide a fast instruction counter. It
defines the observable sequence of CPU bus operations, ULA activity, memory and
I/O contention, interrupts, raster state, tape edges, and ROM-visible behavior
on one deterministic machine timeline.

## 1. Scope and authority

The initial certified target is the 48K PAL profile. The 128K PAL profile is a
later required profile and must reuse the same timing architecture with its own
validated profile data. The emulator must not use host wall-clock time,
compiler evaluation order, or GUI frame delivery to determine machine state.

The authoritative hierarchy is:

1. validated hardware measurements and cited timing evidence;
2. frozen profile constants and same-edge ordering tables;
3. this document and the Architecture #1/#2 documents;
4. implementation code and test fixtures.

If an implementation disagrees with hardware evidence, the implementation is
wrong until the evidence is revisited and the profile is deliberately revised.

## 2. Canonical clock domains

### 2.1 Master time

All devices share an integer `wz_master_tick_t` timeline:

```c
typedef uint64_t wz_master_tick_t;
```

The machine profile stores the master frequency as an exact rational and the
exact relationship to CPU T-states. No floating-point value may decide a
machine event. CPU T-state, CPU phase, frame, scanline, raster clock, ULA fetch
position, audio edge, and tape edge are derived coordinates over this timeline.

### 2.2 Classic PAL profile constants

The initial 48K PAL contract is:

| Quantity | Value |
|---|---:|
| CPU T-states per scanline | 224 |
| Scanlines per frame | 312 |
| CPU T-states per frame | 69,888 |
| ULA pixel-clock positions per scanline | 448 |
| Active bitmap width | 256 pixels |
| Active bitmap height | 192 lines |
| Frame rate | approximately 50 Hz; freeze from evidence |

The 128K PAL placeholder profile in the architecture is 228 T-states per line,
311 lines per frame, and 70,908 T-states per frame. These values are not a
substitute for the required 128K evidence freeze.

For an integral profile ratio:

```text
cpu_tstate = master_tick / master_ticks_per_cpu_tstate
cpu_phase  = master_tick % master_ticks_per_cpu_tstate
```

The scheduler timestamp is always `master_tick`, never a CPU T-state alone.

## 3. CPU instruction execution model

The Z80 executes one instruction as an ordered sequence of machine cycles. A
machine cycle is an ordered sequence of T-states. Every T-state has a defined
start/end interval on the master timeline and may expose a bus, refresh, ULA,
contention, interrupt, or internal CPU event.

The instruction engine must therefore expose at least:

```text
instruction boundary
machine-cycle kind
cycle T-state index
bus address and direction
bus data or data-source phase
MREQ / IORQ / RD / WR / M1 / RFSH meaning
wait/contention insertion
refresh address and refresh data
interrupt sampling and acceptance
```

The decoder may be table-driven, but opcode tables must identify all prefix and
repeat paths explicitly. A compact fast path is valid only when its externally
visible bus and timing trace is equivalent to the cycle engine.

### 3.1 T-state phases

For each machine cycle, represent these conceptual phases separately:

1. cycle setup and address presentation;
2. address/control assertion;
3. device/ULA observation and possible contention;
4. data sample or data drive;
5. control release and cycle completion.

The exact edge ordering is profile data. Do not infer it from C statement order.

### 3.2 Base machine-cycle classes

The following classes are the minimum CPU bus vocabulary:

| Cycle | Meaning | Typical duration |
|---|---|---:|
| `M1_FETCH` | opcode fetch with refresh semantics | 4 T-states before waits |
| `MEM_READ` | operand/data memory read | 3 T-states before waits |
| `MEM_WRITE` | operand/data memory write | 3 T-states before waits |
| `IORQ_READ` | I/O read, including keyboard/ULA reads | 4 T-states before waits |
| `IORQ_WRITE` | I/O write, including port-FE writes | 4 T-states before waits |
| `INT_ACK` | maskable interrupt acknowledge | profile-defined; includes refresh behavior |
| `INTERNAL` | CPU-only work with no external bus | instruction-specific |

The duration in the table is the normal base duration. Contention and explicit
wait states extend the cycle; they never alter the instruction's architectural
semantics.

### 3.3 Common documented instruction timings

These are reference values for the uncontented Z80 path. Prefixes, indexed
displacements, conditional paths, and repeat termination must be represented as
separate paths rather than reconstructed from a single average.

| Instruction family | Taken/normal | Untaken/alternate |
|---|---:|---:|
| `NOP` | 4 T | — |
| `LD r,r'` | 4 T | — |
| `LD r,n` | 7 T | — |
| `LD (HL),r` | 7 T | — |
| `LD r,(HL)` | 7 T | — |
| `LD (HL),n` | 10 T | — |
| `INC/DEC r` | 4 T | — |
| `INC/DEC (HL)` | 11 T | — |
| `JP nn` | 10 T | — |
| `JP cc,nn` | 10 T | 10 T |
| `JR e` | 12 T | — |
| `JR cc,e` | 12 T | 7 T |
| `DJNZ e` | 13 T | 8 T |
| `CALL nn` | 17 T | — |
| `CALL cc,nn` | 17 T | 10 T |
| `RET` | 10 T | — |
| `RET cc` | 11 T | 5 T |
| `RST p` | 11 T | — |
| `PUSH qq` | 11 T | — |
| `POP qq` | 10 T | — |
| `LD (nn),A` / `LD A,(nn)` | 13 T | — |
| `LD rr,nn` | 10 T | — |
| `LD (nn),rr` / `LD rr,(nn)` | 20 T | — |
| `IN A,(n)` / `OUT (n),A` | 11 T | — |
| `EX (SP),HL` | 19 T | — |
| `DI` / `EI` | 4 T | — |
| `HALT` | 4 T to enter; repeated fetch behavior while halted | — |

The table is a sanity reference, not permission to omit the underlying bus
trace. The implementation must add the official DD/FD, CB, ED, and DD/FD-CB
indexed families, including displacement reads, undocumented forms selected by
the compatibility policy, and block repeat timing.

### 3.4 Prefix and repeat rules

Prefix handling is an instruction-stream operation:

- DD/FD changes the interpretation of affected HL-class operands;
- repeated DD/FD prefixes consume their own fetch timing and the final
  effective prefix determines the instruction;
- CB after DD/FD consumes a displacement and applies the indexed bit operation;
- ED selects extended operations and has explicit undefined/undocumented policy;
- a repeating block instruction performs the full repeat path until its
  terminating condition, with the final and repeating paths separately traced.

The `M1` refresh side effect occurs for each opcode-fetch machine cycle that the
selected Z80 behavior defines, including prefix fetches. `R` increments with
the documented refresh behavior and is not a generic instruction counter.

## 4. Memory bus and contention

### 4.1 48K memory map

The 48K profile presents a 16K ROM region followed by 48K RAM:

```text
0000-3FFF  ROM
4000-7FFF  contended display RAM
8000-BFFF  contended/general RAM according to the profile
C000-FFFF  uncontended RAM
```

The ROM is supplied externally to the emulator, identified by a declared hash,
and loaded before the machine is released from reset. ROM bytes are not silently
substituted by a host file or embedded into release artifacts.

### 4.2 Contention

Contention is a bus-level delay inserted when a CPU bus operation overlaps a
profile-defined contended ULA access window. The model must define:

- which addresses are contended;
- which machine cycles are delayed;
- the delay table for each raster phase;
- whether I/O operations incur the memory/I/O contention sequence;
- the ULA fetch that owns each contested slot;
- same-tick ordering between CPU request, ULA fetch, and visible writes.

Contention belongs in the memory/I/O bus and machine timing layer, never in
individual opcode implementations. A delayed CPU cycle advances the shared
master timeline; it does not merely add a statistic after the instruction.

### 4.3 Floating bus

Reads from partially decoded or floating ULA address space return the data value
currently visible on the ULA/bus at the sampled master tick. The implementation
must derive this from scheduled ULA fetches and bus ownership, not from the last
CPU read or a random value.

## 5. I/O and interrupt timing

### 5.1 Port FE

The ULA port-FE path must decode the Spectrum's partial address behavior and
sample/write these externally visible latches at the correct bus edge:

- border color;
- beeper output;
- MIC output;
- keyboard row selection on reads.

A port-FE write is a timed machine event. Border and audio changes become
visible at the profile-defined master tick of the write, not at instruction
completion or host presentation time.

### 5.2 Maskable interrupts

The profile schedules the interrupt request and defines its duration. The CPU
samples interrupt acceptance at the documented instruction boundary and only
accepts it when `IFF1` and the HALT/enable rules permit it. `EI` has the Z80's
delayed acceptance behavior; `DI` disables acceptance immediately according to
the CPU rules.

Interrupt modes 0, 1, and 2 require distinct acknowledge/vector behavior. The
acknowledge bus cycle, refresh side effect, stack writes, vector reads, and
entry timing must be traceable. A frame interrupt is not implemented as a host
callback.

## 6. ULA raster and frame timeline

For 48K PAL, raster state derives from the same master tick as the CPU:

```text
frame       = master_tick / ticks_per_frame
line        = (master_tick / ticks_per_line) % lines_per_frame
line_phase  = master_tick % ticks_per_line
```

The complete frame includes border and blanking regions. The active 256x192
bitmap is only a subregion. ULA fetches, attribute fetches, border writes,
floating-bus values, flash phase, and interrupt edges are scheduled over the
full 312-line frame.

The canonical raster output is indexed/palette state, not host RGB pixels. A
host may drop a presented frame but may not skip emulated raster events or
change their timing to maintain display rate.

## 7. Tape and ROM-loader behavior

Tape playback is an emulated peripheral. In Normal mode, the tape subsystem
produces deterministic EAR-level transitions at master ticks; the ROM samples
those transitions through the normal I/O path. Instant/Trap mode is an explicit
alternative and must preserve safe fallback to authentic timed loading when
recognition is uncertain.

The ROM is therefore required for meaningful loader tests. A runner must:

1. obtain the approved ROM through the project-controlled runner setup;
2. verify its expected identity/hash before execution;
3. inject it into the machine profile without committing the ROM bytes;
4. reset the machine into the authentic ROM entry path;
5. mount each supported tape image through the media subsystem;
6. run in Normal mode first, preserving master-tick traces and EAR edges;
7. classify failures as parser, tape-edge, CPU, bus/contention, ROM identity,
   raster, or host/UI defects.

The ROM must never be fetched from an arbitrary developer machine path in CI.
If licensing or secret distribution prevents a runner from obtaining the ROM,
the run must report `ROM_UNAVAILABLE` and fail certification rather than claim a
successful emulator test.

## 8. Test-media acceptance sequence

Every usable file in `test-media/` is a public difficult-media candidate. The
test harness must enumerate the directory without assuming a particular case or
extension, detect TAP/TZX/WAV as supported, and produce a deterministic result
record containing:

```text
media path and format
ROM profile and ROM hash
machine profile
loading mode
initial master tick
first and last tape edge
terminal machine state or timeout
frame/raster hash where applicable
timing trace/checkpoint path on failure
failure classification
```

A clean run means that every file accepted by the format matrix loads or reaches
its declared expected outcome without an unexplained divergence. It does not
mean that an unsupported or malformed file is silently ignored. Unsupported
formats must be reported explicitly and remain visible in the acceptance
summary.

## 9. Required timing tests

Before declaring the core architecture complete, the test suite must cover:

- every base opcode and all supported prefix families;
- taken and untaken conditional timing;
- indexed displacement reads and writes;
- block repeat and terminating paths;
- HALT, EI/DI, and all interrupt modes;
- ROM/RAM boundaries and ROM write protection;
- contended and uncontended memory access;
- contended I/O and port-FE border/beeper/MIC writes;
- floating-bus reads during ULA fetch windows;
- frame, line, active-display, blanking, and interrupt boundaries;
- raster-racing, overscan, multicolor, and flash-phase cases;
- tape-edge timing through the authentic ROM loader;
- all `test-media/` images that the frozen media matrix accepts.

Each timing failure must preserve a minimal deterministic reproducer and a
first-divergence trace. Fixes must add a regression before the implementation is
changed again.

## 10. Implementation invariants

The following are hard invariants:

1. One integer master timeline owns all emulated time.
2. CPU T-state is a derived CPU coordinate, not an independent clock.
3. Bus requests and waits are visible to the scheduler.
4. Contention is applied by the bus/profile, not by opcode-specific hacks.
5. ULA fetches and visible writes have explicit same-tick ordering.
6. Host clocks, host thread scheduling, and display callbacks cannot affect
   canonical machine state.
7. ROM identity is explicit and verified.
8. Normal tape loading is the baseline compatibility path.
9. Unsupported media and unavailable ROMs are explicit failures.
10. Optimizations are accepted only after trace/state equivalence is proven.

This reference intentionally does not define the out-of-scope Architecture #3
network/router behavior.
