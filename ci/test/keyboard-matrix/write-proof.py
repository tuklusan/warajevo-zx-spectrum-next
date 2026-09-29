#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Write the source-pinned hosted keyboard-matrix contract proof."""

import argparse
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path

FIXTURES = (
    ".github/workflows/keyboard-matrix-contract.yml",
    "ci/test/keyboard-matrix/CMakeLists.txt",
    "ci/test/keyboard-matrix/keyboard-matrix-contract.c",
    "ci/test/keyboard-matrix/write-proof.py",
    "test-drivers/keyboard-matrix.driver.json",
    "src/core/wz_keyboard_matrix.c",
    "src/core/wz_keyboard_matrix.h",
)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--commit", required=True)
    parser.add_argument("--run-id", required=True, type=int)
    parser.add_argument("--runner", required=True)
    parser.add_argument("--proof", required=True, type=Path)
    args = parser.parse_args()
    fixtures = [
        {"path": name, "sha256": hashlib.sha256(Path(name).read_bytes()).hexdigest()}
        for name in FIXTURES
    ]
    proof = {
        "testId": "keyboard-matrix",
        "status": "pass",
        "commit": args.commit,
        "runId": args.run_id,
        "runner": args.runner,
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "caseCount": 4,
        "fixtures": fixtures,
    }
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    args.proof.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
