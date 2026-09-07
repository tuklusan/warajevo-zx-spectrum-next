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

## Windows Debugger Prerequisite

Windows remote validation requires the Windows SDK Debugging Tools for native
crash diagnosis. At minimum, install `cdb.exe` and `windbg.exe`; the standard
installation location is:

```text
C:\Program Files (x86)\Windows Kits\10\Debuggers\<architecture>\
```

The debugger probe checks that tree as well as `PATH`. Debugger binaries are
third-party platform components and are intentionally not stored in this
public repository. Obtain them through Microsoft's official Windows SDK
installer. `procdump.exe` and `llvm-addr2line.exe` are optional diagnostics,
not prerequisites for the standard smoke gate.

- `invoke_remote_harness.py`
  Local orchestrator for the approved SSH remotes
- `invoke_remote_windows_powershell.py`
  Runs a repository-local PowerShell script on an approved Windows SSH remote
  through the parser-first path and records the session under
`test-artefacts/remote-runs/`

Smoke validation first verifies the exact published CODE PASS authority. If
review authority is missing or stale, no SSH connection or remote command is
attempted; the harness writes `session.json` with
`classification: review_pending` and `remote_execution_started: false`. SSH,
build/test, and artifact-transport failures are recorded only after this
precondition has passed and remain separate classifications.

The Intel Mac SSH lane uses the frozen connection path when
`WZSN_MAC_SSH_PASSWORD` is present and `sshpass` is installed: `ssh -tt` with
explicit password authentication and public-key authentication disabled. If
that secret is absent, the configured local key remains the fallback.
Interactive Windows commands retain the TTY path, while Windows archive pulls
use an explicit non-TTY channel so base64/ZIP transport cannot be contaminated
by terminal framing.

`forward_syslog.py` forwards every file in an approved local or hosted
evidence tree as bounded base64 Unix syslog records over UDP to the fixed
secured endpoint. The local remote harness invokes it after each lane; the
hosted matrix invokes it from an always-run, non-blocking step before upload.
Both paths require `WZ_TRACE_FORWARD=Y` or `WZ_TRACE_FORWARD=1`; absent or
other values produce no network traffic.

`run_durable_session.py` is the project-owned boundary for slow or verbose
operations. It atomically writes a JSON state record and emits a small start
event before waiting; stdout and stderr are redirected to declared files, and
the state supports polling or termination without relaunching the command.
- `run_cmake_smoke.py`
  Shared probe and CMake smoke-build entry point used both remotely and by
  GitHub Actions
- `verify_platform_evidence.py`
  Verifies each lane's summary, inventory, conformance manifest, and optional
  screenshot/trace evidence, then writes the machine-readable result manifest.
- `run_fuse_ed_platform.py`
  Acquires the pinned Fuse source privately and gates every platform on the
  complete ED corpus, the complete DDCB/FDCB indexed-bit corpus, all ordinary
  CB rotate/shift vectors including undocumented SLL, and all CALL/RET/RST/
  PUSH/POP vectors including conditional paths, plus all JR/JP/DJNZ branch
  vectors with taken and not-taken paths, all primary INC/DEC vectors, and the
  primary HALT state/timing vector.
  Acquires the pinned Fuse revision into private project-local test artifacts,
  verifies its identity, and runs every ED vector without silent skips
- `stream_zip_tree.py`
  Packages a remote artefact tree to stdout as ZIP bytes for pull-back
- `capture-linux-active-display.sh`
  Captures the active Linux desktop when X11 session access is available
- `cleanup-hosted-runner-state.sh`
  Runs the pre-matrix housekeeping gate: it removes older same-ref
  `platform-smoke` queue entries and their temporary Actions artifacts, then
  clears only project-generated hosted-runner workspace residue. Matrix lanes
  use its `--workspace-only` mode and never call the GitHub API.
- `Capture-WindowsDesktopScreenshot.ps1`
  Captures the active Windows desktop through the screen API
- `Invoke-WindowsInteractiveScreenshot.ps1`
  Bridges an SSH-created noninteractive process into the active Windows
  desktop through a short-lived interactive Task Scheduler action, then
  removes the action after the screenshot is written
- `windows-toolchain/*.ps1`
  Reusable Windows toolchain maintenance and verification scripts for the
  approved remotes

## Local usage

Run the orchestrator from the repository root:

```text
python tools/harness/invoke_remote_harness.py probe linux-x64-lxqt
python tools/harness/invoke_remote_harness.py smoke windows-10-reference
python tools/harness/invoke_remote_harness.py screenshot windows-11-laptop
python tools/harness/invoke_remote_harness.py probe macos-bigsur-lab
python tools/harness/invoke_remote_windows_powershell.py windows-11-laptop tools/harness/windows-toolchain/verify-toolchain.ps1
```

The current lab snapshot has all four hosts available: Linux at
`192.168.4.76`, macOS/Intel at `192.168.4.77`, Windows 10 at `192.168.4.75`,
and Windows 11 at `192.168.4.103`. Record any unavailable-host transport
results as environmental and continue with the complete hosted runner matrix.

The temporary hosted-only validation mode has been withdrawn by operator
direction. Local test-machine soaks are enabled again; use the existing SSH
harness and machine definitions without changing their safety contracts.
Continue to run and fully await the complete GitHub-hosted matrix as required.

Before probes and tests, the orchestrator updates the approved remote checkout
with `git pull --ff-only origin main`; it refuses to proceed if the checkout
cannot be advanced safely. The orchestrator always pulls results back into:

```text
test-artefacts/remote-runs/<machine>/<run-id>/
```

## Remote artefact convention

## Harness Lock

All working harnesses and their governing parser/gate scripts are tracked in
`tools/harness/harness-lock.json`. The lock is active. Any modification to a
listed harness requires explicit operator authorization before editing,
review, commit, or publication. Do not replace a working harness with an
untracked one-shot script.

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
- `macos-14`
- `macos-13`
- `macos-15-intel`
- `macos-26-intel`
- `macos-26`
- `xcode-27`

The workflow lives in:

```text
.github/workflows/platform-smoke.yml
```

It runs the same CMake smoke and pinned Fuse ED conformance entry points as the
SSH remotes and uploads the resulting manifest and smoke artefact tree for each
runner. The acquired upstream source remains under ignored `test-artefacts/`
and is not uploaded.

The hosted matrix must be allowed to finish naturally. Keep `fail-fast: false`
and wait for all lanes, including queued or slow lanes. A queued job is not a
failure and must not trigger cancellation or a replacement run; the aggregate
publication gate evaluates only terminal conclusions for the exact commit.

Every configured hosted macOS lane is scheduled when capacity permits. The
publication gate accepts macOS verification when at least one Intel lane and
at least one ARM lane succeeds, while every non-macOS lane succeeds. At CR
closure, an exact-commit result from a reachable local or remote Intel lab
machine and/or ARM lab machine may substitute for the corresponding hosted
architecture; record the machine identity and complete results under
`test-artefacts/`. A queued, missing, or unavailable lane is environmental
evidence, never an implicit pass, and all configured lanes must still be
awaited to terminal state.

The `deep-housekeeping` job runs before the matrix. It preserves the current
run, cancels only older queued or in-progress `platform-smoke` runs for the
same ref, deletes artifacts owned by those older runs, and clears generated
build/test residue. It does not delete source, tracked guidance, or retained
test evidence. Each matrix lane repeats only the workspace cleanup after
checkout so reused runner workspaces cannot influence a result.

## Screenshot notes

Windows screenshot capture uses the active desktop exposed to the logged-in user
session.

Linux screenshot capture depends on access to an active X11 desktop session.
The current Linux remote already has capture tools installed, but successful
capture still depends on `DISPLAY` and `XAUTHORITY` resolving to the live
desktop session.

The Windows SSH path uses the tracked interactive-session bridge and removes
its temporary scheduled task after capture. This path was verified on the
Windows 11 remote under `CR-0014`; future application screenshot evidence uses
the same run-local bridge.
