<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Phase 12 UI implementation decisions

These decisions complete the Phase 12 design record. They do not close the
Phase 12 implementation or acceptance gate.

## Phase 12 gate review

Reviewed on 2026-09-27 against UI Architecture §§4–7, 18, 24–25, 43, and 49,
and the Core host/core boundary. The choices retain the canonical semantic menu
tree, shared registry, host-only preferences, and deterministic machine state.
Platform-specific rendering and file-dialog dependencies stay in the host; the
modal-dialog pacing contract re-anchors host time without changing core clock
ratios; all returned file paths cross the command boundary as UTF-8. The
registry result and permission metadata remain shared by GUI, application
tests, and Telnet projections. No design conflict blocks implementation, so
this baseline is approved for implementation.

This review approves design choices only. File dialogs, the versioned settings
reader/writer, complete registry population, and complete local UI workflows
remain unimplemented or incomplete. Keyboard/focus behavior and platform
accessibility exposure remain acceptance gaps; none is waived by this review.

## Toolkit, renderer, and text

- Use Nuklear v4.13.3 at the immutable commit recorded in
  [`nuklear-pin.md`](dependencies/nuklear-pin.md).
- Compile Nuklear into the application and render through the matching pinned
  Sokol `sokol_nuklear.h` backend. The host uses D3D11 on Windows, Metal on
  macOS, and X11/OpenGL on Linux.
- Use Nuklear's embedded ProggyClean default font; ship no external font file.
- Render the seven semantic menus in-window on each platform. Do not create
  native menu bars; semantic IDs and registry handlers remain canonical.

## File dialogs

- Windows open/save use the system Common Item Dialog interfaces
  `IFileOpenDialog` and `IFileSaveDialog`.
- macOS open/save use AppKit `NSOpenPanel` and `NSSavePanel`.
- Linux/X11 uses GTK 3.20 or later `GtkFileChooserNative`; it uses an available
  desktop file chooser portal and otherwise falls back to the GTK chooser.
- Dialogs run synchronously on the host/UI owner thread. Host pacing is paused
  while a modal dialog is open and re-anchored after it closes. Dialog results
  return UTF-8 paths; cancel is a distinct outcome and dispatches no command.
- GTK is a Linux host build/runtime dependency only. Windows and macOS use their
  operating-system APIs without GTK.

## Accessibility and persistence

- Nuklear supplies in-window keyboard input/navigation. The project owns menu
  labels, action state/reasons, focus order, and a visible focus indicator.
  No native accessibility bridge is currently implemented; platform API
  exposure remains an acceptance gap and is not waived by this choice.
- Persist only host preferences, never canonical machine/session state. Use a
  versioned `host-settings.v1` key/value file in the platform's user config
  directory: `%LOCALAPPDATA%` on Windows, Application Support on macOS, and
  `$XDG_CONFIG_HOME` (falling back to `~/.config`) on Linux. Serialize writes
  with an exclusive sibling lock, a same-directory temporary file, and atomic
  replacement. Store no ROM contents, media contents, or remote secrets.

## Command registry API

The C API is the public surface in [`wz_command_registry.h`](../src/app/wz_command_registry.h).
Initialize fixed-capacity caller-owned storage, bind the owner thread, register
stable dotted IDs with semantic metadata and handlers, then finalize. Read-only
lookup/state projection is available after registration; state-changing
dispatch is serialized on the bound owner thread. Menus and toolbars acquire
parameters in the UI and dispatch through this API; they do not implement
semantic behavior.

Handlers return `wz_result_t` and populate `wz_command_result_t`: `status` is
one of success, rejected, unavailable, or failed; `result` carries the stable
project result code; `reason` carries a stable machine-readable reason; and
`message` carries bounded user-facing detail. This representation is shared by
the GUI, application tests, and the later Telnet projection.

## References

- [Nuklear v4.13.3](https://github.com/Immediate-Mode-UI/Nuklear/tree/a53ad2c658151071501372a5e0e5e978153835aa)
- [Sokol pinned renderer/event adapter](https://github.com/floooh/sokol/blob/1847290135f95e57e6d220b0a41208306aafc0dd/util/sokol_nuklear.h)
- [GTK `GtkFileChooserNative`](https://docs.gtk.org/gtk3/class.FileChooserNative.html)
- [Windows `IFileOpenDialog`](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nn-shobjidl_core-ifileopendialog) and [`IFileSaveDialog`](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nn-shobjidl_core-ifilesavedialog)
- [Apple `NSOpenPanel`](https://developer.apple.com/documentation/appkit/nsopenpanel) and [`NSSavePanel`](https://developer.apple.com/documentation/appkit/nssavepanel)
