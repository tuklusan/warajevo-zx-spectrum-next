#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Validate and pin Debug Sokol trace startup/shutdown evidence."""

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
    driver = json.loads((root / "test-drivers/debug-trace-startup.driver.json")
                        .read_text(encoding="utf-8"))
    measurement = json.loads(args.measurement.read_text(encoding="utf-8"))
    if (measurement.get("testId") != driver["testId"] or
            measurement.get("status") != "pass" or
            measurement.get("fileBytes") != 16777216 or
            measurement.get("frozen") is not True or
            measurement.get("recordCount", 0) <= 0):
        raise SystemExit("measurement does not prove Debug trace startup/shutdown")

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
        "runner": driver["runner"],
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "fileBytes": measurement["fileBytes"],
        "frozen": measurement["frozen"],
        "recordCount": measurement["recordCount"],
        "fixtures": fixtures,
    }
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    args.proof.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
