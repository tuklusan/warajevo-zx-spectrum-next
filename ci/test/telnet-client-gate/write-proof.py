#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Validate the hosted Telnet client gate matrix and pin its proof."""

import argparse
import hashlib
import json
import subprocess
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath

RUNNERS = ("windows-x64", "windows-arm64", "macos-x64", "macos-arm64")
PASS_MARKER = "Telnet client gate regression passed ("


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--project-commit", required=True)
    parser.add_argument("--run-id", required=True, type=int)
    parser.add_argument("--artifact-root", required=True, type=Path)
    parser.add_argument("--proof-out", required=True, type=Path)
    args = parser.parse_args()
    root = Path.cwd()
    driver = json.loads((root / "test-drivers/telnet-client-gate.driver.json")
                        .read_text(encoding="utf-8"))
    runners = []
    for runner_id in RUNNERS:
        runner_dir = args.artifact_root / runner_id
        metadata = json.loads((runner_dir / "runner.json").read_text(encoding="utf-8"))
        output = (runner_dir / "run.log").read_text(encoding="utf-8")
        if metadata.get("id") != runner_id or PASS_MARKER not in output:
            raise SystemExit(f"runner proof missing or mismatched: {runner_id}")
        count = output.split(PASS_MARKER, 1)[1].split(" ", 1)[0]
        if not count.isdigit() or int(count) != driver["requiredCases"]:
            raise SystemExit(f"case count mismatch: {runner_id}")
        runners.append(metadata)
    fixtures = []
    for record in driver["fixtures"]:
        path = PurePosixPath(record["path"])
        if path.is_absolute() or ".." in path.parts:
            raise SystemExit(f"unsafe fixture path: {record['path']}")
        content = subprocess.check_output(["git", "show", f"HEAD:{path.as_posix()}"], cwd=root)
        digest = hashlib.sha256(content).hexdigest()
        if digest != record["sha256"]:
            raise SystemExit(f"fixture hash mismatch: {record['path']}")
        fixtures.append({"path": record["path"], "sha256": digest})
    proof = {
        "testId": driver["testId"], "status": "pass",
        "commit": args.project_commit,
        "runner": "Windows x64/ARM64 and macOS Intel/ARM64 hosted matrix",
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "runId": args.run_id, "runners": runners, "fixtures": fixtures,
    }
    args.proof_out.parent.mkdir(parents=True, exist_ok=True)
    args.proof_out.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
