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
from datetime import datetime, timezone
from pathlib import Path


FIXTURES = (
    ".github/workflows/file-open-run-path-contract.yml",
    "ci/test/file-open-run-path/CMakeLists.txt",
    "ci/test/file-open-run-path/file-open-run-path-contract.c",
    "ci/test/file-open-run-path/write-proof.py",
    "src/app/wz_file_open_run.c",
    "src/app/wz_file_open_run.h",
    "test-drivers/file-open-run-path.driver.json",
)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--proof", required=True, type=Path)
    args = parser.parse_args()
    driver = json.loads(Path("test-drivers/file-open-run-path.driver.json").read_text(
        encoding="utf-8"))
    if driver.get("testId") != "file-open-run-path":
        raise SystemExit("file-open-run path driver identity mismatch")
    for fixture in driver["fixtures"]:
        actual = hashlib.sha256(Path(fixture["path"]).read_bytes()).hexdigest()
        if actual != fixture["sha256"]:
            raise SystemExit(f"fixture hash mismatch: {fixture['path']}")
    proof = {
        "testId": "file-open-run-path",
        "status": "pass",
        "commit": os.environ["GITHUB_SHA"],
        "runId": int(os.environ["GITHUB_RUN_ID"]),
        "runner": f"{os.environ['RUNNER_OS']}/{os.environ['RUNNER_ARCH']}",
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "caseCount": driver["requiredCases"],
        "fixtures": [
            {
                "path": name,
                "sha256": hashlib.sha256(Path(name).read_bytes()).hexdigest(),
            }
            for name in FIXTURES
        ],
    }
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    args.proof.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
