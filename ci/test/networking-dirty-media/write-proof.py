#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Validate the hosted result and write a source-pinned dirty-media proof."""

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
    parser.add_argument("--runner", required=True)
    parser.add_argument("--run-log", required=True, type=Path)
    parser.add_argument("--proof", required=True, type=Path)
    args = parser.parse_args()
    root = Path.cwd()
    driver = json.loads((root / "test-drivers/networking-dirty-media.driver.json")
                        .read_text(encoding="utf-8"))
    log = args.run_log.read_text(encoding="utf-8")
    match = re.search(r"networking-dirty-media cases=(\d+) status=pass", log)
    if match is None or int(match.group(1)) != driver["requiredCases"]:
        raise SystemExit("hosted run output does not match the required cases")

    fixtures = []
    for record in driver["fixtures"]:
        relative = PurePosixPath(record["path"])
        if relative.is_absolute() or ".." in relative.parts:
            raise SystemExit(f"unsafe fixture path: {record['path']}")
        committed = subprocess.check_output(
            ["git", "show", f"HEAD:{relative.as_posix()}"], cwd=root,
            stderr=subprocess.PIPE)
        digest = hashlib.sha256(committed).hexdigest()
        if digest != record["sha256"]:
            raise SystemExit(f"fixture hash mismatch: {record['path']}")
        fixtures.append({"path": record["path"], "sha256": digest})

    proof = {
        "testId": driver["testId"],
        "status": "pass",
        "commit": args.commit,
        "runId": args.run_id,
        "runner": args.runner,
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "caseCount": int(match.group(1)),
        "fixtures": fixtures,
    }
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    args.proof.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
