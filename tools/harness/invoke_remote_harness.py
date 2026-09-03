#!/usr/bin/env python3
# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

from __future__ import annotations

import argparse
import base64
import binascii
import hashlib
import io
import json
import os
import re
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
        "ssh_target": "sanyalnet@10.0.0.161",
        "project_dir": "/home/sanyalnet/SOFTWARE-DEVELOPMENT/Warajevo-Spectrum-Next",
        "python_command": "python3",
    },
    "windows-10-reference": {
        "kind": "windows",
        "ssh_target": "sanyalnet@10.0.0.106",
        "project_dir": r"D:\WarajevoSpectrum.Next",
        "python_command": "py -3",
    },
    "windows-11-laptop": {
        "kind": "windows",
        "ssh_target": "vagab@10.0.0.133",
        "project_dir": r"C:\Users\vagab\WarajevoSpectrum.Next",
        "python_command": "py -3",
    },
    "macos-bigsur-lab": {
        "kind": "macos",
        "ssh_target": "rumtuk@10.0.0.114",
        "project_dir": "/Users/rumtuk/SOFTWARE_DEV/WARAJEVO-NEXT",
        "python_command": "python3",
        "max_bytes": 1073741824,
        "identity_file": "test-artefacts/ssh-private/macos-bigsur",
    },
}
REMOTE_REPOSITORY = "https://github.com/tuklusan/warajevo-zx-spectrum-next.git"
SAFE_RUN_ID = re.compile(r"[A-Za-z0-9][A-Za-z0-9._-]{0,63}\Z")
MAINTENANCE_REF = re.compile(r"wzsn/maintenance/CR-[0-9]{4}[A-Za-z0-9._/-]*\Z")


def utc_stamp() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")


def validate_run_id(value: str) -> str:
    if not SAFE_RUN_ID.fullmatch(value) or value in {".", ".."} or ".." in value:
        raise SystemExit("run-id must be 1-64 ASCII letters, digits, dots, underscores, or hyphens without '..'")
    return value


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def canonical_json(value) -> str:
    return json.dumps(value, ensure_ascii=True, separators=(",", ":"), sort_keys=True)


def _bound_file(root: Path, value: str) -> Path:
    path = (root / value).resolve()
    try:
        path.relative_to(root.resolve())
    except ValueError as exc:
        raise SystemExit("remote smoke blocked: receipt authority path escapes project") from exc
    if not path.is_file():
        raise SystemExit("remote smoke blocked: receipt authority source is missing")
    return path


def validate_review_authority(root: Path, receipt: dict) -> None:
    protocol_version = receipt.get("review_protocol_version", 0)
    if not isinstance(protocol_version, int):
        raise SystemExit("remote smoke blocked: malformed review protocol version")
    if protocol_version < 2:
        return
    cr_number = receipt.get("cr_number")
    sources = receipt.get("requirement_sources")
    if not isinstance(cr_number, str) or not cr_number or not isinstance(sources, list) or not sources:
        raise SystemExit("remote smoke blocked: protocol-v2 authority binding is incomplete")
    requirement_identity = []
    seen_sources = set()
    for source in sources:
        if (not isinstance(source, dict) or not isinstance(source.get("source"), str)
                or not isinstance(source.get("sha256"), str)):
            raise SystemExit("remote smoke blocked: malformed requirement authority binding")
        if source["source"] in seen_sources:
            raise SystemExit("remote smoke blocked: duplicate requirement authority binding")
        seen_sources.add(source["source"])
        data = _bound_file(root, source["source"]).read_bytes()
        digest = hashlib.sha256(data).hexdigest()
        if digest != source["sha256"]:
            raise SystemExit("remote smoke blocked: requirement authority changed after review")
        requirement_identity.append({"source": source["source"], "sha256": digest})
    requirements_hash = hashlib.sha256(canonical_json(requirement_identity).encode()).hexdigest()
    if requirements_hash != receipt.get("requirements_manifest_hash"):
        raise SystemExit("remote smoke blocked: requirement manifest identity mismatch")

    tracker_path = _bound_file(root, "issues/change-requests.json")
    tracker_data = tracker_path.read_bytes()
    try:
        tracker = json.loads(tracker_data.decode("utf-8", errors="strict"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise SystemExit("remote smoke blocked: CR tracker is invalid") from exc
    items = tracker.get("change_requests", []) if isinstance(tracker, dict) else tracker if isinstance(tracker, list) else []
    if not isinstance(items, list):
        raise SystemExit("remote smoke blocked: CR tracker change_requests is not an array")
    matches = [item for item in items if isinstance(item, dict) and item.get("cr_number") == cr_number]
    if len(matches) != 1 or matches[0].get("status") != "in_progress":
        raise SystemExit("remote smoke blocked: reviewed CR is not uniquely active")
    item = matches[0]
    scope = {
        "cr_number": cr_number,
        "title": item.get("title"),
        "status": item.get("status"),
        "source_authority": item.get("source_authority", []),
        "notes": item.get("notes", ""),
        "tracker_source": "issues/change-requests.json",
        "tracker_sha256": hashlib.sha256(tracker_data).hexdigest(),
        "record_sha256": hashlib.sha256(canonical_json(item).encode()).hexdigest(),
    }
    private_source = receipt.get("scope_private_source")
    if private_source is not None:
        if not isinstance(private_source, str) or not private_source:
            raise SystemExit("remote smoke blocked: malformed private scope binding")
        if private_source not in seen_sources:
            raise SystemExit("remote smoke blocked: private scope is absent from requirement authority")
        private_data = _bound_file(root, private_source).read_bytes()
        scope["private_scope"] = {
            "source": private_source,
            "sha256": hashlib.sha256(private_data).hexdigest(),
            "content": private_data.decode("utf-8", errors="strict"),
        }
    if hashlib.sha256(canonical_json(scope).encode()).hexdigest() != receipt.get("scope_manifest_hash"):
        raise SystemExit("remote smoke blocked: CR scope changed after review")


def require_code_review_pass(root: Path) -> None:
    receipt_path = root / "test-artefacts" / "reviewer" / "code-pass.json"
    if not receipt_path.is_file():
        raise SystemExit("remote smoke blocked: no private CODE PASS receipt")
    try:
        receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise SystemExit("remote smoke blocked: invalid CODE PASS receipt") from exc
    try:
        head = subprocess.run(
            ["git", "rev-parse", "HEAD"], cwd=root, check=True,
            capture_output=True, text=True,
        ).stdout.strip()
    except (subprocess.CalledProcessError, OSError) as exc:
        raise SystemExit("remote smoke blocked: current commit identity unavailable") from exc
    try:
        remote_output = subprocess.run(
            ["git", "ls-remote", "--heads", "origin", "main"], cwd=root, check=True,
            capture_output=True, text=True, timeout=30,
        ).stdout.strip()
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired, OSError) as exc:
        raise SystemExit("remote smoke blocked: published commit identity unavailable") from exc
    fields = remote_output.split()
    if len(fields) != 2 or fields[1] != "refs/heads/main":
        raise SystemExit("remote smoke blocked: authoritative origin/main identity is malformed")
    published_head = fields[0]
    if published_head != head:
        raise SystemExit("remote smoke blocked: reviewed commit is not published on origin/main")
    snapshot = receipt.get("snapshot_id", "")
    if receipt.get("verdict") != "PASS" or receipt.get("review_complete") is not True:
        raise SystemExit("remote smoke blocked: reviewer verdict is not PASS")
    validate_review_authority(root, receipt)
    try:
        without_scheme = snapshot[4:] if snapshot.startswith("git:") else snapshot
        commit_range, marker, recorded_digest = without_scheme.rpartition(":sha256:")
        base, separator, reviewed_head = commit_range.partition("..")
    except (AttributeError, ValueError) as exc:
        raise SystemExit("remote smoke blocked: malformed CODE PASS snapshot") from exc
    if marker != ":sha256:" or not separator or reviewed_head != head:
        raise SystemExit("remote smoke blocked: CODE PASS does not match current commit")
    try:
        diff = subprocess.run(
            ["git", "diff", "--no-ext-diff", "--unified=80", base, reviewed_head],
            cwd=root, check=True, capture_output=True,
        ).stdout
    except (subprocess.CalledProcessError, OSError) as exc:
        raise SystemExit("remote smoke blocked: reviewed diff identity unavailable") from exc
    if hashlib.sha256(diff).hexdigest() != recorded_digest:
        raise SystemExit("remote smoke blocked: CODE PASS diff identity mismatch")


def require_bootstrap_maintenance_pass(root: Path, cr_number: str, published_ref: str) -> None:
    if not MAINTENANCE_REF.fullmatch(published_ref) or cr_number not in published_ref:
        raise SystemExit("remote smoke blocked: bootstrap maintenance ref is invalid")
    receipt_path = root / "test-artefacts" / "reviewer" / "bootstrap-pass.json"
    try:
        receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise SystemExit("remote smoke blocked: bootstrap PASS receipt is missing or invalid") from exc
    if receipt.get("verdict") != "PASS" or receipt.get("review_complete") is not True or receipt.get("cr_number") != cr_number:
        raise SystemExit("remote smoke blocked: bootstrap PASS receipt is not authorized for this CR")
    sources = receipt.get("requirement_sources")
    if not isinstance(sources, list) or len(sources) != 1 or sources[0].get("source") != "design/deepseek-review-gate.md":
        raise SystemExit("remote smoke blocked: bootstrap authority is not the review-gate specification")
    source = root / "design" / "deepseek-review-gate.md"
    if hashlib.sha256(source.read_bytes()).hexdigest() != sources[0].get("sha256"):
        raise SystemExit("remote smoke blocked: bootstrap authority changed after review")
    head = run_git(root, "rev-parse", "HEAD")
    published = run_git(root, "ls-remote", "--heads", "origin", published_ref).split()
    if len(published) != 2 or published[0] != head:
        raise SystemExit("remote smoke blocked: maintenance candidate is not published exactly")
    snapshot = str(receipt.get("snapshot_id", ""))
    if not snapshot.startswith("git:") or f"..{head}:sha256:" not in snapshot:
        raise SystemExit("remote smoke blocked: bootstrap PASS does not match current commit")


def run_git(root: Path, *args: str) -> str:
    try:
        return subprocess.run(["git", *args], cwd=root, check=True, capture_output=True, text=True, timeout=30).stdout.strip()
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired, OSError) as exc:
        raise SystemExit("remote smoke blocked: git identity check failed") from exc


def known_hosts_option(root: Path) -> list[str]:
    path = root / "test-artefacts" / "ssh-known-hosts.local"
    if not path.exists():
        return []
    return ["-o", "UserKnownHostsFile=test-artefacts/ssh-known-hosts.local"]


def ssh_base(root: Path, machine: dict[str, str]) -> list[str]:
    command = ["ssh", "-o", "BatchMode=yes", "-o", "ConnectTimeout=30", *known_hosts_option(root)]
    identity_file = machine.get("identity_file")
    if identity_file:
        command.extend(["-i", identity_file, "-o", "IdentitiesOnly=yes"])
    return command


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
    remote_command = (
        f"cd {shlex.quote(machine['project_dir'])} && "
        f"usage_kb=$(du -sk . | awk '{{print $1}}') && "
        f"test \"$usage_kb\" -le {machine.get('max_bytes', 0) // 1024 or 1024 * 1024} && "
        f"{shell_command} && "
        f"usage_kb=$(du -sk . | awk '{{print $1}}') && "
        f"test \"$usage_kb\" -le {machine.get('max_bytes', 0) // 1024 or 1024 * 1024}"
    )
    return subprocess.run(
        [*ssh_base(root, machine), machine["ssh_target"], remote_command],
        cwd=root,
        check=False,
        capture_output=True,
    )


def sync_linux(machine: dict[str, str], root: Path, published_ref: str | None = None) -> subprocess.CompletedProcess[bytes]:
    bootstrap = (
        f"if [ ! -d .git ]; then git clone {shlex.quote(REMOTE_REPOSITORY)} .; fi"
    )
    update = (f"git fetch origin {shlex.quote(published_ref)} && git checkout --detach FETCH_HEAD" if published_ref
              else "git checkout main && git pull --ff-only origin main")
    command = f"{bootstrap} && {update}"
    return run_linux(machine, command, root)


def sync_windows(machine: dict[str, str], root: Path, published_ref: str | None = None) -> subprocess.CompletedProcess[bytes]:
    script_text = "\n".join(
        [
            "$ErrorActionPreference = 'Stop'",
            f"Set-Location '{machine['project_dir']}'",
            "$env:Path = 'C:\\Program Files\\CMake\\bin;C:\\Program Files\\Git\\cmd;C:\\Program Files\\LLVM\\bin;C:\\ProgramData\\mingw64\\mingw64\\bin;' + $env:Path",
            "$git = @((Get-Command git.exe -ErrorAction SilentlyContinue).Source, 'C:\\Program Files\\Git\\cmd\\git.exe', 'C:\\Program Files\\Git\\bin\\git.exe', 'C:\\Program Files\\Microsoft Visual Studio\\2022\\BuildTools\\Common7\\IDE\\CommonExtensions\\Microsoft\\TeamFoundation\\Team Explorer\\Git\\cmd\\git.exe') | Where-Object { $_ -and (Test-Path -LiteralPath $_) } | Select-Object -First 1",
            "if ($git) {",
            "    & $git stash push --include-untracked --message 'wzsn-harness-preserved-remote-state' | Out-Null",
            (f"    & $git fetch origin '{published_ref}'\n    & $git checkout --detach FETCH_HEAD" if published_ref
             else "    & $git checkout main\n    & $git pull --ff-only origin main"),
            "} else {",
            "    $archive = Join-Path (Get-Location) '.wzsn-source.zip'",
            "    $extract = Join-Path (Get-Location) '.wzsn-source-extract'",
            "    if (Test-Path -LiteralPath $extract) { Remove-Item -LiteralPath $extract -Recurse -Force }",
            "    & curl.exe --fail --location --silent --show-error 'https://github.com/tuklusan/warajevo-zx-spectrum-next/archive/refs/heads/main.zip' --output $archive",
            "    if ($LASTEXITCODE -ne 0) { throw 'source archive download failed' }",
            "    Expand-Archive -LiteralPath $archive -DestinationPath $extract -Force",
            "    $source = Get-ChildItem -LiteralPath $extract -Directory | Select-Object -First 1",
            "    if (-not $source) { throw 'source archive has no root directory' }",
            "    Copy-Item -Path (Join-Path $source.FullName '*') -Destination (Get-Location) -Recurse -Force",
            "    Remove-Item -LiteralPath $archive -Force",
            "    Remove-Item -LiteralPath $extract -Recurse -Force",
            "}",
            "exit $LASTEXITCODE",
        ]
    )
    script_text = script_text.replace("& git ", "& $git ")
    return run_windows(machine, script_text, root)


def run_windows(machine: dict[str, str], script_text: str, root: Path) -> subprocess.CompletedProcess[bytes]:
    validate_powershell(root, script_text)
    encoded = base64.b64encode(script_text.encode("utf-16le")).decode("ascii")
    return subprocess.run(
        [
            *ssh_base(root, machine),
            machine["ssh_target"],
            f"powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass -OutputFormat Text -EncodedCommand {encoded}",
        ],
        cwd=root,
        check=False,
        capture_output=True,
    )


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
            *ssh_base(root, machine),
            machine["ssh_target"],
            f"powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass -OutputFormat Text -EncodedCommand {encoded}",
        ],
        cwd=root,
        check=False,
        capture_output=True,
        input=stdin_text.encode("utf-8"),
    )


def stage_windows_file(
    machine: dict[str, str],
    remote_path: str,
    content: str,
    root: Path,
) -> subprocess.CompletedProcess[bytes]:
    escaped_remote_path = remote_path.replace("'", "''")
    stage_wrapper = "\n".join(
        [
            "$ErrorActionPreference = 'Stop'",
            f"$target = '{escaped_remote_path}'",
            "$parent = Split-Path -Parent $target",
            "$null = New-Item -ItemType Directory -Force -Path $parent",
            "$body = [Console]::In.ReadToEnd()",
            "[System.IO.File]::WriteAllText($target, $body, [System.Text.UTF8Encoding]::new($false))",
            "exit 0",
        ]
    )
    return run_windows_with_input(machine, stage_wrapper, content, root)


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


def decode_windows_archive(payload: bytes) -> bytes:
    """Decode native-command text without weakening ZIP integrity checks."""
    # Native Windows transport can insert a non-data marker into a long line.
    # Strip only characters outside the base64 alphabet; strict decoding and
    # the ZIP checksum below still reject truncation or altered payload data.
    encoded = re.sub(r"[^A-Za-z0-9+/=]", "", decode_output(payload))
    if not encoded or re.fullmatch(r"[A-Za-z0-9+/]*={0,2}", encoded) is None:
        raise SystemExit("remote smoke failed: Windows archive transport is not base64")
    encoded += "=" * ((-len(encoded)) % 4)
    try:
        archive_bytes = base64.b64decode(encoded, validate=True)
        with zipfile.ZipFile(io.BytesIO(archive_bytes)) as archive:
            if archive.testzip() is not None:
                raise ValueError("ZIP member checksum failed")
    except (ValueError, zipfile.BadZipFile, binascii.Error) as exc:
        raise SystemExit("remote smoke failed: Windows archive transport is incomplete") from exc
    return archive_bytes


def main() -> int:
    parser = argparse.ArgumentParser(description="Invoke the shared remote harness.")
    parser.add_argument("action", choices=("probe", "smoke", "screenshot"))
    parser.add_argument("machine", choices=sorted(REMOTE_MACHINES))
    parser.add_argument("--run-id", default=utc_stamp())
    parser.add_argument("--bootstrap-maintenance-cr")
    parser.add_argument("--published-ref")
    parser.add_argument("--sanitizers", action="store_true",
                        help="Enable address and undefined-behavior sanitizers on remote smoke builds.")
    parser.add_argument("--sokol-host", action="store_true",
                        help="Also configure and build the opt-in Sokol host target.")
    args = parser.parse_args()
    args.run_id = validate_run_id(args.run_id)

    root = repo_root()
    if args.action == "smoke":
        if args.bootstrap_maintenance_cr or args.published_ref:
            if not args.bootstrap_maintenance_cr or not args.published_ref:
                raise SystemExit("bootstrap maintenance requires both CR and published ref")
            require_bootstrap_maintenance_pass(root, args.bootstrap_maintenance_cr, args.published_ref)
        else:
            require_code_review_pass(root)
    machine = REMOTE_MACHINES[args.machine]
    remote_dir = f".wzsn-harness/{args.run_id}"
    remote_dir_windows = windows_relative_path(remote_dir)
    local_dir = root / "test-artefacts" / "remote-runs" / args.machine / args.run_id
    local_dir.mkdir(parents=True, exist_ok=True)
    sanitizer_arg = " --sanitizers" if args.sanitizers else ""
    sokol_arg = " --sokol-host" if args.sokol_host else ""

    sync = (sync_linux(machine, root, args.published_ref) if machine["kind"] in {"linux", "macos"}
            else sync_windows(machine, root, args.published_ref))
    write_text(local_dir / "sync-stdout.txt", decode_output(sync.stdout))
    write_text(local_dir / "sync-stderr.txt", decode_output(sync.stderr))
    if sync.returncode != 0:
        write_text(
            local_dir / "session.json",
            json.dumps({"action": args.action, "machine": args.machine,
                        "run_id": args.run_id, "sync_returncode": sync.returncode},
                       indent=2, sort_keys=True) + "\n",
        )
        return sync.returncode

    if machine["kind"] in {"linux", "macos"}:
        if args.action == "probe":
            primary = run_linux(
                machine,
                f"{machine['python_command']} tools/harness/run_cmake_smoke.py --artifact-dir {shlex.quote(remote_dir)} --probe-only",
                root,
            )
            command_text = f"{machine['python_command']} tools/harness/run_cmake_smoke.py --artifact-dir {remote_dir} --probe-only"
        elif args.action == "smoke":
            fuse_command = (
                f"{machine['python_command']} tools/harness/run_fuse_ed_platform.py "
                f"--runner-root {shlex.quote(remote_dir + '/build')} "
                "--private-root test-artefacts/fuse-corpus "
                f"--manifest {shlex.quote(remote_dir + '/fuse-complete-manifest.json')} "
                "--unresolved-baseline tools/harness/fuse-unresolved-baseline.json"
            )
            primary = run_linux(
                machine,
                f"{machine['python_command']} tools/harness/run_cmake_smoke.py --artifact-dir {shlex.quote(remote_dir)}{sanitizer_arg}{sokol_arg} && {fuse_command}",
                root,
            )
            command_text = (
                f"{machine['python_command']} tools/harness/run_cmake_smoke.py --artifact-dir {remote_dir}{sanitizer_arg}{sokol_arg} && "
                f"{fuse_command}"
            )
        else:
            screenshot_command = (
                f"screencapture -x {shlex.quote(remote_dir + '/desktop-screenshot.png')}"
                if machine["kind"] == "macos"
                else f"bash tools/harness/capture-linux-active-display.sh {shlex.quote(remote_dir + '/desktop-screenshot.png')}"
            )
            primary = run_linux(
                machine,
                screenshot_command,
                root,
            )
            command_text = screenshot_command

        pulled = pull_linux(machine, remote_dir, root)
    else:
        script_text = ""
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
                    f"& {machine['python_command']} '.\\tools\\harness\\run_cmake_smoke.py' --artifact-dir '.\\{remote_dir_windows}'{sanitizer_arg}{sokol_arg}",
                    "if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }",
                    f"& {machine['python_command']} '.\\tools\\harness\\run_fuse_ed_platform.py' --runner-root '.\\{remote_dir_windows}\\build' --private-root '.\\test-artefacts\\fuse-corpus' --manifest '.\\{remote_dir_windows}\\fuse-complete-manifest.json' --unresolved-baseline '.\\tools\\harness\\fuse-unresolved-baseline.json'",
                    "exit $LASTEXITCODE",
                ]
            )
        else:
            bridge_path = root / "tools" / "harness" / "Invoke-WindowsInteractiveScreenshot.ps1"
            capture_path = root / "tools" / "harness" / "Capture-WindowsDesktopScreenshot.ps1"
            bridge_source = bridge_path.read_text(encoding="utf-8")
            capture_source = capture_path.read_text(encoding="utf-8")
            validate_powershell(root, bridge_source)
            validate_powershell(root, capture_source)
            remote_bridge = str(Path(machine["project_dir"]) / remote_dir_windows / bridge_path.name).replace("/", "\\")
            remote_capture = str(Path(machine["project_dir"]) / remote_dir_windows / capture_path.name).replace("/", "\\")
            remote_output = str(Path(machine["project_dir"]) / remote_dir_windows / "desktop-screenshot.png").replace("/", "\\")
            stage_bridge = stage_windows_file(machine, remote_bridge, bridge_source, root)
            stage_capture = stage_windows_file(machine, remote_capture, capture_source, root)
            if stage_bridge.returncode != 0:
                primary = stage_bridge
            elif stage_capture.returncode != 0:
                primary = stage_capture
            else:
                script_text = "\n".join(
                    [
                        "$ErrorActionPreference = 'Stop'",
                        "$ProgressPreference = 'SilentlyContinue'",
                        f"Set-Location '{machine['project_dir']}'",
                        f"& '.\\tools\\Test-PowerShellSyntax.ps1' -FilePath '{remote_dir_windows}\\{bridge_path.name}'",
                        "if (-not $?) { exit 1 }",
                        f"& '.\\tools\\Test-PowerShellSyntax.ps1' -FilePath '{remote_dir_windows}\\{capture_path.name}'",
                        "if (-not $?) { exit 1 }",
                        "try {",
                        f"    & '{remote_bridge}' -CaptureScriptPath '{remote_capture}' -OutputPath '{remote_output}'",
                        "    if (-not $?) { exit 1 }",
                        "} catch {",
                        "    Write-Error $_",
                        "    exit 1",
                        "}",
                        "exit 0",
                    ]
                )
                primary = run_windows(machine, script_text, root)
        if args.action != "screenshot":
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
            write_text(local_dir / "pull-stdout.txt", decode_output(pulled.stdout))
            write_text(local_dir / "pull-stderr.txt", decode_output(pulled.stderr))
            pulled_bytes = decode_windows_archive(pulled.stdout)
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
