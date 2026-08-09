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
import os
import shlex
import shutil
import subprocess
import sys
import tempfile
import zipfile
from datetime import datetime, timezone
from pathlib import Path


REMOTE_MACHINES = {
    "linux-x64-lxqt": {
        "kind": "linux",
        "ssh_target": "sanyalnet@192.168.4.76",
        "project_dir": "/home/sanyalnet/SOFTWARE-DEVELOPMENT/Warajevo-Spectrum-Next",
        "python_command": "python3",
    },
    "windows-10-reference": {
        "kind": "windows",
        "ssh_target": "sanyalnet@192.168.4.75",
        "project_dir": r"D:\WarajevoSpectrum.Next",
        "python_command": "py -3",
    },
    "windows-11-laptop": {
        "kind": "windows",
        "ssh_target": "vagab@192.168.4.35",
        "project_dir": r"C:\Users\vagab\WarajevoSpectrum.Next",
        "python_command": "py -3",
    },
}


def utc_stamp() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def known_hosts_option(root: Path) -> list[str]:
    path = root / "test-artefacts" / "ssh-known-hosts.local"
    if not path.exists():
        return []
    return ["-o", "UserKnownHostsFile=test-artefacts/ssh-known-hosts.local"]


def ssh_base(root: Path) -> list[str]:
    return ["ssh", "-o", "BatchMode=yes", "-o", "ConnectTimeout=10", *known_hosts_option(root)]


def decode_output(payload: bytes) -> str:
    return payload.decode("utf-8", errors="replace")


def write_bytes(path: Path, content: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(content)


def write_text(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def powershell_executable() -> str:
    system_root = os.environ.get("SystemRoot")
    if system_root:
        candidate = Path(system_root) / "System32" / "WindowsPowerShell" / "v1.0" / "powershell.exe"
        if candidate.exists():
            return str(candidate)

    for executable in ("powershell", "pwsh"):
        resolved = shutil.which(executable)
        if resolved:
            return resolved

    return "powershell"


def validate_powershell(root: Path, script_text: str) -> None:
    parser_script = root / "tools" / "Test-PowerShellSyntax.ps1"
    temp_dir = root / "test-artefacts" / ".parser-tmp"
    temp_dir.mkdir(parents=True, exist_ok=True)

    with tempfile.NamedTemporaryFile(
        mode="w",
        encoding="utf-8",
        suffix=".ps1",
        dir=temp_dir,
        delete=False,
    ) as handle:
        handle.write(script_text)
        temp_path = Path(handle.name)

    try:
        result = subprocess.run(
            [
                powershell_executable(),
                "-NoProfile",
                "-File",
                str(parser_script),
                "-FilePath",
                str(temp_path),
            ],
            cwd=root,
            check=False,
            capture_output=True,
            text=True,
        )
    finally:
        temp_path.unlink(missing_ok=True)

    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or result.stdout.strip() or "PowerShell parser validation failed")


def run_linux(machine: dict[str, str], shell_command: str, root: Path) -> subprocess.CompletedProcess[bytes]:
    remote_command = f"cd {shlex.quote(machine['project_dir'])} && {shell_command}"
    return subprocess.run(
        [*ssh_base(root), machine["ssh_target"], remote_command],
        cwd=root,
        check=False,
        capture_output=True,
    )


def run_windows(machine: dict[str, str], script_text: str, root: Path) -> subprocess.CompletedProcess[bytes]:
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
    )


def windows_relative_path(remote_dir: str) -> str:
    return remote_dir.replace("/", "\\")


def pull_linux(machine: dict[str, str], remote_dir: str, root: Path) -> subprocess.CompletedProcess[bytes]:
    shell_command = (
        f"{machine['python_command']} tools/harness/stream_zip_tree.py {shlex.quote(remote_dir)}"
    )
    return run_linux(machine, shell_command, root)


def pull_windows(machine: dict[str, str], remote_dir: str, root: Path) -> subprocess.CompletedProcess[bytes]:
    script_text = "\n".join(
        [
            "$ErrorActionPreference = 'Stop'",
            "$ProgressPreference = 'SilentlyContinue'",
            f"Set-Location '{machine['project_dir']}'",
            f"& {machine['python_command']} '.\\tools\\harness\\stream_zip_tree.py' --base64 '.\\{windows_relative_path(remote_dir)}'",
            "exit $LASTEXITCODE",
        ]
    )
    return run_windows(machine, script_text, root)


def extract_zip(zip_path: Path, target_dir: Path) -> None:
    target_dir.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(zip_path) as archive:
        archive.extractall(target_dir)


def main() -> int:
    parser = argparse.ArgumentParser(description="Invoke the shared remote harness.")
    parser.add_argument("action", choices=("probe", "smoke", "screenshot"))
    parser.add_argument("machine", choices=sorted(REMOTE_MACHINES))
    parser.add_argument("--run-id", default=utc_stamp())
    args = parser.parse_args()

    root = repo_root()
    machine = REMOTE_MACHINES[args.machine]
    remote_dir = f".wzsn-harness/{args.run_id}"
    remote_dir_windows = windows_relative_path(remote_dir)
    local_dir = root / "test-artefacts" / "remote-runs" / args.machine / args.run_id
    local_dir.mkdir(parents=True, exist_ok=True)

    if machine["kind"] == "linux":
        if args.action == "probe":
            primary = run_linux(
                machine,
                f"{machine['python_command']} tools/harness/run_cmake_smoke.py --artifact-dir {shlex.quote(remote_dir)} --probe-only",
                root,
            )
            command_text = f"{machine['python_command']} tools/harness/run_cmake_smoke.py --artifact-dir {remote_dir} --probe-only"
        elif args.action == "smoke":
            primary = run_linux(
                machine,
                f"{machine['python_command']} tools/harness/run_cmake_smoke.py --artifact-dir {shlex.quote(remote_dir)}",
                root,
            )
            command_text = f"{machine['python_command']} tools/harness/run_cmake_smoke.py --artifact-dir {remote_dir}"
        else:
            primary = run_linux(
                machine,
                f"bash tools/harness/capture-linux-active-display.sh {shlex.quote(remote_dir + '/desktop-screenshot.png')}",
                root,
            )
            command_text = f"bash tools/harness/capture-linux-active-display.sh {remote_dir}/desktop-screenshot.png"

        pulled = pull_linux(machine, remote_dir, root)
    else:
        if args.action == "probe":
            script_text = "\n".join(
                [
                    "$ErrorActionPreference = 'Stop'",
                    "$ProgressPreference = 'SilentlyContinue'",
                    f"Set-Location '{machine['project_dir']}'",
                    f"& {machine['python_command']} '.\\tools\\harness\\run_cmake_smoke.py' --artifact-dir '.\\{remote_dir_windows}' --probe-only",
                    "exit $LASTEXITCODE",
                ]
            )
        elif args.action == "smoke":
            script_text = "\n".join(
                [
                    "$ErrorActionPreference = 'Stop'",
                    "$ProgressPreference = 'SilentlyContinue'",
                    f"Set-Location '{machine['project_dir']}'",
                    f"& {machine['python_command']} '.\\tools\\harness\\run_cmake_smoke.py' --artifact-dir '.\\{remote_dir_windows}'",
                    "exit $LASTEXITCODE",
                ]
            )
        else:
            script_text = "\n".join(
                [
                    "$ErrorActionPreference = 'Stop'",
                    "$ProgressPreference = 'SilentlyContinue'",
                    f"Set-Location '{machine['project_dir']}'",
                    f"& '.\\tools\\harness\\Capture-WindowsDesktopScreenshot.ps1' -OutputPath '.\\{remote_dir_windows}\\desktop-screenshot.png'",
                    "exit $LASTEXITCODE",
                ]
            )

        primary = run_windows(machine, script_text, root)
        command_text = script_text
        pulled = pull_windows(machine, remote_dir, root)

    write_text(local_dir / "command.txt", command_text + "\n")
    write_text(local_dir / "stdout.txt", decode_output(primary.stdout))
    write_text(local_dir / "stderr.txt", decode_output(primary.stderr))

    if pulled.returncode == 0:
        zip_path = local_dir / "remote-artefacts.zip"
        pulled_bytes = pulled.stdout
        if machine["kind"] == "windows":
            pulled_bytes = base64.b64decode(decode_output(pulled.stdout).strip())
        write_bytes(zip_path, pulled_bytes)
        extract_zip(zip_path, local_dir / "unzipped")
    else:
        write_text(local_dir / "pull-stdout.txt", decode_output(pulled.stdout))
        write_text(local_dir / "pull-stderr.txt", decode_output(pulled.stderr))

    session_record = {
        "action": args.action,
        "machine": args.machine,
        "run_id": args.run_id,
        "primary_returncode": primary.returncode,
        "pull_returncode": pulled.returncode,
        "local_dir": str(local_dir),
        "remote_dir": remote_dir,
    }
    write_text(local_dir / "session.json", json.dumps(session_record, indent=2, sort_keys=True) + "\n")

    if primary.returncode != 0:
        return primary.returncode

    if pulled.returncode != 0:
        return pulled.returncode

    return 0


if __name__ == "__main__":
    sys.exit(main())
