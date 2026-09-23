# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Aggregate the four hosted multi-instance stress records into test evidence."""

from __future__ import annotations

import argparse
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path
import re
import subprocess
import sys


REPO = Path(__file__).resolve().parents[3]
MATRIX = ("windows-x64", "windows-arm64", "macos-intel", "macos-arm64")
PROJECT_FILES = (
    "tests/multi-instance-stress.c",
    "src/app/wz_control_port.c",
    "src/app/wz_control_port.h",
    "src/app/wz_host_config.c",
    "src/app/wz_host_config.h",
    "src/app/wz_host_media_ownership.c",
    "src/app/wz_host_media_ownership.h",
    "src/app/wz_host_output.c",
    "src/app/wz_host_output.h",
    "src/app/wz_host_socket.c",
    "src/app/wz_host_socket.h",
    "src/app/wz_telnet_keyboard_command.c",
    "src/app/wz_telnet_keyboard_command.h",
    "src/app/wz_command_registry.c",
    "src/app/wz_command_registry.h",
    "src/app/wz_telnet_keymap.c",
    "src/app/wz_telnet_keymap.h",
    "src/app/wz_screenshot_service.c",
    "src/app/wz_screenshot_service.h",
    "src/core/wz_presentation_snapshot.c",
    "src/core/wz_presentation_snapshot.h",
    "ci/test/multi-instance/CMakeLists.txt",
    "ci/test/multi-instance/run-stress.py",
    "ci/test/multi-instance/write-proof.py",
    "ci/test/multi-instance/README.md",
    "tests/multi-instance-stress.md",
    "test-drivers/multi-instance-stress.driver.json",
    ".github/workflows/multi-instance-stress.yml",
)
EXPECTED_CASES = 5


def blob(commit: str, path: str) -> bytes:
    return subprocess.check_output(["git", "show", f"{commit}:{path}"], cwd=REPO)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--project-commit", required=True)
    parser.add_argument("--artifact-root", required=True, type=Path)
    parser.add_argument("--proof-out", required=True, type=Path)
    args = parser.parse_args()

    if not re.fullmatch(r"[0-9a-f]{40}", args.project_commit):
        print("Invalid project commit identifier", file=sys.stderr)
        return 1
    runners = []
    for runner_id in MATRIX:
        runner_file = args.artifact_root / runner_id / "runner.json"
        log_file = args.artifact_root / runner_id / "run.log"
        if not runner_file.is_file() or not log_file.is_file():
            print(f"Hosted stress result is missing: {runner_id}",
                  file=sys.stderr)
            return 1
        if runner_file.stat().st_size > 16384 or log_file.stat().st_size > 65536:
            print(f"Hosted stress result is oversized: {runner_id}",
                  file=sys.stderr)
            return 1
        with runner_file.open(encoding="utf-8") as stream:
            metadata = json.load(stream)
        marker = re.compile(
            rf"^PASS multi-instance stress cases: {EXPECTED_CASES}\s*$"
        )
        with log_file.open(encoding="utf-8") as stream:
            passed = any(marker.fullmatch(line.rstrip("\r\n")) for line in stream)
        if not passed:
            print(f"Hosted stress result missing or failed: {runner_id}",
                  file=sys.stderr)
            return 1
        runners.append({"id": runner_id, **metadata})

    fixtures = []
    for relative_path in PROJECT_FILES:
        digest = hashlib.sha256(blob(args.project_commit, relative_path)).hexdigest()
        fixtures.append({"path": relative_path, "sha256": digest})

    proof = {
        "testId": "multi-instance-stress",
        "status": "pass",
        "commit": args.project_commit,
        "runner": "Windows x64/ARM64 and macOS Intel/ARM64 hosted matrix",
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "caseCount": EXPECTED_CASES,
        "runners": runners,
        "fixtures": fixtures,
    }
    args.proof_out.parent.mkdir(parents=True, exist_ok=True)
    args.proof_out.write_text(json.dumps(proof, indent=2) + "\n",
                              encoding="utf-8")
    print(f"PASS aggregated multi-instance proof for {len(runners)} runners")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
