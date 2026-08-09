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

## Before commit

Run the repository gates locally:

```text
python tools/validate_project_gates.py
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Typical publish flow

```text
git status
git add .
git commit -m "Initial repository baseline"
git push -u origin main
```

## Reference sync

To inspect the preserved upstream reference without merging it into the build:

```text
git fetch upstream
git log --oneline upstream/HEAD -n 1
```
