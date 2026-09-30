#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Validate and pin the hosted eight-frame trace-retention measurement."""

import argparse
import hashlib
import json
import re
import subprocess
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--commit", required=True)
    parser.add_argument("--run-id", required=True, type=int)
    parser.add_argument("--measurement", required=True, type=Path)
    parser.add_argument("--proof", required=True, type=Path)
    args = parser.parse_args()
    if re.fullmatch(r"[0-9a-f]{40}", args.commit) is None:
        raise SystemExit("commit must be a full lowercase SHA-1 object ID")

    root = Path.cwd()
    driver = json.loads((root / "test-drivers/timing-trace-retention.driver.json")
                        .read_text(encoding="utf-8"))
    measurement = json.loads(args.measurement.read_text(encoding="utf-8"))
    required_frames = driver["requiredFrames"]
    if (measurement.get("status") != "pass" or
            measurement.get("requestedFrames") != required_frames or
            measurement.get("retainedFrameSpan", 0) < required_frames or
            measurement.get("fileBytes") != 16777216 or
            measurement.get("retainedRecords", 0) <= 0):
        raise SystemExit("measurement does not prove the required trace retention")

    fixtures = []
    for item in driver["fixtures"]:
        relative = PurePosixPath(item["path"])
        if relative.is_absolute() or ".." in relative.parts:
            raise SystemExit(f"unsafe fixture path: {item['path']}")
        committed = subprocess.check_output(
            ["git", "show", f"{args.commit}:{relative.as_posix()}"],
            cwd=root, stderr=subprocess.PIPE)
        digest = hashlib.sha256(committed).hexdigest()
        if digest != item["sha256"]:
            raise SystemExit(f"fixture hash mismatch: {item['path']}")
        fixtures.append({"path": item["path"], "sha256": digest})

    proof = {
        "testId": driver["testId"],
        "status": "pass",
        "commit": args.commit,
        "runId": args.run_id,
        "runner": "GitHub Actions Ubuntu 24.04 x86-64",
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "requestedFrames": required_frames,
        "frameTicks": measurement["frameTicks"],
        "machineTicks": measurement["machineTicks"],
        "retainedFrameSpan": measurement["retainedFrameSpan"],
        "retainedRecords": measurement["retainedRecords"],
        "ringGenerations": measurement["ringGenerations"],
        "fileBytes": measurement["fileBytes"],
        "fixtures": fixtures,
    }
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    args.proof.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
