<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Warajevo ZX Spectrum Next - UI Architecture

This document is the canonical authority for the user-facing application
architecture of Warajevo ZX Spectrum Next (WZSN).

It is a companion to:

```text
design/warajevo-zx-spectrum-next-architecture.md
```

The core/system architecture remains authoritative for machine timing,
deterministic state, CPU/bus/ULA/audio/media semantics, host/core boundaries,
and Telnet transport/keyboard injection. This document may expose those
capabilities; it may not redefine them.

The downstream routed-network architecture is:

```text
design/zx48-mic-ear-router-network-architecture.md
```

That document owns the Ear+Mic resident stack, router protocol, physical/virtual
MIC/EAR behavior, and Architecture-#3 acceptance. This UI document owns only
the WZSN presentation/command surface used to select or control that downstream
feature.

---

## 1. UI mission

The WZSN interface exists to make an accurately emulated ZX Spectrum easy to
operate without discarding the unusually powerful media, inspection, and
compatibility facilities that distinguished classic Warajevo.

The central product model is:

```text
running Spectrum
    |
    +-- ordinary machine controls
    +-- mounted media
    +-- debugger/diagnostics
    +-- compatibility/preservation tools
    +-- local GUI control
    +-- Telnet remote control
```

The modern UI must **preserve capabilities, not preserve 1998 navigation**.

Classic Warajevo was organized around a DOS integrated environment that
selected and transformed files before launching separate emulator executables.
WZSN is one continuously running application. Its UI is therefore centered on
the emulated machine, with media managers and preservation tools surrounding
that machine.

---

## 2. Sources and historical authority

The legacy integrated-environment menu structure is reconstructed from the
preserved Warajevo 2.50/2.51-era sources and help, principally:

```text
src/environment/SETSTATE.PAS
src/environment/MENUCONS.PAS
src/environment/WARAJEVO.PAS
src/environment/HELP.TXT
src/environment/DOSMENU.PAS
src/environment/SETUP.PAS
```

The historical top-level environment exposed seven primary menus:

```text
TapeFiles
Z80Snaps
MdriveFiles
DockFiles
DataBase
Setup
DOS
```

It also exposed a persistent Start action, shortcut/status line, a Names window
showing selected files/configuration, and, in graphics mode, a Spectrum-screen
window.

The item-by-item disposition of those facilities is frozen in Section 20 of
this document.

---

## 3. Non-negotiable UI principles

### 3.1 The running machine is primary

The user should be able to launch WZSN, see the emulated machine, insert media,
reset, change emulation speed, pause/resume, save/load state, and inspect status
without entering a separate environment or launching a second executable.

### 3.2 File format is not a top-level navigation concept

Formats such as Z80, SNA, TAP, TZX, MDR, DCK, SLT, SIT, SNP, and historical
emulator-specific formats do not each receive top-level menus.

The top-level UI is organized by user intent:

```text
File
Machine
Media
View
Tools
Settings
Help
```

Format-specific detail appears only in the relevant media manager,
compatibility tool, file workflow, or diagnostic surface.

### 3.3 One semantic command, many front ends

Every user-invokable semantic application operation must be represented once in
a shared application command registry.

The GUI menu, toolbar, Telnet control interface, automated application tests,
and any future command-line interface are projections of that registry.

No front end may contain a private implementation of reset, model selection,
emulation-speed control, media ejection, screenshot capture, debugger control,
or another operation that already exists as a semantic command.

### 3.4 Presentation cannot define emulation

Menus, dialogs, file choosers, toolbars, window layout, animations, and host
threads are presentation concerns.

They may request changes through application/orchestrator commands. They may
not directly mutate deterministic Spectrum state or become a source of
canonical emulated time.

### 3.5 Powerful tools must not obstruct ordinary use

Preservation-oriented conversions, block editors, sector editors, and legacy
format transformations remain available where retained, but they are grouped
under managers or Compatibility Tools rather than exposed across the primary
menu bar.

### 3.6 State-sensitive actions remain explicit

Classic Warajevo enabled and disabled menu operations according to selected
media and machine type. WZSN preserves this principle through command-registry
predicates rather than through ad-hoc GUI logic.

A disabled command must have a machine-readable reason.

---

## 4. Shared application command registry

### 4.1 Purpose

The shared command registry is the semantic backbone of the WZSN application.

Conceptually:

```text
GUI menu ---------+
Toolbar -----------+
Telnet ------------+----> shared command registry ----> application/orchestrator
Application tests -+                                      |             |
Future CLI --------+                                      v             v
                                                   deterministic     host services
                                                       core
```

### 4.2 Required command metadata

Every registered command has, at minimum:

```text
stable command ID
canonical English display label
short description
menu/group location, if any
parameter schema
result schema
availability predicate
unavailability reason
remote permission class
handler identity
whether invocation affects deterministic machine state
whether invocation is recordable/replayable
GUI parameter-acquisition policy, if parameters are missing
keyboard shortcut, if any
```

### 4.3 Stable IDs

Stable IDs are lowercase ASCII dotted identifiers.

Examples:

```text
file.open_run
snapshot.load
snapshot.save
host.screenshot.save
host.screenshot.temp
machine.pause
machine.resume
machine.reset
machine.model.set
machine.speed.set
media.tape.insert
media.tape.eject
media.tape.manager
media.microdrive.mount
media.microdrive.eject
tools.debugger.open
settings.open
help.about
application.quit
```

A stable command ID is an application API. Cosmetic GUI renaming does not
change it.

### 4.4 Parameter acquisition

The registry describes semantic parameters; front ends acquire those parameters
appropriately.

The frozen C foundation uses bounded caller-owned storage and the following
application-layer objects: immutable command metadata, a byte-oriented argument
view, and a tagged result containing a project result code, stable reason token,
and bounded message. Registration rejects malformed or duplicate IDs and null
handlers; finalization makes lookup/enumeration stable. Availability predicates
are read-only, and dispatch delegates validated arguments to an application
handler without acquiring GUI parameters or mutating deterministic state.

For example:

```text
GUI File > Open / Run...
    -> file chooser obtains PATH
    -> file.open_run(PATH)

Telnet
    -> DO file.open_run "C:\Spectrum\game.tzx"
    -> same file.open_run(PATH) command
```

The GUI file chooser is therefore presentation, not the implementation of
`file.open_run`.

If a front end cannot legally or safely acquire the required parameters, the
command remains visible but cannot be executed through that front end.

### 4.5 Serialization of state-changing commands

Commands that affect emulated machine state are serialized by the
application/orchestrator so that one emulation thread remains the owner of
mutable Spectrum state.

The UI thread, network thread, file-dialog callback, or test harness may enqueue
an application command. They do not mutate the machine directly.

### 4.6 Command equivalence requirement

For the same initial application state and identical semantic arguments:

```text
GUI action
Toolbar action
Telnet alias
Telnet DO <stable-id>
application test invocation
```

must reach the same semantic handler and produce the same machine-visible
result, subject only to host-only presentation differences.

### 4.7 Menu nodes and fixed-argument action bindings

The registry distinguishes semantic commands from menu-tree nodes. A selectable
menu action may bind fixed arguments to a parameterized semantic command.

Example:

```text
menu action ID:  machine.speed.400
label:           400%
command:         machine.speed.set
fixed argument:  400
```

Both of these generic invocations are therefore equivalent:

```text
DO machine.speed.400
DO machine.speed.set 400
```

The same rule applies to fixed model and tape-loading-mode choices. Menu action
IDs are stable registry IDs and may be invoked by `DO` when their underlying
command is permitted.

Dynamic GUI data such as Recent-file entries is not assigned globally stable
command IDs merely to expose host paths.

---

## 5. Remote permission classes

Every command has exactly one initial Telnet permission class.

```text
REMOTE_SAFE
HOST_READ
HOST_WRITE
MEDIA_DESTRUCTIVE
APPLICATION_CONTROL
LOCAL_ONLY
```

### 5.1 REMOTE_SAFE

Allowed by the initial unauthenticated Telnet service.

This class may affect the emulated machine but may not arbitrarily read or write
host files, destructively modify media images, terminate WZSN, or invoke local
OS facilities.

Examples:

```text
machine.pause
machine.resume
machine.reset
machine.model.set
machine.speed.set
machine.networking.set
host.screenshot.temp
status/query operations
menu/registry discovery
keyboard operations
safe debugger execution controls where explicitly registered
```

### 5.2 HOST_READ

Requires arbitrary host-file read access or host path traversal.

Examples include opening a media path supplied by Telnet.

Initial Telnet policy: **DENIED**.

### 5.3 HOST_WRITE

Writes a caller-selected host path.

Examples include Save Snapshot As or Save Screenshot As.

Initial Telnet policy: **DENIED**.

### 5.4 MEDIA_DESTRUCTIVE

Can alter or destroy mounted/offline media contents.

Examples include formatting a Microdrive, deleting tape blocks, deleting MDR
files, or overwriting a media image.

Initial Telnet policy: **DENIED**.

### 5.5 APPLICATION_CONTROL

Controls the host application rather than merely the emulated machine.

Examples include application quit or application restart.

Initial Telnet policy: **DENIED**.

### 5.6 LOCAL_ONLY

Presentation-only or local-OS actions that are not remotely executable in the
initial architecture.

Examples include opening a native file chooser or moving/resizing a host
window.

Initial Telnet policy: **DENIED** and not user-overridable without a later
security architecture change.

### 5.7 Discoverability despite denial

`MENU`, `MENU TREE`, `MENU FIND`, and `DESCRIBE` expose the entire registered
menu/command tree, including commands that are disabled by current state or
denied by Telnet policy.

Remote visibility is not remote permission.

---

## 6. Global application layout

The exact widget toolkit and visual styling remain Phase-12 implementation
decisions. The semantic composition is frozen.

The initial desktop application contains:

```text
application menu bar or platform-equivalent menu presentation
compact primary toolbar
Spectrum display viewport
optional/collapsible Machine & Media status panel
status indicators
manager/debugger/settings windows or panels opened on demand
```

The Spectrum display remains visually dominant.

On platforms that relocate standard commands, for example macOS moving About,
Settings, or Quit into platform-standard locations, the semantic command IDs and
canonical menu hierarchy remain unchanged even though native presentation may
move an item.

---

## 7. Canonical top-level menu tree

The initial semantic menu tree is:

```text
File
Machine
Media
View
Tools
Settings
Help
```

No `TapeFiles`, `Z80Snaps`, `MdriveFiles`, `DockFiles`, `DataBase`, `Setup`, or
`DOS` top-level menu is reproduced merely for historical appearance.

### 7.1 Canonical menu-node IDs

The top-level menu-node IDs are:

```text
file
machine
media
view
tools
settings
help
```

Required initial child-node/action IDs include:

```text
file.open_run
file.recent
snapshot.load
snapshot.save
snapshot.save_as
host.screenshot.save
application.quit

machine.model
machine.model.48k
machine.model.128k
machine.reset
machine.pause_resume
machine.speed
machine.speed.25
machine.speed.50
machine.speed.100
machine.speed.200
machine.speed.400
machine.speed.800
machine.speed.unlimited

media.tape
media.tape.insert
media.tape.eject
media.tape.loading_mode
media.tape.loading_mode.normal
media.tape.loading_mode.instant
media.tape.manager
media.microdrive
media.microdrive.drive1
media.microdrive.drive2
media.microdrive.drive3
media.microdrive.drive4
media.microdrive.drive5
media.microdrive.drive6
media.microdrive.drive7
media.microdrive.drive8
media.microdrive.manager
media.zx_printer
media.zx_printer.manager

view.fullscreen
view.machine_media_status
view.display_settings

tools.debugger
tools.diagnostics
tools.snapshot_inspector
tools.compatibility
tools.compatibility.tape
tools.compatibility.snapshot
tools.compatibility.spectrum_data
tools.compatibility.microdrive
tools.compatibility.database

settings.display
settings.audio
settings.input
settings.rom_firmware
settings.peripherals
settings.peripherals.networking
settings.peripherals.zx_printer
settings.peripherals.telnet

help.help
help.about
```

Required initial non-menu semantic command IDs used by fixed menu actions,
Telnet aliases, or manager controls include:

```text
host.screenshot.temp
machine.pause
machine.resume
machine.model.set
machine.speed.set
machine.networking.set
media.tape.loading_mode.set
media.microdrive.mount
media.microdrive.eject
media.microdrive.set_default
```

Menu action IDs that represent a fixed choice bind to the parameterized
semantic command. For example, `machine.model.48k` binds
`machine.model.set 48k`, and `media.tape.loading_mode.normal` binds
`media.tape.loading_mode.set normal`.

`machine.pause_resume` is a state-sensitive menu/toolbar action: while running
it invokes `machine.pause`; while paused it invokes `machine.resume`. Explicit
Telnet automation should prefer the non-toggle `PAUSE` and `RESUME` aliases.

`file.recent` is a dynamic local presentation group. Each local Recent entry
invokes the existing `file.open_run` semantic command with the locally stored
path as its argument; it does not create a new semantic file-opening
implementation. Current Recent host paths are not part of the remotely
discoverable registry payload.

### 7.2 Help menu

Canonical structure:

```text
Help
├─ Help
└─ About Warajevo ZX Spectrum Next
```

The stable action IDs are `help.help` and `help.about`. Platform-standard
relocation of About does not change those IDs.

---

## 8. File menu

Canonical semantic structure:

```text
File
├─ Open / Run...
├─ Recent
├─ Load Snapshot...
├─ Save Snapshot...
├─ Save Snapshot As...
├─ Save Screenshot...
└─ Quit
```

### 8.1 Open / Run

Stable command:

```text
file.open_run
```

This is the modern descendant of the classic Warajevo Start button.

The user selects a supported file without first choosing a media category.
WZSN identifies the format and performs the appropriate operation.

Initial native routing includes, where the corresponding subsystem is
implemented:

```text
TAP/TZX/WAV -> tape path
SNA/Z80     -> snapshot load
MDR         -> Microdrive mount workflow
```

Later machine/media formats become eligible only when their architecture and
compatibility status permit them.

A format that requires a preservation conversion rather than native execution
is routed to the appropriate Compatibility Tool or produces an explanatory
unsupported-format result. WZSN must not silently perform a lossy conversion
that changes machine-visible behavior.

### 8.2 Recent

`Recent` is host-only convenience metadata.

It must not be part of deterministic machine state. Missing recent files are
reported cleanly and may be removed from the recent list without affecting
emulation.

Because the initial Telnet service is unauthenticated, `MENU` discovery may
show that the `file.recent` group exists but must not enumerate recent host
paths or path-derived labels.

### 8.3 Snapshot commands

`Load Snapshot...`, `Save Snapshot...`, and `Save Snapshot As...` are direct
front ends to the snapshot subsystem described by the core/system architecture.

Snapshot load remains atomic: failure never leaves a partially mutated live
machine.

`Save Snapshot...` writes to the current snapshot destination when one exists;
if no current save destination exists, the GUI acquires one using the same
parameter-acquisition path as `Save Snapshot As...`. `Save Snapshot As...`
always acquires a new destination. Neither command makes a snapshot a mounted
medium.

### 8.4 Save Screenshot

Stable command:

```text
host.screenshot.save
```

The GUI obtains a destination path and writes PNG using the shared screenshot
capture service defined by the core/system architecture.

The saved image contains the Spectrum display raster only, using the selected
host-visible crop/border presentation, with no WZSN menus, toolbar, status UI,
debugger chrome, cursor, or desktop content.

### 8.5 Quit

Stable command:

```text
application.quit
```

This is host application control and is not remotely executable through the
initial unauthenticated Telnet service.

---

## 9. Machine menu

Canonical structure:

```text
Machine
├─ Model
│  ├─ ZX Spectrum 48K
│  └─ ZX Spectrum 128K
├─ Reset
├─ Pause / Resume
└─ Emulation Speed
   ├─ 25%
   ├─ 50%
   ├─ 100%
   ├─ 200%
   ├─ 400%
   ├─ 800%
   └─ Unlimited
```

Later certified machine profiles are added to `Model`; they do not receive
independent top-level menus.

### 9.1 Model

Stable command:

```text
machine.model.set <48k|128k>
```

Changing the model performs the defined application machine-change/reset
workflow. It may not hot-swap profile identity while preserving incompatible
machine state.

Unsupported/later models do not appear as selectable initial options.

### 9.2 Reset

Stable command:

```text
machine.reset
```

This performs an authentic emulated Spectrum reset according to the active
machine profile.

It does not:

- restart WZSN;
- disconnect Telnet;
- change emulation speed;
- change host display/audio/input preferences;
- implicitly eject mounted media unless authentic emulated hardware semantics
  require a state change;
- clear Telnet/local keyboard ownership beyond the reset policy frozen by the
  core/system architecture.

If invoked while application-paused, the reset is applied and the application
remains paused afterward. Reset is a deterministic machine-control operation
and is eligible for application/session recording using the same semantic
command regardless of front end.

### 9.3 Pause / Resume

Stable commands:

```text
machine.pause
machine.resume
```

Pause is an application execution state, not a Spectrum hardware signal.
Canonical master time does not advance while the machine is application-paused.

### 9.4 Emulation Speed

Stable command:

```text
machine.speed.set <25|50|100|200|400|800|unlimited>
```

The menu and toolbar expose exactly the runtime-speed values frozen by the
core/system architecture:

```text
25%
50%
100%
200%
400%
800%
Unlimited
```

The control changes host pacing, not Spectrum clock relationships.

The UI must visibly reflect the audio policy:

- host audio audible from 50% through 200% inclusive;
- host audio muted outside that range;
- emulated beeper/AY state continues regardless of host mute.

Changing speed while running must not reset the machine or introduce a
master-tick discontinuity.

---

## 10. Media menu

Canonical initial structure:

```text
Media
├─ Tape
│  ├─ Insert...
│  ├─ Eject
│  ├─ Loading Mode
│  │  ├─ Normal
│  │  └─ Instant / Trap
│  └─ Tape Manager...
├─ Microdrive
│  ├─ Drive 1...
│  ├─ Drive 2...
│  ├─ Drive 3...
│  ├─ Drive 4...
│  ├─ Drive 5...
│  ├─ Drive 6...
│  ├─ Drive 7...
│  ├─ Drive 8...
│  └─ Microdrive Manager...
└─ ZX Printer
   └─ Printer Manager...
```

A Dock Cartridge branch is added only when Timex/DCK support is promoted from
LATER to an implemented machine target.

---

## 11. Tape quick controls

### 11.1 Insert Tape

Stable command:

```text
media.tape.insert <path>
```

The GUI acquires the path with a file chooser. The toolbar Tape control may
expose the same action.

### 11.2 Eject Tape

Stable command:

```text
media.tape.eject
```

It is disabled when no tape is mounted and reports the reason
`no-tape-mounted` through the registry.

### 11.3 Loading Mode

Stable command:

```text
media.tape.loading_mode.set <normal|instant>
```

`Normal` is the default and must be shown as the authenticity-first mode.
`Instant / Trap` is explicitly optional and must not be presented as an
indistinguishable hidden optimization.

### 11.4 Toolbar Tape control

The toolbar contains a compact Tape control showing mounted/unmounted state.
Its menu provides at least:

```text
Insert...
Eject
Normal
Instant / Trap
Open Tape Manager...
```

---

## 12. Tape Manager

The Tape Manager owns detailed inspection and manipulation of tape media.

It is not a second emulator and does not duplicate the core tape engine.

### 12.1 Required presentation

For a mounted tape, the manager exposes:

```text
filename/path or source identity
format
loading mode
current block/position
playback/loading state
block list
block type
logical length
stored length where meaningful
flags/metadata where meaningful
selection state
```

### 12.2 Required block operations

The manager owns the modern equivalents of classic Warajevo's block functions:

```text
reorder blocks
extract selected blocks
add/import block
delete selected blocks
edit block data where format permits
copy selected blocks to a new tape
change position
```

Drag-and-drop may be used for reordering. The implementation must not reproduce
awkward historical list-box gestures merely for nostalgia.

### 12.3 Warajevo-native tape maintenance

Classic native-format operations such as exclude/linearize/implode/decompress
and compression-efficiency analysis are retained under an **Advanced Warajevo
Tape** or Compatibility Tools surface, visible only when applicable.

They are not shown as ordinary operations for standard TAP/TZX media.

### 12.4 Reports

Legacy Print-to-Screen is replaced by the manager view.

Legacy Print-to-Printer is dropped.

A non-destructive `Export Tape Report...` operation may produce a text or other
portable report if retained.

### 12.5 Failure behavior

Unsupported mutation, malformed blocks, read-only media, and save failures must
leave the original image unchanged unless an operation explicitly completed an
atomic replacement.

---

## 13. Snapshot workflow and Snapshot Inspector

A snapshot is a machine-state load/save artifact, not a continuously mounted
medium.

Therefore classic `Select Snapshot` / `Unselect Snapshot` state is not
reproduced.

### 13.1 Native workflow

The normal UI provides:

```text
Load Snapshot...
Save Snapshot...
Save Snapshot As...
Snapshot Inspector...
```

`Snapshot Inspector...` is reached through `Tools > Snapshot Inspector...` and
uses stable action ID `tools.snapshot_inspector`. It may inspect a selected
snapshot without making that file a mounted medium.

### 13.2 Snapshot Inspector

The inspector may display:

```text
format/version
machine profile implied by file
register summary
paging/port state
AY state where applicable
memory-page inventory
warnings/unsupported extensions
```

### 13.3 Editing snapshot state

Classic Warajevo exposed direct snapshot edits for processor registers,
hardware devices, and memory.

WZSN does not create a second independent implementation of those editors.
Direct register, hardware, and memory mutation belongs to the shared debugger
and machine-state inspection machinery.

An offline snapshot-inspection mode may reuse debugger/state-editor components,
but the semantics come from one implementation.

---

## 14. Microdrive quick controls

WZSN preserves eight Interface 1 Microdrive slots when the subsystem is
available.

Each slot has a semantic mounted/unmounted state.

Typical command IDs include:

```text
media.microdrive.mount <1..8> <path>
media.microdrive.eject <1..8>
media.microdrive.set_default <1..8>
```

The toolbar initially exposes a compact `MDV 1` control. The full eight-drive
state appears in the status panel and Microdrive Manager.

Mounting the same writable image in multiple drives simultaneously must be
prevented unless a later explicit safe-sharing policy is designed. The same
exclusive-writer rule applies across concurrently running WZSN processes: a
second process may not mount an image writable unless it obtains the required
interprocess write claim.

---

## 15. Microdrive Manager

The Microdrive Manager preserves one of classic Warajevo's strongest media
management features.

### 15.1 Drive overview

The manager displays all eight slots and, for each mounted cartridge:

```text
host image identity
logical cartridge name
sector count
write-protection state
current/default drive state
format/validation status
```

### 15.2 Whole-cartridge operations

The manager owns:

```text
mount/select
eject/unselect
set default drive
catalog
format
optimize/reorder sectors
view sector allocation
logical rename
write protect
write unprotect
```

Destructive operations require explicit confirmation and are classified
`MEDIA_DESTRUCTIVE` for Telnet.

### 15.3 Logical files

A file-oriented view owns:

```text
delete
rename
hide
unhide
copy to another mounted cartridge
```

### 15.4 Advanced sectors

An Advanced/Sectors view owns:

```text
verify/repair sector logical structure
edit sector data
edit whole sector including metadata/checksums where explicitly permitted
```

The UI must distinguish ordinary data editing from dangerous raw-sector editing.

### 15.5 Legacy MDR conversion

`Enlarge MDR to 254 sectors` is a Compatibility Tool, not a primary manager
command.

---

## 16. ZX Printer UI

The authentic ZX Printer peripheral remains part of the initial required
emulation scope.

Its host presentation belongs under:

```text
Media > ZX Printer > Printer Manager...
Settings > Peripherals > ZX Printer
```

The manager may expose captured virtual printer output and export functions.
Those functions consume the shared printer event stream and host-side bitmap
projection; they must not access BIOS/LPT devices or duplicate core printer
semantics.
Historical LPT/printer-port routing is not reproduced as a host requirement.

---

## 17. Later Dock Cartridge UI

DCK/Timex support is not an initial certified target.

No disabled DockFiles top-level menu is carried in the initial UI.

When Timex/DCK support becomes implemented, add:

```text
Media
└─ Dock Cartridge
   ├─ Insert...
   ├─ Eject
   └─ Dock Manager...
```

The Dock Manager then owns view and merge operations. ROM-to-DCK,
Binary-to-LROS, and Binary-to-AROS remain Compatibility Tools.

---

## 18. View menu

Canonical structure:

```text
View
├─ Fullscreen
├─ Machine / Media Status
└─ Display Settings...
```

### 18.1 Fullscreen

Fullscreen is host presentation only. Entering or leaving fullscreen must not
alter canonical machine state or emulated time.

### 18.2 Machine / Media Status

This toggles the compact status panel described in Section 25.

### 18.3 Display Settings

This is a shortcut to the display settings category. It does not duplicate a
second configuration store.

---

## 19. Tools menu

Canonical structure:

```text
Tools
├─ Debugger / Monitor...
├─ Diagnostics...
├─ Snapshot Inspector...
└─ Compatibility Tools...
   ├─ Tape Converter...
   ├─ Snapshot Converter...
   ├─ Spectrum Data Converter...
   ├─ Microdrive Tools...
   └─ Legacy Database Converter...
```

Compatibility entries may remain hidden or marked later/unimplemented until the
underlying retained utility is implemented. Required emulation features may not
be hidden behind Compatibility Tools merely to avoid implementing them.

---

## 20. Complete legacy-menu disposition

This section is the item-by-item migration authority for the classic Warajevo
integrated-environment UI.

### 20.1 TapeFiles > Tapes

| Legacy item | WZSN destination | Initial disposition |
|---|---|---|
| Select | Toolbar / quick action | `Insert Tape...` |
| Unselect | Toolbar / quick action | `Eject Tape` |
| Parameters | Media-manager function | Split among Tape Manager position/properties and visible Normal/Instant loading mode |
| View | Media-manager function | Tape block browser |
| Print > Screen | Dropped legacy host function | Replaced by Tape Manager view |
| Print > Printer | Dropped legacy host function | Not reproduced |
| Print > File | Media-manager function | Optional `Export Tape Report...` |
| Copy to New | Media-manager function | Copy selected blocks into a new image |

### 20.2 TapeFiles > Blocks

| Legacy item | WZSN destination | Initial disposition |
|---|---|---|
| Reorder | Media-manager function | Retain |
| Extract | Media-manager function | Retain |
| Delete | Media-manager function | Retain |
| Add | Media-manager function | Retain |
| Edit | Media-manager function | Retain where format permits |
| Exclude | Compatibility utility | Warajevo-native tape advanced operation |
| Change Position | Media-manager function | Retain; drag/drop or explicit move |

### 20.3 TapeFiles > Implode

| Legacy item | WZSN destination | Initial disposition |
|---|---|---|
| Compress > All blocks | Compatibility utility | Retain with native Warajevo tape support |
| Compress > Linearize only | Compatibility utility | Retain with native Warajevo tape support |
| Compress > Compress Selected | Compatibility utility | Retain with native Warajevo tape support |
| Decompress | Compatibility utility | Retain with native Warajevo tape support |
| Efficiency | Compatibility utility | Retain as analysis/maintenance information |

### 20.4 TapeFiles > Communications > RS232

| Legacy item | WZSN destination | Initial disposition |
|---|---|---|
| Send to Spectrum | Dropped legacy host function | Old PC-to-real-Spectrum bridge, not Interface 1 emulation |
| Receive from Spectrum | Dropped legacy host function | Old PC-to-real-Spectrum bridge |
| Send communication program | Dropped legacy host function | DOS-era transfer helper |
| Configure RS232 | Dropped legacy host function | Old host serial-port configuration |

Authentic Interface 1 serial state remains a machine/peripheral concern. A
future real-hardware bridge would require its own architecture.

### 20.5 TapeFiles > Communications > Cassette

| Legacy item | WZSN destination | Initial disposition |
|---|---|---|
| Receive from cassette | Dropped legacy host function initially | Live physical capture is LATER |
| Sample from cassette | Dropped legacy host function initially | Live physical capture is LATER |
| Advanced setup | Dropped legacy host function initially | Old DOS timing/device calibration not reproduced |

If live physical capture is later implemented, it becomes a new dedicated Tape
Capture workflow rather than resurrecting the old DOS dialog.

### 20.6 TapeFiles > Convert > source-language/text conversions

All of the following are **Compatibility utilities**, not main-menu items:

| Legacy conversion |
|---|
| ASCII -> BASIC |
| BASIC -> ASCII |
| ASCII -> HiSoft GENS |
| HiSoft GENS -> ASCII |
| ASCII -> HiSoft C |
| HiSoft C -> ASCII |
| ASCII -> HiSoft Pascal |
| HiSoft Pascal -> ASCII |
| ASCII -> Tassword 2 |
| Tassword 2 -> ASCII |
| ASCII -> Tassword 3 |
| Tassword 3 -> ASCII |
| ASCII -> The Last Word |
| The Last Word -> ASCII |
| ASCII -> Machine Lightning |
| Machine Lightning -> ASCII |
| ASCII -> Abersoft Forth |
| Abersoft Forth -> ASCII |
| ASCII -> Sinclair Logo |
| Sinclair Logo -> ASCII |

### 20.7 TapeFiles > Convert > SCREEN$

| Legacy item | WZSN destination | Initial disposition |
|---|---|---|
| SCREEN$ -> color TIFF | Compatibility utility | Preserve only as legacy conversion; ordinary screenshots use PNG |
| SCREEN$ -> black/white TIFF | Compatibility utility | Preserve only as legacy conversion |

### 20.8 TapeFiles > Convert > other-emulator/media formats

Every operation below is a **Compatibility utility**. Native runtime support for
a format is separate from explicit conversion.

| Legacy conversion |
|---|
| Roman & Easy -> Warajevo |
| Warajevo -> Roman & Easy |
| Lunter TAP -> Warajevo TAP |
| Warajevo TAP -> Lunter TAP |
| Irish SpecEm -> Warajevo TAP |
| Warajevo TAP -> Irish SpecEm |
| Polish SP/SPC -> Warajevo TAP |
| Warajevo TAP -> Polish SP/SPC |
| Spectrum 2.00 BLK -> Warajevo TAP |
| Warajevo TAP -> BLK |
| ZX Garabik LTP -> Warajevo TAP |
| Warajevo TAP -> LTP |
| ZX Brukner -> Warajevo TAP |
| Warajevo TAP -> ZX Brukner |
| TZX -> Warajevo TAP |
| Warajevo TAP -> TZX |
| TZX conversion setup |
| ZX Museum ZXS -> Warajevo |
| Warajevo TAP -> ZX Museum ZXS |
| TR-DOS TRD -> Warajevo TAP |
| Warajevo TAP -> TRD |
| ZX32 ZXT/ZXS -> Warajevo TAP |
| Warajevo TAP -> ZX32 ZXT/ZXS |
| VOC -> Warajevo TAP |
| Warajevo TAP -> VOC |

### 20.9 Z80Snaps > Snaps

| Legacy item | WZSN destination | Initial disposition |
|---|---|---|
| Select | Toolbar / quick action | `Load Snapshot...` |
| Unselect | Dropped legacy host function | Snapshot is not persistent mounted media |
| Rename | Dropped legacy host function | Replaced by `Save Snapshot As...` |
| Info | Media-manager function | Snapshot Inspector |

### 20.10 Z80Snaps > Edit

| Legacy item | WZSN destination | Initial disposition |
|---|---|---|
| Processor registers | Debugger tool | Shared register editor |
| Hardware devices | Debugger tool | Shared hardware/peripheral state inspector/editor |
| Memory | Debugger tool | Shared memory inspector/editor |

### 20.11 Z80Snaps > Convert

Every explicit conversion below is a **Compatibility utility**:

| Legacy conversion |
|---|
| Spanish SPECTRUM SP -> Z80 |
| Z80 -> Spanish SPECTRUM SP |
| old VGASPEC SP -> Z80 |
| Z80 -> old VGASPEC SP |
| Irish SpecEm PRG -> Z80 |
| Z80 -> Irish SpecEm PRG |
| JPP SNA -> Z80 |
| Z80 -> JPP SNA |
| SpecEmu-G SEM -> Z80 |
| Z80 -> SpecEmu-G SEM |
| SP_UKV SNA 128 -> Z80 |
| Z80 -> SNA 128 |
| Nuclear ZX SNP -> Z80 |
| Z80 -> Nuclear ZX SNP |
| X128 SLT -> Z80 + TAP |
| Z80 + TAP -> X128 SLT |
| Spectrum 2.00 SIT -> Z80 |
| Z80 -> Spectrum 2.00 SIT |
| ZX32 ZXS RIFF -> Z80 |
| Z80 -> ZX32 ZXS RIFF |
| Any Z80 -> 48K without Interface 1 |
| Any Z80 -> 128K without Interface 1 |
| Z80 snapshot -> TAP |

SNA and Z80 are native initial snapshot formats. Loading them does not require
the Compatibility Tool.

### 20.12 MdriveFiles > Microdrive

| Legacy item | WZSN destination | Initial disposition |
|---|---|---|
| Select | Toolbar / quick action | Mount cartridge in selected drive |
| Unselect | Toolbar / quick action | Eject cartridge |
| Set default drive (legacy `Default`) | Media-manager function | Set current/default Microdrive |
| Catalog | Media-manager function | Logical-file view |
| Format | Media-manager function | Retain; destructive confirmation required |
| Optimize | Media-manager function | Retain |
| View | Media-manager function | Sector/allocation view |
| Rename | Media-manager function | Logical cartridge rename |
| Write protect | Media-manager function | Retain |
| Write unprotect | Media-manager function | Retain |

### 20.13 MdriveFiles > Sectors

| Legacy item | WZSN destination | Initial disposition |
|---|---|---|
| Verify | Media-manager function | Advanced/Sectors |
| Edit | Media-manager function | Advanced/Sectors; dangerous raw mode explicit |

### 20.14 MdriveFiles > Files

| Legacy item | WZSN destination | Initial disposition |
|---|---|---|
| Delete | Media-manager function | Retain |
| Rename | Media-manager function | Retain |
| Hide | Media-manager function | Retain |
| Unhide | Media-manager function | Retain |
| Copy | Media-manager function | Retain |

### 20.15 MdriveFiles > Convert

| Legacy item | WZSN destination | Initial disposition |
|---|---|---|
| Enlarge MDR to 254 sectors | Compatibility utility | Retain for legacy interoperability |

### 20.16 DockFiles

The entire branch is absent from the initial UI because Timex/DCK is a later
machine target.

| Legacy item | WZSN destination when Timex exists |
|---|---|
| Select Dock | Toolbar / quick action |
| Unselect Dock | Toolbar / quick action |
| View Dock | Media-manager function |
| Merge Dock | Media-manager function |
| ROM -> DCK | Compatibility utility |
| Binary -> LROS | Compatibility utility |
| Binary -> AROS | Compatibility utility |

### 20.17 DataBase

The historical database application itself is **not** recreated in the initial
WZSN UI.

| Legacy item | WZSN destination | Initial disposition |
|---|---|---|
| Select DB directory | Dropped legacy host function | Drop |
| Unselect DB directory | Dropped legacy host function | Drop |
| Run | Dropped legacy host function | Goal replaced by `Open / Run...`; future Library may supersede |
| Edit/Browse | Dropped legacy host function | Drop initial legacy database UI |
| Sort | Dropped legacy host function | Drop |
| Mark sort priority > Conditional | Dropped legacy host function | Drop |
| Mark sort priority > Swap marker | Dropped legacy host function | Drop |
| Mark sort priority > Unmark all | Dropped legacy host function | Drop |
| Report | Dropped legacy host function | Drop |

A future WZSN Library, if designed, is a new modern feature rather than a direct
port of the historical database schema.

### 20.18 DataBase > Convert

These remain low-priority **Compatibility utilities**:

| Legacy conversion |
|---|
| Warajevo -> SpecPic |
| SpecPic -> Warajevo |
| Warajevo -> SGD |
| SGD -> Warajevo |
| Warajevo -> ZX Rainbow |
| ZX Rainbow -> Warajevo |
| Warajevo -> SpecBase |
| SpecBase -> Warajevo |

### 20.19 Setup

| Legacy item | WZSN destination | Initial disposition |
|---|---|---|
| Emulator | Main menu | `Machine > Model` |
| Video | Settings | `Settings > Display` |
| Speed | Toolbar / quick action + main menu | `Machine > Emulation Speed` |
| Test | Debugger tool | Split into relevant diagnostics/test infrastructure |
| Network | Settings | `Settings > Peripherals > Networking` when `Interface-1` is selected |
| Sound | Settings | `Settings > Audio` |
| Interface 1 | Settings | `Settings > Peripherals > Networking` radio choice `Interface-1` |
| Joystick | Settings | `Settings > Input > Joystick` |
| Printer | Settings | `Settings > Peripherals > ZX Printer` |
| ROM | Settings / Machine | ROM/firmware selection in Settings; machine model not hidden here |
| MS Windows | Dropped legacy host function | DOS/V86 compatibility machinery removed |

Historical `Fast BASIC` and inverse-cursor ROM patch controls are not normal
initial WZSN settings. Accuracy-first machine execution does not modify ROM
behavior for convenience unless a later compatibility feature is explicitly
designed.

Historical PC-speaker/AdLib/SoundBlaster selection, VGA/CGA/Hercules selection,
and hand-tuned ULA delay controls are host-era implementation compensations and
are not reproduced as emulated-hardware controls.

### 20.20 DOS

| Legacy item | WZSN destination | Initial disposition |
|---|---|---|
| About | Main menu | `Help > About Warajevo ZX Spectrum Next` |
| Directory | Dropped legacy host function | Native file dialogs replace |
| Change directory | Dropped legacy host function | Native file dialogs replace |
| OS Shell | Dropped legacy host function | Do not embed a shell launcher |
| Exit | Main menu | `File > Quit` |

### 20.21 Legacy Start/status/window controls

| Legacy facility | WZSN destination | Initial disposition |
|---|---|---|
| Start button | Toolbar / quick action | `Open / Run...` |
| F10 Emulator | Toolbar / quick action | Run/Resume semantics; no second emulator executable |
| F1 Help | Main menu/shortcut | `Help` |
| Alt+F4 Exit | Native shortcut/main menu | `File > Quit` |
| Ctrl+F4 Close window | Native window behavior | No emulator semantics |
| Alt+F5 Zoom | Native/fullscreen/view behavior | Replace |
| Ctrl+F5 Resize | Native window behavior | Replace |
| Heap/free-RAM indicator | Dropped legacy host function | Not user machine state |
| Names window | Status panel | Replace with Machine/Media Status |
| Generated command parameters | Dropped legacy host function | No DOS child-emulator command line |
| Emulator executable filename | Dropped legacy host function | One integrated application |
| Temporary directory display | Dropped legacy host function | Host implementation detail |

---

## 21. Debugger / Monitor

The debugger is a first-class tool, not a second emulation implementation.

Initial presentation should support the required monitor/debugger workflows
when the core subsystem reaches Phase 11.

Conceptual surfaces include:

```text
execution control
registers
flags
interrupt state
disassembly
memory view/editor
breakpoints/watchpoints
I/O and peripheral state
paging state
ULA/raster position diagnostics
AY state
keyboard/joystick state
trace/log views
```

Classic snapshot register/hardware/memory editing reuses these components.

Legacy diagnostic concepts such as detecting use of 128K ports from 48K
software belong under Diagnostics rather than a generic Settings/Test dialog.

Automated quit-after-N-seconds behavior belongs in the headless/application test
infrastructure, not the normal UI.

---

## 22. Compatibility Tools

Compatibility Tools preserve historical Warajevo conversion value without
polluting ordinary emulator navigation.

The initial category structure is:

```text
Compatibility Tools
├─ Tape Converter
├─ Snapshot Converter
├─ Spectrum Data Converter
├─ Microdrive Tools
└─ Legacy Database Converter
```

### 22.1 Availability

A tool may be implemented later according to the core architecture's legacy
feature disposition. An unavailable later tool must not masquerade as a
working conversion.

### 22.2 Native load versus conversion

If WZSN natively supports a format for emulation, `Open / Run...` should load it
natively. Explicit conversion remains a separate preservation operation.

### 22.3 Loss disclosure

A conversion known to discard unsupported hardware state, metadata, timing, or
format capabilities must state that before writing the output.

### 22.4 Preservation provenance

Compatibility utilities should expose source format, destination format, and
conversion warnings clearly enough that generated regression artifacts can be
reproduced and understood.

---

## 23. Settings architecture

Canonical Settings categories are:

```text
Settings
├─ Display
├─ Audio
├─ Input
├─ ROM / Firmware
└─ Peripherals
   ├─ Networking
   ├─ ZX Printer
   └─ Telnet Keyboard & Remote Control
```

The exact visual control arrangement is toolkit-dependent. Semantic settings
and their machine/host ownership are not.

### 23.1 Display

Display settings control host presentation only, for example scaling, crop,
fullscreen preference, and later optional presentation effects.

They may not alter ULA timing, contention, FLASH timing, or the canonical
raster.

### 23.2 Audio

Audio settings control host output/presentation policy. Emulated beeper and AY
state remain machine truth.

DOS-era PC speaker, AdLib, and Sound Blaster implementation choices are not
reproduced as emulated hardware settings.

### 23.3 Input

Input settings own host-to-normalized mapping, including Kempston joystick host
mapping.

They may not put host key identities into the Spectrum core.

### 23.4 ROM / Firmware

ROM selection/identity is explicit and must respect the core architecture's ROM
hash/licensing policy.

Machine selection is not hidden inside ROM settings.

### 23.5 Networking

Networking mode is one radio-button group backed by the single semantic command:

```text
machine.networking.set <none|interface1|ear_mic>
```

The visible choices are exactly:

```text
Networking
    ( ) None
    ( ) Interface-1
    ( ) Ear+Mic
```

Exactly one choice is selected. The UI must not present independent Interface-1
and Ear+Mic checkboxes or any other interaction that can make both active.

The meanings are owned by Section 24 of the core/system architecture:

- **None**: no Interface 1 and no routed MIC/EAR network;
- **Interface-1**: authentic Interface 1, Microdrive, and original ZX Net are
  available; Ear+Mic is inactive;
- **Ear+Mic**: Interface 1, Microdrive, and original ZX Net are unavailable;
  Architecture #3's virtual MIC/EAR attachment is used when implemented.

`Ear+Mic` never implies networking-ROM paging. It uses the distributed
high-memory software stack defined by Architecture #3. Selecting the radio
button does not inject that stack into RAM; the Spectrum must still load/install
it through the Architecture-#3 tape/bootstrap workflow.

A networking-mode change invokes the core's **cold machine-reconfiguration**
transition. It discards the current Spectrum RAM/device context and constructs a
fresh context with the same machine model and selected networking mode; this is
not the ordinary `Reset` command. If the application is paused, it remains
paused after the transition. The UI must clearly warn that changing networking
mode loses current Spectrum RAM state.

Leaving `Interface-1` while writable Microdrive media is dirty requires local
resolution: dirty media must be successfully committed or the transition is
cancelled. The unauthenticated Telnet `DO machine.networking.set ...` path is
`REMOTE_SAFE` only when no host write/confirmation is required; otherwise the
registry marks it unavailable with a reason such as
`dirty-media-requires-local-resolution`. Successful departure from
`Interface-1` logically detaches the Microdrive slots from the new machine
context.

`Ear+Mic` may remain disabled with a registry-provided reason until Architecture
#3 has passed its integration gate; it must not pretend to work before then. It
is also unavailable unless the active machine is the Architecture-#3-certified
ZX Spectrum 48K Issue-2 profile/variant. On 128K or another uncertified variant,
the availability predicate returns a reason such as `requires-48k-issue2` and
leaves the current machine unchanged.

When `Interface-1` is selected, the Networking page may expose subordinate
Interface-1/ZX-Net settings justified by the frozen peripheral architecture.
The UI does not automatically expose historical host file-lock timing knobs
such as PSWait/FreeNet/BusyWait merely because old Warajevo had them.

When `Ear+Mic` is selected, the ordinary Tape transport controls are disabled
while Architecture #3 owns the cassette EAR/MIC signals, with a reason such as
`cassette-owned-by-ear-mic-network`. Once Architecture #3 is implemented, its
Networking surface exposes the downstream **Bootstrap Ear+Mic Stack** action
that enters the router's `BOOTSTRAP_TAPE` mode and emits the distributed stack
as an authentic tape waveform. That action must not inject RAM or page a ROM.

### 23.6 ZX Printer

Configures the virtual printer presentation/output policy, not DOS LPT ports.

### 23.7 Telnet Keyboard & Remote Control

This page shows at minimum:

```text
service enabled-by-architecture state
base Control Port 30740
probe range 30740-32787
selected Control Port <number|unavailable>
IPv4 listener UP/DOWN
IPv6 listener UP/DOWN
normal/degraded/unavailable listener state
active client ACTIVE/NONE
plaintext/no-authentication warning
initial remote permission policy summary
```

The selected Control Port is session state chosen independently by each WZSN
process through the core Section-55.2 first-free probing contract. It is not a
persisted preference and the initial UI has no manual port override.

The listener starts automatically as defined by the core/system architecture.
The initial UI does not pretend that plaintext/no-authentication is secure.

---

## 24. Toolbar architecture

The initial toolbar is deliberately compact.

Canonical semantic controls:

```text
[Open/Run]
[Pause/Resume]
[Reset]
[Emulation Speed: 100% v]
[Tape v]
[Load Snapshot]
[Save Snapshot]
[MDV 1 v]
[Screenshot]
[Fullscreen]
[Debugger]
```

A toolkit may render text, icons, or both. The semantic set and shared command
handlers remain the same.

### 24.1 Emulation Speed toolbar control

The selected value is always visible.

Changing it invokes `machine.speed.set` and immediately updates status,
including audio-muted indication when outside 50%-200%.

### 24.2 Screenshot toolbar control

The toolbar Screenshot action invokes the GUI save-screenshot workflow. It does
not use the Telnet temporary-path naming rule unless a later explicit quick-save
mode is added.

### 24.3 Toolbar state

Controls use the same command-registry availability predicates as menus and
Telnet. The toolbar may not independently decide that a command is enabled.

---

## 25. Machine / Media status panel

The legacy Names window is replaced by a compact, optional status panel.

Conceptual example:

```text
ZX Spectrum 48K
100%     Audio On

Tape
  Jet Set Willy.tzx
  Normal

Microdrive
  1: utilities.mdr
  2: -
  3: -
  4: -
  5: -
  6: -
  7: -
  8: -

Networking
  Interface-1

Remote Control
  Control Port: 30741
  IPv4 UP / IPv6 UP
  No client
```

The panel must not expose obsolete implementation trivia such as:

```text
separate emulator EXE filename
generated DOS command options
host heap counter
DOS temporary directory as routine user state
```

### 25.1 Status-line indicators

Even when the full panel is hidden, the main window must make these conditions
readily visible:

```text
active machine model
emulation speed
pause/running state
host audio on/muted-by-speed state
mounted tape state
primary Microdrive state
networking mode
Control Port: <number|unavailable>
Telnet listener/client state
```

The exact text `Control Port: xxxxx` is always present in the status line when a
listener is available, using the selected five-digit decimal port. If the full
2048-port range is exhausted, the same field reads `Control Port: unavailable`.
The Control Port/Telnet indicator is interactive and opens the Telnet
settings/status page.

---

## 26. Telnet console role

The Telnet service is both:

1. a hardware-level remote Spectrum keyboard source; and
2. a remote application-control console backed by the shared command registry.

The transport, one-client rule, wildcard binding, Telnet IAC handling,
threading, and keyboard-matrix injection rules are owned by Section 55 of the
core/system architecture.

This document owns the application command grammar and remote-control
projection.

No Telnet control command may write directly into Spectrum RAM, call ROM
routines, or bypass the application/orchestrator.

---

## 27. Initial Telnet application grammar

The complete initial user-visible command families are:

```text
HELP
STATUS

KEY DOWN <key>
KEY UP <key>
KEY PRESS <key>
RELEASE ALL

PAUSE
RESUME
RESET

MODEL 48K
MODEL 128K

SPEED 25
SPEED 50
SPEED 100
SPEED 200
SPEED 400
SPEED 800
SPEED UNLIMITED

SCREENSHOT

MENU
MENU TREE
MENU <menu-or-command-id>
MENU FIND <text>
DESCRIBE <command-id>
DO <command-id> [arguments]
```

Command words, command IDs, model tokens, speed tokens, and Spectrum key names
are ASCII case-insensitive on input. Responses use canonical uppercase command
words and canonical lowercase command IDs where IDs are shown.

The keyboard key vocabulary remains exactly the physical 40-key vocabulary
frozen by the core/system architecture.

### 27.1 Keyboard-command responses

Successful keyboard state commands return:

```text
OK
```

This applies to successful `KEY DOWN`, `KEY UP`, `KEY PRESS`, and `RELEASE ALL`.

An unknown physical key returns:

```text
ERR BAD_KEY
```

A `KEY PRESS` rejected because that Telnet key is already held or already has a
pending automatic release returns:

```text
ERR BAD_STATE
```

These response rules do not alter the deterministic keyboard semantics owned by
the core/system architecture.

---

## 28. Telnet line grammar and bounded parsing

### 28.1 Input encoding

Initial application command lines are ASCII. Control bytes other than Telnet
framing and line termination are rejected.

Both `CRLF` and bare `LF` terminate a command.

### 28.2 Maximum line length

A decoded application line is limited to **1024 bytes**, excluding Telnet IAC
negotiation bytes and line termination.

An overlong line returns:

```text
ERR LINE_TOO_LONG
```

The parser discards input through the next line terminator and then resumes
normal parsing without altering machine/application state.

### 28.3 Tokens

Unquoted tokens are separated by ASCII spaces or tabs.

Double-quoted arguments are supported for generic `DO` string parameters.
Within a quoted argument the initial escapes are:

```text
\\   literal backslash
\"   literal double quote
```

No shell expansion, environment expansion, wildcard expansion, command
substitution, or OS-shell interpretation occurs.

### 28.4 Stable command IDs

`DO`, `DESCRIBE`, and explicit `MENU` command references accept canonical
registry IDs composed from lowercase letters, digits, dot, underscore, and
hyphen. Input matching is case-insensitive; output uses canonical lowercase.

---

## 29. Telnet HELP and STATUS

### 29.1 HELP

`HELP` returns the special-command grammar and points the client to `MENU TREE`
and `DESCRIBE` for the complete registry.

The response is multi-line and terminates with:

```text
END
```

### 29.2 STATUS

`STATUS` is `REMOTE_SAFE` and has no side effects.

Its required initial prefix is:

```text
STATUS PROTOCOL=1 CONTROL_PORT=<30740..32787> IPV4=<UP|DOWN> IPV6=<UP|DOWN> CLIENT=<ACTIVE|NONE> MODEL=<48K|128K> STATE=<RUNNING|PAUSED> SPEED=<25|50|100|200|400|800|UNLIMITED> AUDIO=<ON|MUTED|UNAVAILABLE> NETWORKING=<NONE|INTERFACE1|EAR_MIC>
```

Additional trailing `name=value` fields may be added compatibly, for example
mounted-media summaries. Existing names and meanings may not silently change.

`AUDIO=MUTED` includes speed-policy mute. `AUDIO=UNAVAILABLE` indicates that no
usable host audio output is currently available. More detailed mute/error
reasons may appear in optional trailing fields.

Optional status fields exposed by the initial unauthenticated protocol must not
contain arbitrary absolute host media paths. Mounted-media status may expose
media type, slot, or a sanitized basename when explicitly approved, but not a
full host directory path.

---

## 30. Telnet RESET

`RESET` is a dedicated first-class alias for:

```text
DO machine.reset
```

It is `REMOTE_SAFE`.

On success:

```text
OK RESET
```

`RESET`:

- resets the emulated Spectrum, not WZSN;
- preserves the Telnet connection;
- preserves selected emulation speed;
- preserves host configuration;
- follows the mounted-media and held-key reset semantics frozen by the
  core/system architecture;
- is valid while running or paused;
- reaches the same machine-reset handler as the GUI menu and toolbar.

No `RESET HARD` or second reset meaning exists in the initial protocol.

---

## 31. Telnet PAUSE and RESUME

Aliases:

```text
PAUSE  -> DO machine.pause
RESUME -> DO machine.resume
```

Both are `REMOTE_SAFE`.

Successful responses:

```text
OK PAUSE
OK RESUME
```

They are idempotent: pausing an already paused machine and resuming an already
running machine succeed without introducing an additional state transition.

---

## 32. Telnet MODEL

Aliases:

```text
MODEL 48K  -> DO machine.model.set 48k
MODEL 128K -> DO machine.model.set 128k
```

Both are `REMOTE_SAFE` for the initial certified profiles.

A model change uses the same defined reset/model-switch workflow as the GUI.
If invoked while application-paused, the newly selected/reset machine remains
paused until an explicit Resume.

Successful responses:

```text
OK MODEL 48K
OK MODEL 128K
```

Unsupported values return:

```text
ERR BAD_MODEL
```

---

## 33. Telnet SPEED

Aliases:

```text
SPEED 25
SPEED 50
SPEED 100
SPEED 200
SPEED 400
SPEED 800
SPEED UNLIMITED
```

Each maps to `machine.speed.set` and is `REMOTE_SAFE`.

Successful response:

```text
OK SPEED <25|50|100|200|400|800|UNLIMITED>
```

Invalid values return:

```text
ERR BAD_SPEED
```

Changing speed over Telnet has exactly the same pacing, tape wall-clock, and
audio-mute semantics as changing the toolbar or Machine menu control.

---

## 34. Telnet SCREENSHOT

### 34.1 Command

```text
SCREENSHOT
```

Alias target:

```text
host.screenshot.temp
```

Permission class:

```text
REMOTE_SAFE
```

### 34.2 Captured image

The command captures the Spectrum display only, according to the screenshot
capture boundary in the core/system architecture.

It does not include WZSN menus, toolbar, status indicators, debugger windows,
mouse pointer, or desktop content.

### 34.3 File format

Output format is exactly:

```text
PNG
```

### 34.4 Destination

The file is written to the host operating system's temporary directory.

The client cannot supply an arbitrary destination path to the initial
`SCREENSHOT` alias.

### 34.5 Filename

The base filename is:

```text
ZX-Screen-YYYYMMDDHHMMSSmmm.png
```

where the timestamp is host-local wall-clock date/time at naming time and
`mmm` is milliseconds.

Example:

```text
ZX-Screen-20260809102243731.png
```

If the target already exists, WZSN must not overwrite it. Filename creation
must use an exclusive-create/no-clobber host operation so that a race cannot
overwrite an existing file. It appends a numeric suffix before `.png`:

```text
ZX-Screen-20260809102243731-1.png
ZX-Screen-20260809102243731-2.png
```

and so on until an unused name is found.

### 34.6 Timing

The request is serialized to the application owner path. It captures the most
recently completed eligible presentation raster at the next safe capture
boundary.

If the machine is paused, the currently displayed completed raster is saved.

Taking a screenshot does not advance canonical master time and is not recorded
as deterministic machine input.

### 34.7 Success response

Only after the PNG has been successfully encoded, closed, and made available at
the reported path does the server send:

```text
OK SCREENSHOT <absolute-host-path>
```

The path occupies the remainder of the response line and is not shell-escaped.

Example:

```text
OK SCREENSHOT C:\Users\Example\AppData\Local\Temp\ZX-Screen-20260809102243731.png
```

### 34.8 Failure response

Failure returns:

```text
ERR SCREENSHOT <reason>
```

The reason is a short ASCII machine-readable token, for example:

```text
no-raster
cannot-create-file
encode-failed
write-failed
```

A failed screenshot does not alter Spectrum state.

---

## 35. Telnet MENU projection

The entire semantic menu/command registry is discoverable over Telnet.

### 35.1 MENU

```text
MENU
```

returns the canonical top-level menu IDs and labels, followed by `END`.

### 35.2 MENU TREE

```text
MENU TREE
```

returns the complete registered menu/command tree, including:

```text
stable command ID
canonical label
enabled/disabled state
unavailability reason when disabled
remote permission class
remote executable/denied state
```

Commands denied by remote policy remain visible.

Each returned tree/action record uses the initial line form:

```text
ITEM <id> PARENT=<id|ROOT> TYPE=<MENU|ACTION|COMMAND> STATE=<ENABLED|DISABLED> REMOTE=<ALLOWED|DENIED> CLASS=<permission-class> LABEL="<ASCII-label>" [REASON=<token>] [COMMAND=<command-id>] [ARGS="<fixed-args>"]
```

The response terminates with `END`. Dynamic host-sensitive values such as
Recent-file absolute paths are not emitted in these records.

### 35.3 MENU <id>

```text
MENU machine
MENU media.tape
MENU machine.speed.set
```

returns the referenced node and its immediate children or command metadata.
Unknown IDs return `ERR BAD_COMMAND_ID`.

### 35.4 MENU FIND

```text
MENU FIND <text>
```

performs a case-insensitive search over stable IDs, canonical labels, and short
descriptions. It is discovery only and has no side effects.

### 35.5 State equivalence

The enabled/disabled state reported through Telnet must come from the same
registry predicate used by the GUI.

Example with no tape mounted:

```text
ENABLED  media.tape.insert  REMOTE=DENIED  Insert Tape
DISABLED media.tape.eject   REMOTE=SAFE    Eject Tape  REASON=no-tape-mounted
```

The example shows that GUI availability and Telnet security are independent
axes.

---

## 36. Telnet DESCRIBE

```text
DESCRIBE <command-id>
```

returns machine-readable/human-readable metadata for one command, including at
least:

```text
ID
LABEL
DESCRIPTION
PARAMETERS
ENABLED
DISABLED_REASON
REMOTE_CLASS
REMOTE_ALLOWED
AFFECTS_MACHINE_STATE
RECORDABLE
```

The response terminates with `END`.

`DESCRIBE` describes schemas and command metadata. It must not include current
absolute host paths, recent-file history, or other command arguments containing
private host data.

`DESCRIBE` is `REMOTE_SAFE` and has no side effects.

---

## 37. Telnet DO

Generic invocation syntax:

```text
DO <command-id> [arguments]
```

`DO` resolves the stable ID in the shared command registry and validates
arguments against the command's parameter schema.

### 37.1 Permission evaluation

Evaluation order is:

```text
parse command line
resolve command ID
validate argument syntax
query current enabled/disabled state
query Telnet permission class/policy
enqueue semantic command if allowed
return result
```

No handler runs before validation and permission checks succeed.

### 37.2 Disabled command

A state-disabled command returns:

```text
ERR BAD_STATE <command-id> <reason>
```

### 37.3 Denied command

A valid command denied by Telnet policy returns:

```text
DENIED <command-id> <remote-permission-class>
```

No file chooser, confirmation dialog, or hidden GUI interaction is opened on
behalf of a denied remote command.

### 37.4 Success

Generic successful invocation returns at least:

```text
OK DO <command-id>
```

A command may append command-specific ASCII result fields.

For `DO host.screenshot.temp`, success returns the same created file through the
generic form:

```text
OK DO host.screenshot.temp PATH="<absolute-host-path>"
```

The dedicated `SCREENSHOT` alias renders that same semantic result as
`OK SCREENSHOT <absolute-host-path>`.

### 37.5 Aliases are not separate implementations

Special commands such as `RESET`, `SPEED`, and `SCREENSHOT` invoke the same
semantic handlers as their registry equivalents.

Tests must prove that alias and `DO` forms converge.

---

## 38. Telnet errors and response framing

Initial error codes include:

```text
BAD_COMMAND
BAD_COMMAND_ID
BAD_ARGUMENT
BAD_KEY
BAD_MODEL
BAD_SPEED
BAD_STATE
LINE_TOO_LONG
SCREENSHOT
```

Unknown special commands return:

```text
ERR BAD_COMMAND
```

Multi-line discovery/help responses terminate with:

```text
END
```

Single-line responses terminate with `CRLF` on output.

The protocol never emits terminal-control escape sequences as part of required
machine-readable responses.

---

## 39. Telnet security boundary

The Telnet listener is intentionally reachable on all host interfaces on its
dynamically selected Control Port in the 30740-32787 range, is plaintext, and
initially has no application-level authentication.

Therefore the initial protocol deliberately permits only `REMOTE_SAFE`
operations.

The initial release must **not** provide a hidden switch that silently turns
all registry commands into remotely executable commands.

A later design may add authentication, encrypted transport, source-address
policy, or explicit privileged remote modes. Such a change requires a new
security review and architecture update.

### 39.1 Host paths

The command tree may describe host-file operations, but the initial Telnet
policy denies arbitrary `HOST_READ` and `HOST_WRITE` execution.

`SCREENSHOT` is allowed precisely because it chooses a controlled temporary
output location itself.

### 39.2 Destructive media operations

Formatting, deleting, rewriting, or raw-editing media is denied remotely by
default even though those commands remain discoverable.

### 39.3 Application termination

`application.quit` remains visible through `MENU TREE` but is not remotely
executable in the initial architecture.

---

## 40. GUI/Telnet command-state model

A registered command has two independent state dimensions:

```text
application availability:
    ENABLED | DISABLED(reason)

Telnet permission:
    ALLOWED | DENIED(class/policy)
```

The GUI normally cares about application availability.

Telnet reports both.

Examples:

```text
Tape Eject, no tape mounted:
    DISABLED(no-tape-mounted)
    REMOTE_SAFE

Save Snapshot As:
    ENABLED
    HOST_WRITE -> DENIED over initial Telnet

Format Microdrive, writable cartridge mounted:
    ENABLED
    MEDIA_DESTRUCTIVE -> DENIED over initial Telnet

Reset:
    ENABLED
    REMOTE_SAFE -> ALLOWED
```

---

## 41. UI threading and timing

The UI follows the one-owner machine-state rule in the core/system architecture.

### 41.1 Host events

GUI events, toolbar events, Telnet commands, and application-test commands may
arrive from different host contexts. State-changing semantic commands are
serialized before touching machine state.

### 41.2 Blocking dialogs

A blocking host file dialog may pause host pacing. While application-paused for
a host modal operation, canonical master time does not advance unless the
specific workflow is explicitly designed as nonblocking.

Opening a file chooser is never itself a Spectrum timing event.

### 41.3 Long media operations

Long compatibility conversions or media maintenance operations should avoid
freezing unrelated presentation where practical, but correctness and atomic
file behavior take precedence over UI animation.

Background worker threads may process host-only data but may not mutate live
Spectrum state directly.

---

## 42. Keyboard focus and command shortcuts

Local keyboard input to the Spectrum and keyboard shortcuts for WZSN controls
must coexist predictably.

### 42.1 Focus ownership

When the Spectrum viewport owns keyboard focus, ordinary mapped Spectrum keys
are delivered through the host input arbiter.

Application shortcuts that intercept host key combinations must be explicitly
defined and must not accidentally leave Spectrum keys stuck.

### 42.2 Dialog focus

When a modal text/input control owns focus, host typing is consumed by that
control rather than silently reaching the Spectrum unless the control is
explicitly designed for pass-through.

Any focus transition releases or preserves host-source key ownership according
to one documented policy; it may not leave stale matrix presses.

### 42.3 Telnet independence

Changing local GUI focus does not disable the connected Telnet keyboard source.
Local and Telnet ownership remain independently resolved by the host input
arbiter.

---

## 43. Accessibility and keyboard operability

The semantic UI must be operable without a mouse for core workflows.

At minimum:

```text
menu traversal
Open / Run
pause/resume
reset
speed selection
tape insert/eject/loading mode
snapshot load/save
fullscreen toggle
opening debugger/settings/status
confirm/cancel of destructive local operations
```

must have normal keyboard-operable paths supported by the selected UI toolkit.

Visible focus, readable disabled-state explanations where practical, and
non-color-only status signaling are required.

Platform-native accessibility APIs should be used when supplied by the chosen
UI toolkit. The Phase-12 toolkit decision must document accessibility support.

---

## 44. Settings persistence

Host preferences may persist across application launches.

Examples include:

```text
window geometry
fullscreen preference
display presentation settings
audio output preference
host input mappings
last selected emulation speed if product policy chooses to persist it
status-panel visibility
recent-file list
```

Persistent host configuration is not canonical Spectrum state.

Machine-critical certification inputs such as ROM identity remain explicit and
must not silently drift merely because a preferences file changed.

Media insertion state is not automatically persisted unless a specific
workflow is later designed and tested.

### 44.1 Multi-instance persistence and file ownership

Multiple WZSN processes may use the same user profile concurrently. The UI/host
layer must therefore obey the core architecture's interprocess host-data safety
contract.

At minimum:

- shared settings writes are serialized across processes and committed by atomic
  replacement; last-writer-wins for a noncritical preference is acceptable only
  after a complete valid write, never through file corruption;
- the selected Control Port is not written into persistent settings;
- writable media mounting requires an interprocess exclusive write claim based
  on resolved host file identity where practical; a second process may not
  silently become another writer to the same image through a path alias;
- snapshots, conversions, exports, and other persistent output writes use
  interprocess serialization/exclusive destination ownership or atomic
  write-and-replace semantics so concurrent processes cannot produce partial or
  interleaved output;
- Telnet screenshot no-clobber behavior uses atomic exclusive creation, including
  collision-suffix retries across processes.

---

## 45. Error and confirmation policy

### 45.1 Non-destructive failures

Failed file opens, malformed snapshots, unsupported formats, screenshot write
failures, and Telnet bind failures are reported without corrupting live machine
state.

### 45.2 Destructive local operations

Operations such as formatting a Microdrive, deleting blocks/files, raw-sector
editing, or overwriting an existing output require explicit local confirmation
unless the action is intrinsically reversible and that behavior is clearly
shown.

### 45.3 Remote confirmation

The initial Telnet protocol does not emulate GUI confirmation prompts. Commands
that would require destructive or privileged confirmation are denied by remote
policy instead.

### 45.4 No silent fallback

The UI must not silently:

```text
change machine model
select a Control Port outside the frozen 30740-32787 probe policy
replace Normal tape mode with trap mode
choose a lossy conversion
change output path after a write failure
overwrite an existing Telnet screenshot
```

---

## 46. Application test projection

The shared command registry is also the application-level integration-test API.

Tests invoke registry commands directly without requiring Sokol or Telnet when
the presentation/transport layer is not the subject under test.

Required equivalence examples include:

```text
GUI Reset == toolbar Reset == Telnet RESET == DO machine.reset

GUI 400% == toolbar 400% == Telnet SPEED 400
          == DO machine.speed.set 400

GUI Tape Eject == toolbar Tape Eject == permitted direct registry invocation

GUI screenshot raster service == Telnet SCREENSHOT raster service
```

The host file destination differs between GUI screenshot save and Telnet temp
screenshot, but pixel source and PNG encoding path are shared.

---

## 47. Required UI regression tests

Automated or controlled application tests must cover at least:

```text
start with default supported machine and no media
48K <-> 128K model change/reset workflow
pause/resume from menu, toolbar, registry, and Telnet
reset from menu, toolbar, registry, and Telnet
all seven emulation-speed values from menu, toolbar, registry, and Telnet
speed change while running
speed change while paused
correct audio-muted status outside 50%-200%

Open / Run native format routing
cancel file chooser without state mutation
malformed/unsupported file error path
snapshot atomic load failure
snapshot save/save-as

Tape insert/eject
Normal default
Instant/Trap explicit selection
Tape Manager enabled-state transitions
block mutation atomic/error behavior

Microdrive mount/eject for drives 1-8
same writable image duplicate-mount prevention
format destructive confirmation
logical-file and sector-manager state

ZX Printer manager availability
Debugger opening without second core
Fullscreen without core-state change
status panel correctness
Telnet listener/client/bind status presentation
Control Port status text always visible
first instance obtains 30740 when free
second/third concurrent instances obtain first free ascending candidates
occupied candidate is skipped without using a port outside 30740-32787
simultaneous-start race cannot produce duplicate numeric Control Port ownership
full 2048-port exhaustion reports unavailable without terminating WZSN
selected Control Port is not persisted across launches
IPv4/IPv6 split-family collision rejects the candidate numeric port

networking radio group has exactly None / Interface-1 / Ear+Mic
networking mode is structurally mutually exclusive
None disables Interface 1, Microdrive, ZX Net, and Ear+Mic attachment
Interface-1 enables authentic IF1/Microdrive/ZX Net and disables Ear+Mic
Ear+Mic disables Interface 1/Microdrive/ZX Net and does not page a networking ROM
Ear+Mic selection does not auto-install the distributed RAM stack
Ear+Mic disables ordinary Tape transport while routed network owns EAR/MIC
Architecture-3 Bootstrap Ear+Mic Stack uses BOOTSTRAP_TAPE waveform path
networking-mode change uses cold reconfiguration, clears old RAM/hooks, and preserves paused application state
Ear+Mic unavailable-before-Architecture-3 reason propagates through registry/UI
Ear+Mic on 128K/non-Issue2 profile is disabled with requires-48k-issue2 reason

cross-process writable-media exclusion
cross-process settings atomic-write safety
cross-process Telnet screenshot no-clobber creation

command-registry stable-ID uniqueness
required menu-node/action and non-menu semantic IDs from Section 7.1 exist
Snapshot Inspector reachable through Tools and shared state-inspection code
one handler path for equivalent GUI/toolbar/Telnet/test commands
identical enabled predicate across GUI and MENU output
disabled-reason propagation
remote permission-class enforcement

MENU root
MENU TREE complete registry traversal
MENU <id>
MENU FIND
DESCRIBE
DO success
DO bad ID
DO bad argument
DO disabled command
DO denied command

RESET exact response and connection preservation
PAUSE/RESUME idempotence
MODEL valid/invalid values
SPEED valid/invalid values
SCREENSHOT while running
SCREENSHOT while paused
SCREENSHOT PNG pixel-source equivalence
SCREENSHOT temp-directory placement
SCREENSHOT timestamp filename pattern
SCREENSHOT collision suffix/no overwrite
SCREENSHOT failure without machine mutation

Telnet HELP/STATUS response contracts
1024-byte line limit/recovery
quoted DO argument parsing
no shell/environment expansion
multi-line END termination
no terminal escape/control pollution

keyboard-only operation of required local workflows
focus transitions without stuck Spectrum keys
local focus change while Telnet keys remain independently owned
```

---

## 48. UI acceptance contract

The initial WZSN UI architecture is complete only when all of the following are
true:

1. the semantic top-level menu tree is `File`, `Machine`, `Media`, `View`,
   `Tools`, `Settings`, `Help`;
2. the running Spectrum display is the primary application surface;
3. one shared command registry owns every cross-front-end semantic operation;
4. stable command IDs are unique and independent of cosmetic GUI wording;
5. GUI menus, toolbar, Telnet control, and application tests use shared semantic
   handlers rather than private duplicate implementations;
6. command availability predicates and disabled reasons come from the registry;
7. Telnet reports the same availability state as the GUI;
8. remote permission class is independent from application availability;
9. the initial Telnet service allows only `REMOTE_SAFE` command execution;
10. the entire command/menu tree remains discoverable over Telnet even when a
    command is denied remotely;
11. `Open / Run...` is the universal initial user entry for supported media;
12. Tape Insert/Eject/Loading Mode are direct quick operations;
13. Normal tape loading is visibly the default;
14. detailed tape manipulation lives in Tape Manager;
15. detailed Microdrive manipulation lives in Microdrive Manager;
16. snapshots use load/save semantics rather than persistent select/unselect;
17. register/hardware/memory editing reuses debugger/state-inspection machinery;
18. legacy conversion features are grouped under Compatibility Tools;
19. historical DOS shell/directory/heap/child-EXE UI is absent;
20. the historical database application is not reproduced as an initial
    top-level subsystem;
21. Dock UI is absent until Timex/DCK support is actually implemented;
22. the toolbar contains the canonical quick controls in Section 24;
23. the emulation-speed selector exposes exactly 25, 50, 100, 200, 400, 800,
    and Unlimited;
24. speed state is visible and audio-muted-by-speed state is visible;
25. Machine Reset invokes the authentic machine reset and does not restart WZSN;
26. GUI Reset, toolbar Reset, Telnet `RESET`, and `DO machine.reset` converge on
    the same handler;
27. Telnet remains connected across `RESET`;
28. `PAUSE` and `RESUME` are idempotent shared-command aliases;
29. `MODEL 48K` and `MODEL 128K` use the same model-change/reset workflow as the
    GUI;
30. Telnet `SPEED` uses the same runtime-speed handler as GUI/toolbar control;
31. Telnet `SCREENSHOT` writes PNG to the OS temporary directory;
32. Telnet screenshots use `ZX-Screen-YYYYMMDDHHMMSSmmm.png` and collision
    suffixes rather than overwriting;
33. Telnet `SCREENSHOT` reports the absolute saved path only after a successful
    completed write;
34. screenshot capture excludes host application chrome and does not mutate or
    advance Spectrum state;
35. GUI screenshot save and Telnet screenshot use the same capture/PNG path;
36. `MENU`, `MENU TREE`, `MENU <id>`, `MENU FIND`, `DESCRIBE`, and `DO` operate
    against the shared registry;
37. state-disabled registry commands are represented consistently in GUI and
    Telnet discovery;
38. host-read, host-write, destructive-media, application-control, and
    local-only commands are denied by initial unauthenticated Telnet policy;
39. the Telnet application parser is bounded to 1024-byte decoded command lines
    and deterministically recovers after overflow;
40. Telnet generic arguments are never interpreted by an OS shell;
41. required local workflows are keyboard-operable without a mouse;
42. focus changes do not leave stuck local Spectrum matrix keys;
43. local GUI focus does not disable independently owned Telnet keys;
44. fullscreen, manager windows, dialogs, and status presentation do not alter
    canonical machine state;
45. cancellation and non-destructive error paths leave machine/media state
    unchanged where the operation has not committed;
46. destructive local media actions require explicit confirmation;
47. platform-specific menu relocation does not change semantic command IDs;
48. the complete legacy-item disposition in Section 20 is represented in the
    backlog with no unclassified legacy menu command;
49. Phase-12 UI toolkit selection documents static-link/single-binary fit,
    keyboard operation, and accessibility support;
50. all Section 47 required regression tests applicable to the implemented
    milestone pass;
51. successful Telnet keyboard commands return the Section 27.1 response and
    invalid/held-key cases return the frozen error responses;
52. reset/model changes invoked while paused leave the application paused;
53. `MENU TREE` uses the Section 35 record format and does not disclose dynamic
    Recent-file absolute paths;
54. `STATUS`/`DESCRIBE` do not expose arbitrary absolute host media paths under
    the initial unauthenticated policy;
55. `Tools > Snapshot Inspector...` exists and reuses shared state-inspection
    machinery rather than a separate snapshot-state implementation;
56. every required Section 7.1 menu/action and non-menu semantic command ID is
    registered with the frozen metadata contract;
57. the status line always exposes `Control Port: <number>` for an available
    listener or `Control Port: unavailable` after full range exhaustion;
58. each WZSN process uses the first bindable candidate in 30740-32787 and
    simultaneous processes cannot acquire the same numeric Control Port;
59. the selected Control Port is session state and is never persisted;
60. the Networking settings surface is one radio group with exactly `None`,
    `Interface-1`, and `Ear+Mic`, backed by `machine.networking.set`;
61. `Interface-1` and `Ear+Mic` cannot be active simultaneously in UI state,
    registry state, or core state;
62. `Ear+Mic` performs no networking-ROM paging and does not automatically
    install the Architecture-#3 resident RAM stack;
63. networking-mode changes use the shared cold machine-reconfiguration path,
    do not preserve old Spectrum RAM/hooks/device state, and preserve only the
    application paused/running state;
64. multi-instance settings writes, writable-media claims, and Telnet screenshot
    creation satisfy the interprocess safety contract without corrupting shared
    host data;
65. `machine.networking.set` has an explicit remote-permission class and, when
    invoked through an allowed front end, reaches the same cold-reconfiguration
    handler as the Networking radio group; dirty Interface-1 media makes the
    remote command unavailable until resolved locally;
66. in `Ear+Mic` mode the ordinary Tape transport cannot drive/consume the same
    cassette signals concurrently with the routed virtual network, and the
    Architecture-3 bootstrap action uses the explicit `BOOTSTRAP_TAPE` waveform
    path rather than RAM injection;
67. `Ear+Mic` is unavailable on 128K or any 48K profile/variant not certified for
    Architecture #3's Issue-2 target, with a stable disabled reason and no
    machine-state mutation on a rejected request.

---

## 49. Phase-12 UI implementation gate

Before Phase-12 implementation tickets are issued, the project must freeze:

```text
selected UI toolkit and exact pinned revision
platform integration approach
font/text rendering strategy
native versus in-window menu presentation per platform
file-dialog implementation
accessibility support
window/panel persistence approach
exact application command-registry C API
exact command result/error representation
```

The Phase-12 toolkit decision is frozen as follows: Nuklear at the immutable
revision recorded in `design/dependencies/nuklear-pin.md` supplies portable
in-window widgets and event translation. Sokol remains the viewport/presentation
boundary. Native platform menus and file dialogs are used where they provide
the expected desktop behavior, with semantic command IDs remaining unchanged.
Keyboard navigation, focus order, labels, and actionable states are mandatory
project-owned accessibility behavior; host adapters expose those semantics to
platform accessibility facilities where available. Versioned settings are
host-only and interprocess-safe, and never contain canonical machine state.

Those choices may alter implementation but not the semantic menu tree,
command IDs, manager ownership, Telnet grammar, or acceptance requirements in
this document.

Phase 12 exits only when all initial GUI workflows and command-registry tests
applicable before Telnet transport integration pass.

---

## 50. Phase-15 Telnet-control gate

Phase 15 combines the core/system Telnet transport/keyboard contract with this
UI command/control architecture.

Before Phase 15 exits:

```text
core Section 55 transport/keyboard acceptance passes
special-command aliases pass
MENU/DESCRIBE/DO registry projection passes
remote permission policy passes
RESET/SPEED/MODEL/PAUSE/RESUME equivalence passes
SCREENSHOT temp-path/PNG/reporting contract passes
GUI/Telnet status presentation agrees with listener state
```

No application-control command may be considered complete merely because it
works through Telnet; the equivalent shared-registry and GUI/test path must
also be demonstrated where applicable.

---

## 51. Ticket-derivation contract for UI work

Every UI implementation ticket must state, at minimum:

```text
this document section(s)
stable command ID(s) involved
GUI surface(s) involved
Telnet projection/permission class if applicable
core/system architecture dependency
state availability predicate
disabled reason(s)
parameter acquisition path
success result
cancel path
error path
machine-state/timing effect or explicit no-effect
keyboard accessibility path
automated regression evidence
```

A ticket may not invent a new semantic command when an existing registry
command covers the operation.

A UI-only presentation ticket may not modify core behavior to simplify its
implementation.

---

## 52. Initial non-goals

The initial UI architecture does not require:

- reproducing Turbo Vision appearance;
- reproducing DOS directory or shell functions;
- reproducing the historical Warajevo database application;
- exposing every preservation converter in the first executable milestone;
- Timex/DCK UI before Timex support exists;
- live physical cassette-capture UI before that feature is promoted from LATER;
- mobile/touch-specific layout;
- WebAssembly/browser UI;
- a plugin marketplace;
- remotely executable arbitrary host-file operations over the unauthenticated
  Telnet service;
- remotely executable destructive media operations over the unauthenticated
  Telnet service;
- remote WZSN termination;
- ROM/BASIC text injection as a substitute for keyboard-matrix input.

---

## 53. Frozen UI baseline

The following decisions are now frozen for initial development:

```text
Primary navigation:
    File / Machine / Media / View / Tools / Settings / Help

Core interaction model:
    running Spectrum is primary
    media managers are subordinate tools
    compatibility conversions are subordinate preservation tools

Application command architecture:
    one shared command registry
    stable lowercase dotted IDs
    GUI / toolbar / Telnet / tests converge on shared semantic handlers
    common availability predicates and disabled reasons

Toolbar:
    Open/Run
    Pause/Resume
    Reset
    Emulation Speed selector
    Tape control
    Load Snapshot
    Save Snapshot
    MDV 1 control
    Screenshot
    Fullscreen
    Debugger

Emulation speed:
    25 / 50 / 100 / 200 / 400 / 800 / Unlimited
    visible in toolbar/status
    uses canonical core runtime-speed semantics

Networking:
    one radio group: None / Interface-1 / Ear+Mic
    backed by machine.networking.set
    Interface-1 and Ear+Mic are structurally mutually exclusive
    changing mode cold-reconfigures the machine and discards old Spectrum RAM/hooks
    Ear+Mic is available only on Architecture-#3-certified 48K Issue-2 profile/variant
    Ear+Mic pages no networking ROM and does not auto-install the resident stack
    Ear+Mic owns cassette EAR/MIC; ordinary Tape transport is disabled in that mode
    Bootstrap Ear+Mic Stack uses Architecture #3 BOOTSTRAP_TAPE waveform path

Screenshot:
    PNG
    Spectrum display only
    shared capture/encoder path
    GUI chooses destination
    Telnet writes to OS temp
    Telnet filename ZX-Screen-YYYYMMDDHHMMSSmmm.png
    collision suffix; never overwrite
    Telnet reports absolute saved path after success

Telnet / Control Port:
    one client per WZSN process
    first-free Control Port in 30740-32787; wildcard transport owned by core architecture
    status line always shows Control Port: xxxxx or Control Port: unavailable
    selected Control Port is session state and is not persisted
    physical keyboard commands preserved
    whole command/menu registry discoverable
    generic MENU / DESCRIBE / DO interface
    special HELP / STATUS / PAUSE / RESUME / RESET / MODEL / SPEED / SCREENSHOT
    only REMOTE_SAFE operations executable initially
    no authentication/encryption in initial transport

Multi-instance host safety:
    shared settings writes are interprocess-safe and atomic
    writable media has one writer across processes
    snapshots/conversions/exports cannot be partially interleaved by concurrent writers
    Telnet screenshot creation is atomically no-clobber across processes

Legacy UI migration:
    preserve capabilities, not 1998 navigation
    detailed tape/Microdrive operations move to media managers
    snapshot state editing moves to debugger machinery
    old-format converters move to Compatibility Tools
    DOS/host-era shell/directory/hardware-compensation UI is dropped
    historical database UI is not an initial subsystem
    Dock UI appears only with implemented Timex/DCK support
```

The design objective is:

> A modern WZSN interface in which ordinary Spectrum use stays simple, Warajevo's
> deep preservation/media capabilities remain available without dominating the
> main UI, and GUI, Telnet, automated tests, and future control surfaces all
> operate the same application through one stable semantic command layer.
