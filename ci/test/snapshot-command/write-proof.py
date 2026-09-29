#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Write the source-pinned hosted snapshot-command contract proof."""

import argparse
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path


FIXTURES = (
    ".github/workflows/snapshot-command-regression.yml",
    "ci/test/snapshot-command/CMakeLists.txt",
    "ci/test/snapshot-command/snapshot-command-contract.c",
    "ci/test/snapshot-command/write-proof.py",
    "src/app/wz_file_dialog.h",
    "src/app/wz_file_dialog_linux.c",
    "src/app/wz_file_dialog_macos.m",
    "src/app/wz_file_dialog_win32.c",
    "src/app/wz_file_open_run.c",
    "src/app/wz_snapshot_save_workflow.c",
    "src/app/wz_snapshot_save_workflow.h",
    "src/app/wz_sokol_main.c",
    "src/core/wz_state.c",
    "src/core/wz_state.h",
    "test-drivers/snapshot-command.driver.json",
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
        "testId": "snapshot-command",
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
