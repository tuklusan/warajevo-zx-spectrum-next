<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms, warranty disclaimer, termination,
patent, trademark, and governing-law provisions. -->

# Warajevo ZX Spectrum Next — Project Workflow

This document defines how the project is specified, implemented, tested, and
advanced. It is the workflow companion to the canonical architecture documents.

## 1. Canonical document hierarchy

The active scope is deliberately limited to the first two architectures:

1. `01-warajevo-zx-spectrum-next-architecture.md` is authoritative for the
   deterministic emulator core, machine profiles, timing, memory, ULA, CPU,
   media, peripherals, and host/core boundary.
2. `02-warajevo-zx-spectrum-next-ui-architecture.md` is authoritative for the
   user interface, command registry, orchestration, presentation, input, and
   settings.
3. `04-wzsn-architectures-1-2-developer-tasks.md` translates those documents
   into ordered implementation tasks and acceptance gates.
4. This document defines repository, CI, test-proof, runner, and change-record workflow.

`03-zx48-mic-ear-router-network-architecture.md` is explicitly out of scope.

## 2. Change workflow — main only, no PRs, no branches

All project work is committed directly to `main`. Do not create or use another
branch, and do not open or use a pull request. Keep each commit small and tied
to an Architecture #1/#2 task or change record. Before pushing, identify the
applicable architecture/task, update the implementation and deterministic
tests, update test evidence, and pass the mandatory local pre-push review gate
in Section 3. Push directly to `main`; then resolve the required CI evidence
for that commit before marking its change record ready to close. Automated
review and test output are evidence; the responsible developer makes the final
adjudication.

Every push-triggered GitHub workflow is scoped to `main`. The local pre-push
gate accepts direct updates only from local `main` to remote `main` and checks
every complete tracked pathname, including directory components, for forbidden
terms case-insensitively before contacting GitHub. No branch other than `main`
is created or used. Manual dispatch jobs require `main`; pull-request triggers are
prohibited. The pathname-policy workflow rechecks every tracked pathname at the
exact commit on each direct push to `main` and on manual runs there.

GitHub push rulesets can reject file paths before receipt only for private or
internal repositories on eligible plans. This repository is public, so GitHub
does not offer a server-side path restriction here. The hosted workflow detects
and fails on a violation after the direct push; the local pre-push gate is the
before-push safeguard used for project commits.

## 3. Local pre-push gate

The sole local validation entrypoint is `.githooks/pre-push`. The checkout uses
`core.hooksPath=.githooks`; new checkouts must enable that setting.

The gate runs test-ledger validation, banned-term validation, license-header
validation, local AI source review, and Git LFS validation. Banned matching is
case-insensitive. Source review is mandatory whenever a pushed change contains
source files covered by the reviewer. The hook enforces a 20-minute wall-clock
limit from review start, saves reviewer output under Git metadata, and on
timeout records `STALL`, elapsed time, and the diagnostic log path in
`wzsn-source-review-stall.log` under Git metadata. A completed review permits
the gate to continue; review findings remain advisory and require developer
adjudication. `STALL` is a failed gate, never a review pass. Missing
credentials, reviewer errors, or other validation/tool configuration failures
also reject the push before the remote is contacted.

The local workstation is never a build or test machine. It may run the
pre-push policy checks, but product compilation, emulator execution, media
loading, screenshots, and test-proof generation must run on hosted project
runners.

## 4. Test ledger contract

The repository contains:

```text
tests/          test implementations and fixtures
test-drivers/   authoritative driver manifests
test-results/   pinned proof records
```

For `tests/<base>.<extension>`, the required companions are:

```text
test-drivers/<base>.driver.json
test-results/<base>.json
```

The driver identifies the test command and every fixture path/hash. The proof
must report `status: "pass"` and pin the tested commit, runner, timestamp, and
fixture SHA-256 hashes. Missing, mismatched, or non-passing records reject the
ledger. The validator runs locally and in `test-ledger-gate.yml`.

## 5. Runner workflow

`runner-matrix.yml` exercises these pinned hosted runner families:

| Platform | Workflow label |
|---|---|
| Windows Intel | `windows-2025` |
| Windows ARM64 | `windows-11-arm` |
| macOS Intel | `macos-15-intel` |
| macOS ARM64 | `macos-15` |

The workflow records the requested label, actual runner name, architecture, and
image. It uses architecture-specific bootstrap caches keyed by runner family
and project build inputs, bootstrapping on cache misses. Hosted runner VMs are
ephemeral; cached bootstrap artifacts are not persistent runner identities.

Runner bootstrap fixtures are kept in the visible `ci/runner-bootstrap/`
directory:

```text
ci/runner-bootstrap/
  common/
  windows-x86_64/
  windows-arm64/
  macos-x86_64/
  macos-arm64/
  manifests/
```

The fixtures contain pinned dependency manifests, installation scripts,
toolchain setup, environment checks, and cache-key inputs. Linux runner
bootstrap must include the X11 development/runtime dependencies required by
the emulator host. Windows and macOS use their platform SDKs and native host
dependencies. An unmatched or missing cache causes a fresh bootstrap and a
new manifest record. No local installation substitutes for runner bootstrap.

## 6. Change-record clean-build gate

`cr-build-gate.yml` is the required clean-build check before a change record is
closed. It runs a clean Release build on all four runner families and accepts
declared build entry points such as `src/cmake/CMakeLists.txt`, a repository-root
Visual Studio solution, or a Python project. The CMake build is the current
product entry point. Generated build-tree solutions are not treated as separate
project entry points. The build and all execution happen on hosted runners, not
on the developer workstation.

Required CI checks apply to the exact commit pushed to `main`; there is no merge
or PR stage.

## 7. ROM and difficult-media validation

The emulator requires an approved ROM on the runner. Development and test
checkouts may contain or obtain approved ROM bytes, as this repository currently
does, provided their identity and licensing status are tracked. Runner setup
must verify the declared hash before injection. The restriction applies to
final public release artifacts: ROM bytes must be excluded from those artifacts
unless redistribution rights have been explicitly established.

The acceptance target is every usable tape in `test-media/`, beginning with
Normal authentic-ROM loading and retaining deterministic master-tick, EAR-edge,
terminal-state, and failure-classification evidence. Unsupported or unavailable
inputs are explicit results, never silent passes.

The CPU, bus, ULA, raster, ROM-loader, and tape timing contract is in
`reference/t-state-description.md`.

The runner-facing DIZZY4K proof is manually dispatched by
`.github/workflows/dizzy4k-demo.yml`. It builds the GUI target, starts the
application on the runner, connects through the control port, inserts
`test-media/DIZZY4K.TAP`, resumes execution, waits for the demo interval, and
requests a Spectrum-display PNG through the screenshot command. The process is
terminated by the harness after capture; screenshots remain untracked runner
output and are not committed to the repository.

## 8. Directory structure and layout rules

The repository layout is functional, not decorative. New files must be placed
according to these rules:

```text
docs/design/       canonical architecture, workflow, and design documents
reference/         timing contracts and historical/reference material
src/               project-owned product source and build inputs
tests/             executable tests and test fixtures only
test-drivers/      one driver manifest per test; no test implementation here
test-results/      one proof JSON per test; no unverified or ad-hoc logs here
ci/runner-bootstrap/ runner bootstrap fixtures and dependency manifests
ci/build/            platform build and packaging harnesses
ci/test/             test orchestration and proof-generation harnesses
tools/             general developer/repository maintenance utilities
.github/workflows/ declarative CI workflows only
.githooks/         thin local hook entrypoints and validators only
roms/              approved development/test ROMs, guidance, and identities;
                   final public releases require a separate redistribution check
test-media/        public difficult-media fixtures
issues/            CR tracker and issue evidence
```

Layout rules:

1. Deterministic emulator code belongs under `src/`; host/UI code must not be
   placed in `reference/`, `tests/`, or workflow directories.
2. Architecture and process decisions belong in `docs/design/`; implementation
   instructions must point back to the applicable canonical document and task.
3. Historical or imported material belongs under `reference/` and must not be
   silently compiled into the product.
4. Workflow YAML stays declarative. Non-trivial shell, Python, or PowerShell
   logic belongs under `ci/` or `tools/` and is invoked by the workflow.
5. Hooks stay small and must call reusable validators rather than duplicate CI
   logic.
6. A test implementation is `tests/<base>.<extension>`. Its driver is exactly
   `test-drivers/<base>.driver.json`; its proof is exactly
   `test-results/<base>.json`.
7. Test drivers describe how to run a test; test proofs describe what actually
   passed. Neither may be used as a miscellaneous scratch area.
8. Generated build output, caches, runner manifests, and temporary traces must
   remain untracked. Runner-produced binaries belong under `dist/`; local
   workstations must not produce them.
9. Root-level files are reserved for project-wide policy, licensing, and
   overview metadata. New operational scripts do not belong at the root.
10. Architecture `03-*` material remains outside the active implementation
    scope even if it is present in the repository.

## 9. Repository locations

```text
docs/design/       canonical architecture and workflow documents
reference/         timing and historical/reference material
ci/runner-bootstrap/ runner bootstrap fixtures and dependency manifests
ci/build/          platform build harnesses
ci/test/           test orchestration and proof generation
tools/             general developer/repository utilities
.github/workflows/ declarative CI workflows
.githooks/         thin local hook entrypoints
```

## 10. Definition of ready to close

A change record is ready to close only when the applicable Architecture #1/#2 task is clear,
the product build succeeds on all required runners, relevant tests have
identifiable drivers and passing pinned proofs, ROM/media outcomes are recorded,
no unexplained release-blocking timing/media divergence remains, and the
developer has adjudicated automated review findings.
