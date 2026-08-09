<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Shared Remote Harness

This directory contains the tracked baseline harness entry points for:

- SSH-driven remote probes, smoke builds, and screenshot capture
- GitHub-hosted platform smoke builds
- remote artefact packaging for pull-back into `test-artefacts/`

## Public entry points

- `invoke_remote_harness.py`
  Local orchestrator for the approved SSH remotes
- `run_cmake_smoke.py`
  Shared probe and CMake smoke-build entry point used both remotely and by
  GitHub Actions
- `stream_zip_tree.py`
  Packages a remote artefact tree to stdout as ZIP bytes for pull-back
- `capture-linux-active-display.sh`
  Captures the active Linux desktop when X11 session access is available
- `Capture-WindowsDesktopScreenshot.ps1`
  Captures the active Windows desktop through the screen API

## Local usage

Run the orchestrator from the repository root:

```text
python tools/harness/invoke_remote_harness.py probe linux-x64-lxqt
python tools/harness/invoke_remote_harness.py smoke windows-10-reference
python tools/harness/invoke_remote_harness.py screenshot windows-11-laptop
```

The local orchestrator always pulls back into:

```text
test-artefacts/remote-runs/<machine>/<run-id>/
```

## Remote artefact convention

On each approved SSH remote, the harness writes transient outputs only inside:

```text
.wzsn-harness/<run-id>/
```

under that remote machine's approved project directory.

This keeps every harness-created build directory, log, screenshot, and summary
inside the project tree while remaining ignored by Git.

## GitHub-hosted runner baseline

As of 2026-08-09, the tracked hosted smoke matrix uses these standard labels:

- `ubuntu-24.04`
- `ubuntu-24.04-arm`
- `windows-2025`
- `macos-15`
- `macos-15-intel`

The workflow lives in:

```text
.github/workflows/platform-smoke.yml
```

It runs the same `run_cmake_smoke.py` entry point as the SSH remotes and uploads
the resulting artefact tree for each runner.

## Screenshot notes

Windows screenshot capture uses the active desktop exposed to the logged-in user
session.

Linux screenshot capture depends on access to an active X11 desktop session.
The current Linux remote already has capture tools installed, but successful
capture still depends on `DISPLAY` and `XAUTHORITY` resolving to the live
desktop session.
