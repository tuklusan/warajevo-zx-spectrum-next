#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

import argparse
import hashlib
import json
import os
import subprocess
from datetime import datetime, timezone
from pathlib import Path


DRIVER_PATH = "test-drivers/multi-instance-linux.driver.json"
EXPECTED_OUTPUT = (
    "PASS concurrent control ports: 12",
    "PASS concurrent settings writes are atomic",
    "PASS settings contention recovery",
    "PASS exclusive writable media contention",
    "PASS concurrent screenshot outputs: 12",
    "PASS multi-instance stress cases: 5",
)


def git_blob(commit, path):
    return subprocess.run(
        ["git", "show", f"{commit}:{path}"],
        check=True,
        stdout=subprocess.PIPE,
    ).stdout


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--artifact-root", required=True, type=Path)
    parser.add_argument("--proof-out", required=True, type=Path)
    args = parser.parse_args()
    commit = os.environ["GITHUB_SHA"]
    driver = json.loads(git_blob(commit, DRIVER_PATH))
    if driver.get("testId") != "multi-instance-linux":
        raise SystemExit("Linux multi-instance driver identity mismatch")

    fixtures = []
    for fixture in driver["fixtures"]:
        digest = hashlib.sha256(git_blob(commit, fixture["path"])).hexdigest()
        if digest != fixture["sha256"]:
            raise SystemExit(f"fixture hash mismatch: {fixture['path']}")
        fixtures.append({"path": fixture["path"], "sha256": digest})
    fixtures.append({
        "path": DRIVER_PATH,
        "sha256": hashlib.sha256(git_blob(commit, DRIVER_PATH)).hexdigest(),
    })

    results = []
    for directory in sorted(args.artifact_root.glob("multi-instance-linux-raw-*")):
        runner_path = directory / "runner.json"
        log_path = directory / "run.log"
        if not runner_path.is_file() or not log_path.is_file():
            raise SystemExit(f"runner artifact incomplete: {directory.name}")
        runner = json.loads(runner_path.read_text(encoding="utf-8"))
        lines = set(log_path.read_text(encoding="utf-8").splitlines())
        missing = [line for line in EXPECTED_OUTPUT if line not in lines]
        if missing:
            raise SystemExit(f"required stress output missing for {runner['id']}")
        results.append({
            "id": runner["id"],
            "os": runner["os"],
            "architecture": runner["architecture"],
            "runnerName": runner["runnerName"],
            "status": "pass",
            "caseCount": 5,
        })

    expected = {"linux-x64": "X64", "linux-arm64": "ARM64"}
    expected_ids = set(expected)
    if {result["id"] for result in results} != expected_ids:
        raise SystemExit("Linux stress matrix is incomplete or duplicated")
    for result in results:
        if result["os"] != "Linux" or \
                result["architecture"] != expected[result["id"]]:
            raise SystemExit(f"runner identity mismatch: {result['id']}")
    proof = {
        "testId": driver["testId"],
        "status": "pass",
        "commit": commit,
        "runId": int(os.environ["GITHUB_RUN_ID"]),
        "runner": "Linux x86-64 and AArch64 hosted runners",
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "caseCount": sum(result["caseCount"] for result in results),
        "runnerResults": results,
        "fixtures": fixtures,
    }
    args.proof_out.parent.mkdir(parents=True, exist_ok=True)
    args.proof_out.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
