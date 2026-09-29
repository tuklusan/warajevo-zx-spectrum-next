#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Write the source-pinned hosted tape-loading equivalence proof."""

import argparse
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path

FIXTURES = (
    ".github/workflows/tape-loading-equivalence.yml",
    "ci/test/tape-loading-equivalence/CMakeLists.txt",
    "ci/test/tape-loading-equivalence/tape-loading-equivalence.c",
    "ci/test/tape-loading-equivalence/write-proof.py",
    "test-drivers/tape-loading-equivalence.driver.json",
    "src/core/wz_machine.c",
    "src/core/wz_machine.h",
    "src/core/wz_runner.c",
    "src/core/wz_runner.h",
    "src/core/wz_state.c",
    "src/core/wz_state.h",
    "src/core/wz_tape.c",
    "src/core/wz_tape.h",
    "src/core/wz_keyboard_matrix.c",
    "src/core/wz_keyboard_matrix.h",
    "docs/design/01-warajevo-zx-spectrum-next-architecture.md",
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
        "testId": "tape-loading-equivalence",
        "status": "pass",
        "commit": args.commit,
        "runId": args.run_id,
        "runner": args.runner,
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "caseCount": 2,
        "fixtures": fixtures,
    }
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    args.proof.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
