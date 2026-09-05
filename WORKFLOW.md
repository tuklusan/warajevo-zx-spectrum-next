<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Project Workflow

## External Review Gate

Before any CR code is executed on a designated remote or hosted test system,
the exact committed snapshot must pass the independent CODE gate documented in
`design/review-gate.md`. Required documentation and post-test artifact
reviews are also mandatory. Serious findings are advisory to the developer but
may not be silently ignored; corrections or evidence-backed disputed
dispositions require a complete new review.

Normal CODE review is bound to the active CR, a clean committed current head,
exact requirement sources, and immutable Git-object evidence. Reviewer passes
discover candidates; only candidates that survive deterministic provenance
checks and hostile falsification become blockers. A new CODE attempt
invalidates the prior private receipt before review begins, and unresolved or
ambiguous authority remains fail-closed.

## Migration Pre-Development Gate

Before editing implementation artifacts for every CR or other tracked work item,
create `design/cr-preflight/CR-NNNN.md`. The record must enumerate every
functional aspect in scope, including normal behavior, edge cases, state
transitions, timing/bus effects, data formats, error behavior, UI or external
contract effects, tests, and compatibility implications.

The record must identify the architecture/task authority and the upstream
repository commit plus every source file, symbol, or search domain examined.
Each discovered upstream behavior must have an explicit disposition:
`IMPLEMENT_NOW`, `DEFERRED_WITH_CR`, `NOT_APPLICABLE`, or
`REIMPLEMENTED_DIFFERENTLY` with an evidence-backed reason.

Before implementation begins, repeat the upstream discovery scan using broader
symbols and neighboring control paths. The preflight exit is valid only when
the record states `Status: APPROVED_FOR_IMPLEMENTATION` and its zero-gap scan
lists no undispositioned behavior. CODE review and remote/hosted execution are
blocked until this tracked record exists for the active CR.

## Root Rule

Never create any project item or artifact above the project directory.

Always create, edit, store, and manage project artifacts within the project
directory tree.

## Local Machine Rule

Never build or test on this local machine.

This local machine is for editing source, documents, workflow files, issue
records, and other development artifacts only.

## Remote Build and Test Rule

All build, test, log-capture, trace-capture, and similar execution activity
must run only on the approved remote test machines.

GitHub-hosted runners used by this repository's tracked workflows are approved
remote build and test machines.

All pulled-back logs, traces, test results, screenshots, and similar outputs
must be stored under:

```text
test-artefacts/
```

The public harness contract for those machines lives in
`test-artefacts/README.md`, and the tracked shared harness entry points live in
`tools/harness/`.

The current lab availability snapshot has all four approved hosts available:
Linux `192.168.4.76`, macOS/Intel `192.168.4.77`, Windows 10
`192.168.4.75`, and Windows 11 `192.168.4.103`. Any SSH failure must be
recorded as an environmental result and must not be treated as evidence that
the software failed. The complete hosted runner matrix remains required for
multi-platform proof regardless of lab availability, and every configured
hosted macOS lane should be exercised when capacity permits.

Hosted-runner waiting is terminal-state based. Use one run for the exact
published commit with `fail-fast: false`; wait for every configured lane,
including queued or slow lanes. Never cancel or replace a live run and never
classify a queued lane as failed. Only terminal GitHub job conclusions may
feed the publication gate.

The hosted matrix has a pre-matrix deep-housekeeping gate. It may cancel only
older queued or in-progress `platform-smoke` runs for the same ref, and may
delete only artifacts owned by those older runs. It must preserve the current
run and all committed source, guidance, and retained evidence. Every matrix
lane also clears only generated build/test residue from its checked-out
workspace before execution using workspace-only mode; queued or slow jobs in
the current run remain valid and must still be awaited.

Cross-platform macOS acceptance is architecture-based, not count-based. The
hosted matrix should schedule every configured macOS label, but publication
requires only one successful Intel macOS lane and one successful ARM macOS
lane, with all non-macOS lanes successful. A reachable local or remote Intel
macOS result and a reachable local or remote ARM macOS result may substitute
for the corresponding hosted lane at CR closure when the exact commit, test
results, and machine identity are recorded under `test-artefacts/`. Missing
or queued lanes are never silently counted as success; every configured lane
must still reach a terminal state before acceptance.

## Private Local Artifacts

## Local Soak Validation Mode

The temporary hosted-only exception has been withdrawn by operator direction.
Local test-machine soaks are enabled again. Keep the local test-machine
harness, machine definitions, SSH credentials, screenshot paths, and approved
directory restrictions unchanged. GitHub-hosted validation remains required
and must still await every configured lane; local results supplement rather
than silently replace hosted evidence.

Private local-only material still belongs inside the project directory.

For the difficult-media regression corpus, use:

```text
WZSN-PRIVATE-TEST-MEDIA/
```

within the repository root. Only its public guidance files are committed. The
private media files inside it remain local-only and ignored by Git.

## Public Repository Rule

If an artifact is intended to stay private, the repository must enforce that
privacy through Git ignore rules, validation rules, and release/packaging
guards rather than by placing the artifact outside the project directory.

## Remote Machine Directory Rule

Remote build or test sessions must stay within the approved per-machine project
directories defined in `test-artefacts/README.md`.

## PowerShell Parser-First Rule

Whenever a PowerShell command is about to be executed on the local machine or
on a remote Windows machine, the exact command text must be passed through a
parser first.

Use the tracked parser helper:

```text
tools/Test-PowerShellSyntax.ps1
```

The parser check must succeed before the actual PowerShell command is executed.

## Hosted Runner Evidence Rule

Every hosted matrix lane must complete its build and tests, emit its
machine-readable result manifest, and upload the complete lane bundle. The
The publication gate downloads and re-verifies every bundle before accepting the run.
Screenshots and traces are inspected by hash when produced; visual
capture is required for a lane only when that platform/application supports an
interactive host. Unsupported capture must be recorded explicitly, never
silently treated as a passing visual result.

## Windows Debugger Requirement

Windows 10 and Windows 11 validation machines must have the Windows SDK
Debugging Tools installed. The required debugger entry points are `cdb.exe`
and `windbg.exe`; the harness searches the Windows Kits 10 Debuggers tree,
including its `x64`, `x86`, `arm`, and `arm64` subdirectories. These tools are
used for native crash capture and stack analysis when a compiler-specific
failure needs diagnosis. Install them from Microsoft's official Windows SDK
or Windows SDK Debugging Tools package; the executables themselves are not
copied into or redistributed by this repository.
