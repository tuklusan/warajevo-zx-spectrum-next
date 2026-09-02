<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Git and GitHub Workflow

## Remotes

This repository uses:

- `origin` for the active project repository:
  `https://github.com/tuklusan/warajevo-zx-spectrum-next.git`
- `upstream` for the preserved Warajevo 2.50 reference repository:
  `https://github.com/tuklusan/warajevo-spectrum-2.50.git`

## Local setup

The repository is initialized on the `main` branch and uses
`.githooks/` as its hooks path. After cloning on another machine, run:

```text
git config core.hooksPath .githooks
```

## PowerShell parser-first rule

Before executing any PowerShell command on the local machine or on a remote
Windows machine, parse the exact command text first with:

```text
powershell -NoProfile -File tools/Test-PowerShellSyntax.ps1 -CommandText '<command>'
```

Only execute the command after the parser succeeds.

## Before commit

Run the repository gates locally:

```text
python tools/validate_project_gates.py
```

Those gates also verify CR numbering and the public remote-machine workflow
contract across the repository documents and harness metadata.

Do not build or test on this local machine. Remote build and test harness rules
are documented in `test-artefacts/README.md`.

## Remote harness baseline

The tracked remote harness entry points live in:

```text
tools/harness/
```

Use the local orchestrator to run a remote probe, smoke build, or screenshot
capture without leaving the approved project directories on the remote
machines:

```text
python tools/harness/invoke_remote_harness.py probe linux-x64-lxqt
python tools/harness/invoke_remote_harness.py smoke windows-10-reference
python tools/harness/invoke_remote_harness.py screenshot windows-11-laptop
python tools/harness/invoke_remote_harness.py probe macos-bigsur-lab
```

For parser-first Windows maintenance or verification scripts, use:

```text
python tools/harness/invoke_remote_windows_powershell.py windows-11-laptop tools/harness/windows-toolchain/verify-toolchain.ps1
```

Pulled-back outputs are stored only under:

```text
test-artefacts/remote-runs/
```

The harness also defines the shared CMake smoke entry point used by the GitHub
Actions workflow:

```text
.github/workflows/platform-smoke.yml
```

As of 2026-08-09, the baseline GitHub-hosted runner matrix is pinned to the
current standard labels verified from the official GitHub Actions runner
reference: all currently documented hosted labels, including Ubuntu 22.04/24.04/
26.04 x64 and ARM, Windows 2022/2025/2025-vs2026 and Windows ARM, and macOS
13/14/15/26 Intel and ARM variants (including the documented Xcode 27 preview).
The baseline labels remain `ubuntu-24.04`, `ubuntu-24.04-arm`, `windows-2025`,
`macos-15`, and `macos-15-intel`.

## Private Difficult Media

The architecture's difficult TAP, TZX, SNA, Z80, MDR, and related regression
corpus lives in the repository-root directory:

```text
WZSN-PRIVATE-TEST-MEDIA/
```

Only the public guidance files in that directory are committed. Private media
files placed there remain ignored by Git and must not be copied into release
artifacts or source archives.

Use the project-local directory through environment configuration:

```text
WZSN_PRIVATE_TEST_MEDIA=./WZSN-PRIVATE-TEST-MEDIA
```

Do not commit a developer-specific absolute path to the repository. The root
`.gitignore` file and `WZSN-PRIVATE-TEST-MEDIA/.gitignore` together ensure that
`README.md` and `.gitignore` are public while everything else in that directory
stays local-only by default.

## Typical publish flow

```text
git status
git add .
git commit -m "<change summary>"
git push
```

After a push, run build and test work only on the approved remote machines, and
pull back logs, traces, screenshots, and other outputs into `test-artefacts/`.

Pushes and pull requests also trigger the hosted smoke matrix in
`.github/workflows/platform-smoke.yml`. Those jobs are remote CI execution and
are allowed by the project workflow rules.

## Reference sync

To inspect the preserved upstream reference without merging it into the build:

```text
git fetch upstream
git log --oneline upstream/HEAD -n 1
```
