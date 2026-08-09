#!/usr/bin/env python3
# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

from __future__ import annotations

import argparse
import base64
import json
import subprocess
import sys
from pathlib import Path

from invoke_remote_harness import (
    REMOTE_MACHINES,
    decode_output,
    extract_zip,
    pull_windows,
    repo_root,
    run_windows,
    ssh_base,
    utc_stamp,
    validate_powershell,
    write_bytes,
    write_text,
    windows_relative_path,
)


def resolve_repo_path(root: Path, candidate: str) -> Path:
    path = Path(candidate)
    if not path.is_absolute():
        path = (root / path).resolve()
    else:
        path = path.resolve()

    if root not in path.parents and path != root:
        raise ValueError("script path must stay within the project directory")

    return path


def ps_single_quote(value: str) -> str:
    return "'" + value.replace("'", "''") + "'"


def build_execution_wrapper(remote_script_path: str, script_args: list[str]) -> str:
    invocation_args = []
    for argument in script_args:
        if argument.startswith("-"):
            invocation_args.append(argument)
        else:
            invocation_args.append(ps_single_quote(argument))
    invocation_suffix = ""
    if invocation_args:
        invocation_suffix = " " + " ".join(invocation_args)
    lines = [
        "$ErrorActionPreference = 'Stop'",
        "$ProgressPreference = 'SilentlyContinue'",
        f"$scriptPath = {ps_single_quote(remote_script_path)}",
        "if (-not (Test-Path -LiteralPath $scriptPath)) {",
        "    throw \"Staged remote script was not found.\"",
        "}",
    ]

    lines.append(f"$result = & $scriptPath{invocation_suffix}")

    lines.extend(
        [
            "$exitCode = 0",
            "if ($result -is [System.Array]) {",
            "    if ($result.Length -gt 0 -and $result[-1] -is [int]) {",
            "        if ($result.Length -gt 1) {",
            "            $result[0..($result.Length - 2)] | Write-Output",
            "        }",
            "        $exitCode = [int]$result[-1]",
            "    } else {",
            "        $result | Write-Output",
            "    }",
            "} elseif ($result -is [int]) {",
            "    $exitCode = [int]$result",
            "} elseif ($null -ne $result) {",
            "    Write-Output $result",
            "}",
            "exit $exitCode",
        ]
    )

    return "\n".join(lines)


def run_windows_with_input(
    machine: dict[str, str],
    script_text: str,
    stdin_text: str,
    root: Path,
) -> subprocess.CompletedProcess[bytes]:
    validate_powershell(root, script_text)
    encoded = base64.b64encode(script_text.encode("utf-16le")).decode("ascii")
    return subprocess.run(
        [
            *ssh_base(root),
            machine["ssh_target"],
            f"powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass -OutputFormat Text -EncodedCommand {encoded}",
        ],
        cwd=root,
        check=False,
        capture_output=True,
        input=stdin_text.encode("utf-8"),
    )


def stage_remote_script(
    machine: dict[str, str],
    script_text: str,
    script_name: str,
    run_id: str,
    root: Path,
) -> tuple[str, subprocess.CompletedProcess[bytes]]:
    remote_rel = windows_relative_path(f".wzsn-harness/{run_id}/{script_name}")
    remote_script_path = str(Path(machine["project_dir"]) / remote_rel).replace("/", "\\")
    stage_wrapper = "\n".join(
        [
            "$ErrorActionPreference = 'Stop'",
            "$ProgressPreference = 'SilentlyContinue'",
            f"$remoteScriptPath = {ps_single_quote(remote_script_path)}",
            "$remoteParent = Split-Path -Parent $remoteScriptPath",
            "$null = New-Item -ItemType Directory -Force -Path $remoteParent",
            "$scriptBody = [Console]::In.ReadToEnd()",
            "[System.IO.File]::WriteAllText($remoteScriptPath, $scriptBody, [System.Text.UTF8Encoding]::new($false))",
            "Write-Output $remoteScriptPath",
            "exit 0",
        ]
    )
    staged = run_windows_with_input(machine, stage_wrapper, script_text, root)
    return remote_script_path, staged


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Run a local PowerShell script on an approved Windows SSH remote."
    )
    parser.add_argument(
        "machine",
        choices=sorted(name for name, machine in REMOTE_MACHINES.items() if machine["kind"] == "windows"),
    )
    parser.add_argument("script_path", help="Repository-local PowerShell script path.")
    parser.add_argument("--run-id", default=utc_stamp())
    parser.add_argument(
        "--pull-dir",
        help="Optional remote project-relative directory to pull back after the script succeeds.",
    )
    parser.add_argument("script_args", nargs=argparse.REMAINDER)
    args = parser.parse_args()

    root = repo_root()
    script_path = resolve_repo_path(root, args.script_path)
    script_text = script_path.read_text(encoding="utf-8")
    validate_powershell(root, script_text)

    machine = REMOTE_MACHINES[args.machine]
    local_dir = root / "test-artefacts" / "remote-runs" / args.machine / args.run_id
    local_dir.mkdir(parents=True, exist_ok=True)

    remote_script_name = f"staged-{script_path.name}"
    remote_script_path, staged = stage_remote_script(machine, script_text, remote_script_name, args.run_id, root)
    write_text(local_dir / "stage-stdout.txt", decode_output(staged.stdout))
    write_text(local_dir / "stage-stderr.txt", decode_output(staged.stderr))

    if staged.returncode != 0:
        session = {
            "machine": args.machine,
            "run_id": args.run_id,
            "script_path": str(script_path),
            "remote_script_path": remote_script_path,
            "stage_returncode": staged.returncode,
        }
        write_text(local_dir / "script-path.txt", str(script_path) + "\n")
        write_text(local_dir / "command.txt", script_text + ("\n" if not script_text.endswith("\n") else ""))
        write_text(local_dir / "session.json", json.dumps(session, indent=2, sort_keys=True) + "\n")
        return staged.returncode

    wrapper = build_execution_wrapper(remote_script_path, args.script_args)
    primary = run_windows(machine, wrapper, root)

    pulled_returncode = None
    if primary.returncode == 0 and args.pull_dir:
        pulled = pull_windows(machine, args.pull_dir, root)
        pulled_returncode = pulled.returncode
        if pulled.returncode == 0:
            zip_path = local_dir / "remote-artefacts.zip"
            pulled_bytes = base64.b64decode(decode_output(pulled.stdout).strip())
            write_bytes(zip_path, pulled_bytes)
            extract_zip(zip_path, local_dir / "unzipped")
        else:
            write_text(local_dir / "pull-stdout.txt", decode_output(pulled.stdout))
            write_text(local_dir / "pull-stderr.txt", decode_output(pulled.stderr))

    write_text(local_dir / "script-path.txt", str(script_path) + "\n")
    write_text(local_dir / "command.txt", script_text + ("\n" if not script_text.endswith("\n") else ""))
    write_text(local_dir / "wrapper.txt", wrapper + "\n")
    write_text(local_dir / "stdout.txt", decode_output(primary.stdout))
    write_text(local_dir / "stderr.txt", decode_output(primary.stderr))

    session = {
        "machine": args.machine,
        "run_id": args.run_id,
        "script_path": str(script_path),
        "remote_script_path": remote_script_path,
        "primary_returncode": primary.returncode,
        "pull_dir": args.pull_dir,
        "pull_returncode": pulled_returncode,
        "stage_returncode": staged.returncode,
    }
    write_text(local_dir / "session.json", json.dumps(session, indent=2, sort_keys=True) + "\n")

    if primary.returncode != 0:
        return primary.returncode

    if pulled_returncode not in (None, 0):
        return pulled_returncode

    return 0


if __name__ == "__main__":
    sys.exit(main())
