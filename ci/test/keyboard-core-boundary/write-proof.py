#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Validate the four-platform host-to-core keyboard projection proof."""

import argparse
import hashlib
import json
import re
from datetime import datetime, timezone
from pathlib import Path

FIXTURES = (
    "ci/test/keyboard-core-boundary/CMakeLists.txt",
    "ci/test/keyboard-core-boundary/keyboard-core-boundary.c",
    "ci/test/keyboard-core-boundary/write-proof.py",
    "src/app/wz_input_arbiter.c",
    "src/app/wz_input_arbiter.h",
    "src/app/wz_input_event.h",
    "src/app/wz_input_projection.c",
    "src/app/wz_input_projection.h",
    "src/app/wz_sokol_main.c",
    "src/app/wz_telnet_input.c",
    "src/app/wz_telnet_input.h",
    "src/app/wz_input_timing.h",
    "src/core/wz_keyboard_matrix.c",
    "src/core/wz_keyboard_matrix.h",
    "src/core/wz_machine.c",
    "src/core/wz_machine.h",
    "src/cmake/CMakeLists.txt",
)

RUNNERS = {"windows-x64", "windows-arm64", "macos-x64", "macos-arm64"}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--commit", required=True)
    parser.add_argument("--run-id", required=True, type=int)
    parser.add_argument("--artifact-root", required=True, type=Path)
    parser.add_argument("--proof", required=True, type=Path)
    args = parser.parse_args()

    runner_records = []
    for path in sorted(args.artifact_root.rglob("runner.json")):
        record = json.loads(path.read_text(encoding="utf-8"))
        log = path.with_name("run.log").read_text(encoding="utf-8")
        match = re.search(r"passed (\d+) app-to-core keyboard projection assertions", log)
        if record.get("status") != "pass" or not match:
            raise SystemExit(f"runner evidence failed validation: {path}")
        record["assertions"] = int(match.group(1))
        runner_records.append(record)

    ids = {record["id"] for record in runner_records}
    if ids != RUNNERS or any(record["commit"] != args.commit for record in runner_records):
        raise SystemExit(f"unexpected runner set or source commit: {sorted(ids)}")

    fixtures = [
        {"path": name, "sha256": hashlib.sha256(Path(name).read_bytes()).hexdigest()}
        for name in FIXTURES
    ]
    proof = {
        "testId": "keyboard-core-boundary",
        "status": "pass",
        "commit": args.commit,
        "runId": args.run_id,
        "runner": "Windows/macOS x64 and ARM64",
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "caseCount": sum(record["assertions"] for record in runner_records),
        "runners": sorted(runner_records, key=lambda item: item["id"]),
        "fixtures": fixtures,
    }
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    args.proof.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
