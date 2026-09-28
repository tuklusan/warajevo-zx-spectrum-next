#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Verify native desktop archives and pin the hosted package audit proof."""

import argparse
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath
import subprocess
import zipfile

PLATFORMS = ("windows-x86_64", "macos-x86_64", "macos-arm64")
PRODUCT = "Warajevo-ZX-Spectrum-Next"


def committed_bytes(root: Path, relative: str) -> bytes:
    path = PurePosixPath(relative)
    if path.is_absolute() or ".." in path.parts:
        raise SystemExit(f"unsafe fixture path: {relative}")
    return subprocess.check_output(
        ["git", "show", f"HEAD:{path.as_posix()}"], cwd=root,
        stderr=subprocess.PIPE)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--project-commit", required=True)
    parser.add_argument("--run-id", required=True, type=int)
    parser.add_argument("--artifact-root", required=True, type=Path)
    parser.add_argument("--proof-out", required=True, type=Path)
    args = parser.parse_args()
    root = Path.cwd()
    driver = json.loads((root / "test-drivers/desktop-packages.driver.json")
                        .read_text(encoding="utf-8"))
    packages = []
    runners = []
    for platform in PLATFORMS:
        archive_name = f"{PRODUCT}-{platform}.zip"
        archive_path = args.artifact_root / archive_name
        checksum_path = args.artifact_root / f"{archive_name}.sha256"
        manifest_path = args.artifact_root / f"{archive_name}.json"
        runner_path = args.artifact_root / f"runner-{platform}.json"
        audit_path = args.artifact_root / f"runtime-audit-{platform}.json"
        for required in (archive_path, checksum_path, manifest_path,
                         runner_path, audit_path):
            if not required.is_file():
                raise SystemExit(f"package artifact is missing: {required.name}")
        digest = hashlib.sha256(archive_path.read_bytes()).hexdigest()
        if checksum_path.read_text(encoding="ascii").split()[0] != digest:
            raise SystemExit(f"package checksum mismatch: {archive_name}")
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        expected_architecture = platform.removeprefix("windows-").removeprefix("macos-")
        if (manifest.get("platform") != platform or
                manifest.get("architecture") != expected_architecture or
                manifest.get("archiveSha256") != digest or
                (platform.startswith("macos-") and
                 (manifest.get("signed") is not False or
                  manifest.get("notarized") is not False))):
            raise SystemExit(f"package manifest mismatch: {archive_name}")
        with zipfile.ZipFile(archive_path) as archive:
            if archive.testzip() is not None or sorted(archive.namelist()) != manifest["members"]:
                raise SystemExit(f"package inventory mismatch: {archive_name}")
            if any(part.casefold().endswith((
                    ".rom", ".tap", ".tzx", ".sna", ".z80", ".szx",
                    ".mdr", ".dsk", ".mgt"))
                   for name in archive.namelist() for part in PurePosixPath(name).parts):
                raise SystemExit(f"media or firmware found in package: {archive_name}")
        runner = json.loads(runner_path.read_text(encoding="utf-8"))
        audit = json.loads(audit_path.read_text(encoding="utf-8"))
        if (runner.get("id") != platform or
                runner.get("commit") != args.project_commit or
                audit.get("binarySha256") != manifest.get("binarySha256")):
            raise SystemExit(f"runner/dependency audit mismatch: {platform}")
        if platform == "windows-x86_64" and audit.get("status") != "pass":
            raise SystemExit("Windows runtime dependency audit did not pass")
        if platform.startswith("macos-") and not audit.get("dependencies"):
            raise SystemExit(f"macOS dependency inventory is empty: {platform}")
        packages.append({**manifest, "runtimeAudit": audit})
        runners.append(runner)

    fixtures = []
    for record in driver["fixtures"]:
        digest = hashlib.sha256(committed_bytes(root, record["path"])).hexdigest()
        if digest != record["sha256"]:
            raise SystemExit(f"fixture hash mismatch: {record['path']}")
        fixtures.append({"path": record["path"], "sha256": digest})

    proof = {
        "testId": driver["testId"],
        "status": "pass",
        "commit": args.project_commit,
        "runner": "Windows x64 and macOS Intel/Apple Silicon hosted matrix",
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "runId": args.run_id,
        "runners": runners,
        "packages": packages,
        "fixtures": fixtures,
    }
    args.proof_out.parent.mkdir(parents=True, exist_ok=True)
    args.proof_out.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
