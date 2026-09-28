#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Validate a hosted normal-path DIZZY4K tape-load run and pin its evidence."""

import argparse
import hashlib
import json
import subprocess
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--project-commit", required=True)
    parser.add_argument("--run-id", required=True, type=int)
    parser.add_argument("--run-log", required=True, type=Path)
    parser.add_argument("--proof-out", required=True, type=Path)
    args = parser.parse_args()

    root = Path.cwd()
    driver = json.loads((root / "test-drivers/tape-keyboard-load.driver.json")
                        .read_text(encoding="utf-8"))
    output = args.run_log.read_text(encoding="utf-8")
    marker = "DIZZY4K BASIC and machine-code blocks loaded through the normal ROM path"
    if marker not in output:
        raise SystemExit("normal ROM path success marker is missing")

    fixtures = []
    for record in driver["fixtures"]:
        relative = PurePosixPath(record["path"])
        if relative.is_absolute() or ".." in relative.parts:
            raise SystemExit(f"fixture path is unsafe: {record['path']}")
        content = subprocess.check_output(
            ["git", "show", f"{args.project_commit}:{relative.as_posix()}"],
            cwd=root)
        digest = hashlib.sha256(content).hexdigest()
        if digest != record["sha256"]:
            raise SystemExit(f"fixture hash mismatch: {record['path']}")
        fixtures.append({"path": record["path"], "sha256": digest})

    proof = {
        "testId": driver["testId"],
        "status": "pass",
        "commit": args.project_commit,
        "runner": "Ubuntu 24.04 x86-64 hosted runner",
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "runId": args.run_id,
        "observedOutput": output.strip(),
        "fixtures": fixtures,
    }
    args.proof_out.parent.mkdir(parents=True, exist_ok=True)
    args.proof_out.write_text(json.dumps(proof, indent=2) + "\n",
                              encoding="utf-8")


if __name__ == "__main__":
    main()
