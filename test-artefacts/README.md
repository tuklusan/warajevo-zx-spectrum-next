<!--
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
-->

# Test Artefacts and Remote Harness

This directory is the only project-local location for remote build, test, log,
trace, screenshot, and similar execution artefacts pulled back to this machine.

## Privacy rule

Only this `README.md` file is public and committed.

All other contents of `test-artefacts/` are local-only and ignored by Git by
default, including:

- SSH credential material
- private machine connection notes
- pulled test logs
- traces
- screenshots
- packaged binaries
- crash dumps
- private review scope and requirement inputs
- review receipts and precision telemetry

Detailed private acceptance material used by the external review gate belongs
under `test-artefacts/reviewer/requirements/`. It remains ignored, but its
project-relative path and SHA-256 identity are bound into the review packet and
PASS receipt when supplied.

## Local secrets file

When SSH credentials or machine-specific private notes are stored locally, keep
them only under `test-artefacts/`, for example:

```text
test-artefacts/remote-machine-secrets.local.txt
```

## Remote-only execution policy

Never build or test on this local machine.

All build and test commands must be executed remotely, and the returned results
must be copied back into `test-artefacts/`.

Prefer existing SSH key-based login when a remote machine already accepts it.
If password fallback is required, keep the password only in the local private
secrets file under `test-artefacts/`.

## Windows PowerShell parser-first rule

Before executing any PowerShell command locally or on either remote Windows
machine, parse the exact command text first with:

```text
powershell -NoProfile -File tools/Test-PowerShellSyntax.ps1 -CommandText '<command>'
```

For remote Windows execution, parse the inner PowerShell command text before it
is wrapped in SSH transport quoting.

## Approved remote machines

The current lab availability snapshot has all four approved hosts available.
If a host becomes unreachable, record the transport result as environmental
evidence, not a project correctness failure, and continue with the hosted
matrix plus any available lab host.

### linux-x64-lxqt (currently available)

- SSH target: `sanyalnet@192.168.4.76`
- Remote project directory:
  `~/SOFTWARE-DEVELOPMENT/Warajevo-Spectrum-Next`
- Rule:
  always work only inside that directory on the remote machine

Example session shape:

```text
ssh sanyalnet@192.168.4.76 'cd ~/SOFTWARE-DEVELOPMENT/Warajevo-Spectrum-Next && <remote-command>'
```

### windows-10-reference (currently available)

- SSH target: `sanyalnet@192.168.4.75`
- Remote project directory:
  `D:\WarajevoSpectrum.Next`
- Rule:
  never leave that directory and never use any drive other than `D:`

Example session shape:

```text
ssh sanyalnet@192.168.4.75 "powershell -NoProfile -Command \"Set-Location 'D:\\WarajevoSpectrum.Next'; <remote-command>\""
```

### windows-11-laptop (currently available)

- SSH target: `vagab@192.168.4.103`
- Remote project directory:
  `C:\Users\vagab\WarajevoSpectrum.Next`
- Rule:
  never leave that directory and never use any other drive

Example session shape:

```text
ssh vagab@192.168.4.103 "powershell -NoProfile -Command \"Set-Location 'C:\\Users\\vagab\\WarajevoSpectrum.Next'; <remote-command>\""
```

### macos-bigsur-lab (currently available)

- SSH target: `rumtuk@192.168.4.77`
- Remote project directory: `/Users/rumtuk/SOFTWARE_DEV/WARAJEVO-NEXT`
- Rule: every file operation, build, test, and pulled-result staging operation
  must remain below this directory; total occupied space must never exceed 1 GiB
- Rule: use existing key-based SSH where available; never place the supplied
  password in tracked files, command arguments, logs, or environment captures
- Harness key: local-only `test-artefacts/ssh-private/macos-bigsur`

The harness checks the directory usage before and after every Mac operation and
fails closed if the 1 GiB limit is exceeded.

## Pull-back rule

Any logs, traces, screenshots, packaged builds, or other remote outputs copied
back to this local machine must be stored only under `test-artefacts/`.

The baseline harness stores pulled outputs under:

```text
test-artefacts/remote-runs/<machine>/<run-id>/
```

The tracked harness entry points that produce those pull-backs live under:

```text
tools/harness/
```

## Hosted runner rule

GitHub-hosted runners used by this repository's tracked workflows are approved
remote build and test machines.

The current hosted smoke matrix is defined in:

```text
.github/workflows/platform-smoke.yml
```

When hosted-runner logs, screenshots, or packaged artefacts are pulled back to
this machine for inspection, they must also be stored only under
`test-artefacts/`.

Hosted-runner waiting is fail-closed and terminal-state based. Start one
validated run for the exact published commit, set `fail-fast: false`, and wait
for every configured lane, including lanes that are queued or slow. Do not
cancel, replace, or label a run failed because a lane has not started yet.
Only GitHub's terminal job conclusions may classify a lane, and the
publication gate must inspect all expected lanes and the exact commit SHA.
