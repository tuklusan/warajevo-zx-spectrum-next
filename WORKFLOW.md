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
`design/deepseek-review-gate.md`. Required documentation and post-test artifact
reviews are also mandatory. Serious findings are advisory to the developer but
may not be silently ignored; corrections or evidence-backed disputed
dispositions require a complete new review.

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

## Private Local Artifacts

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
