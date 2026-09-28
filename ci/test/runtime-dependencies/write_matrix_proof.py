#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Combine native runtime-dependency reports into one pinned CI proof."""

import argparse
import hashlib
import json
import os
import subprocess
from datetime import datetime, timezone
from pathlib import Path


EXPECTED_RUNNERS = {"linux-x64", "linux-arm64", "macos-x64", "macos-arm64"}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--driver", required=True, type=Path)
    parser.add_argument("--reports", required=True, type=Path)
    parser.add_argument("--proof-out", required=True, type=Path)
    args = parser.parse_args()

    driver = json.loads(args.driver.read_text(encoding="utf-8"))
    fixtures = []
    for item in driver["fixtures"]:
        data = subprocess.check_output(
            ["git", "show", f"{os.environ['GITHUB_SHA']}:{item['path']}"]
        )
        digest = hashlib.sha256(data).hexdigest()
        if digest != item["sha256"]:
            raise SystemExit(f"pinned fixture changed: {item['path']}")
        fixtures.append({"path": item["path"], "sha256": digest})

    reports = [
        json.loads(path.read_text(encoding="utf-8"))
        for path in sorted(args.reports.rglob("runner-proof.json"))
    ]
    observed_runners = {report.get("runnerId") for report in reports}
    if observed_runners != EXPECTED_RUNNERS or len(reports) != len(EXPECTED_RUNNERS):
        raise SystemExit(
            f"incomplete or duplicate runner reports: {sorted(observed_runners)}"
        )

    proof = {
        "testId": driver["testId"],
        "status": "pass",
        "commit": os.environ["GITHUB_SHA"],
        "runId": int(os.environ["GITHUB_RUN_ID"]),
        "runner": "Linux x86-64/AArch64 and macOS x86-64/Apple Silicon hosted matrix",
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "caseCount": len(reports),
        "runners": sorted(reports, key=lambda item: item["runnerId"]),
        "fixtures": fixtures,
    }
    args.proof_out.parent.mkdir(parents=True, exist_ok=True)
    args.proof_out.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    print(f"PASS {len(reports)} native release binaries audited")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
