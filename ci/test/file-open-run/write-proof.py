#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Write a source-pinned hosted proof for Open Run routing."""

import argparse
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path


FIXTURES = (
    ".github/workflows/file-open-run-regression.yml",
    "ci/test/file-open-run/write-proof.py",
    "ci/test/file-open-run/wz_file_open_run_tests.c",
    "src/app/wz_file_open_run.c",
    "src/app/wz_file_open_run.h",
    "src/cmake/CMakeLists.txt",
    "test-drivers/file-open-run.driver.json",
)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--commit", required=True)
    parser.add_argument("--run-id", required=True, type=int)
    parser.add_argument("--runner", required=True)
    parser.add_argument("--proof", required=True, type=Path)
    args = parser.parse_args()

    fixtures = []
    for name in FIXTURES:
        path = Path(name)
        fixtures.append(
            {"path": name, "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}
        )
    proof = {
        "testId": "file-open-run-routing",
        "status": "pass",
        "commit": args.commit,
        "runId": args.run_id,
        "runner": args.runner,
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "caseCount": 23,
        "fixtures": fixtures,
    }
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    args.proof.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
