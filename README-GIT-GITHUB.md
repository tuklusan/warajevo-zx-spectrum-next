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

Do not build or test on this local machine. Remote build and test harness rules
are documented in `test-artefacts/README.md`.

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
pull back logs, traces, and other outputs into `test-artefacts/`.

## Reference sync

To inspect the preserved upstream reference without merging it into the build:

```text
git fetch upstream
git log --oneline upstream/HEAD -n 1
```
