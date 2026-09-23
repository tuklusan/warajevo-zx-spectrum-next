#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Validate cross-platform command-boundary output and write its pinned proof."""

import argparse
import hashlib
import json
import subprocess
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath


RUNNERS = ("windows-x64", "windows-arm64", "macos-intel", "macos-arm64")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--project-commit", required=True)
    parser.add_argument("--artifact-root", required=True, type=Path)
    parser.add_argument("--proof-out", required=True, type=Path)
    args = parser.parse_args()

    root = Path.cwd()
    driver_path = root / "test-drivers/application-command-boundary.driver.json"
    driver = json.loads(driver_path.read_text(encoding="utf-8"))
    records = []
    for runner_id in RUNNERS:
        runner_dir = args.artifact_root / runner_id
        metadata = json.loads((runner_dir / "runner.json").read_text(encoding="utf-8"))
        log = (runner_dir / "run.log").read_text(encoding="utf-8")
        if metadata.get("id") != runner_id:
            raise SystemExit(f"runner metadata mismatch: {runner_id}")
        if "PASS application-command-boundary cases=4" not in log:
            raise SystemExit(f"acceptance cases did not pass: {runner_id}")
        records.append(metadata)

    fixtures = []
    for record in driver["fixtures"]:
        relative = PurePosixPath(record["path"])
        if relative.is_absolute() or ".." in relative.parts:
            raise SystemExit(f"fixture path is not repository relative: {record['path']}")
        committed = subprocess.check_output(
            ["git", "show", f"HEAD:{relative.as_posix()}"],
            cwd=root,
            stderr=subprocess.PIPE,
        )
        digest = hashlib.sha256(committed).hexdigest()
        if record["sha256"] != "PENDING" and record["sha256"] != digest:
            raise SystemExit(f"pinned fixture changed: {record['path']}")
        fixtures.append({"path": record["path"], "sha256": digest})

    proof = {
        "testId": driver["testId"],
        "status": "pass",
        "commit": args.project_commit,
        "runner": "Windows x64/ARM64 and macOS Intel/ARM64 hosted matrix",
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "caseCount": 4,
        "runners": records,
        "fixtures": fixtures,
    }
    args.proof_out.parent.mkdir(parents=True, exist_ok=True)
    args.proof_out.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
