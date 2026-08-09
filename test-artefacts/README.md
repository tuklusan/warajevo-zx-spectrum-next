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

### linux-x64-lxqt

- SSH target: `sanyalnet@192.168.4.76`
- Remote project directory:
  `~/SOFTWARE-DEVELOPMENT/Warajevo-Spectrum-Next`
- Rule:
  always work only inside that directory on the remote machine

Example session shape:

```text
ssh sanyalnet@192.168.4.76 'cd ~/SOFTWARE-DEVELOPMENT/Warajevo-Spectrum-Next && <remote-command>'
```

### windows-10-reference

- SSH target: `sanyalnet@192.168.4.75`
- Remote project directory:
  `D:\WarajevoSpectrum.Next`
- Rule:
  never leave that directory and never use any drive other than `D:`

Example session shape:

```text
ssh sanyalnet@192.168.4.75 "powershell -NoProfile -Command \"Set-Location 'D:\\WarajevoSpectrum.Next'; <remote-command>\""
```

### windows-11-laptop

- SSH target: `vagab@192.168.4.35`
- Remote project directory:
  `C:\Users\vagab\WarajevoSpectrum.Next`
- Rule:
  never leave that directory and never use any other drive

Example session shape:

```text
ssh vagab@192.168.4.35 "powershell -NoProfile -Command \"Set-Location 'C:\\Users\\vagab\\WarajevoSpectrum.Next'; <remote-command>\""
```

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
