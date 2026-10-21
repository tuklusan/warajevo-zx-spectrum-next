#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Capture one native runner's hosted test and performance evidence."""

import argparse
import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
WAIT_MARKER = "PASS Interface 1 WAIT pending-cycle resume (75 checks)"


def repository_file(value: str) -> Path:
    path = (ROOT / value).resolve()
    if ROOT not in path.parents or not path.is_file():
        raise SystemExit(f"input must be an existing repository file: {value}")
    return path


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--commit", required=True)
    parser.add_argument("--platform", required=True)
    parser.add_argument("--job-name", required=True)
    parser.add_argument("--runner", required=True)
    parser.add_argument("--architecture", required=True)
    parser.add_argument("--slug", required=True)
    parser.add_argument("--ctest-log", required=True)
    parser.add_argument("--performance-record", required=True)
    parser.add_argument("--output-dir", required=True)
    args = parser.parse_args()

    if re.fullmatch(r"[0-9a-f]{40}", args.commit) is None:
        raise SystemExit("commit must be a full lowercase Git SHA")
    if re.fullmatch(r"[a-z0-9-]+", args.slug) is None:
        raise SystemExit("invalid runner slug")

    ctest_path = repository_file(args.ctest_log)
    performance_path = repository_file(args.performance_record)
    ctest = ctest_path.read_text(encoding="utf-8", errors="replace")
    if WAIT_MARKER not in ctest:
        raise SystemExit("Interface 1 WAIT marker is missing from the CTest log")
    summary = re.search(r"100% tests passed, 0 tests failed out of (\d+)", ctest)
    if summary is None or int(summary.group(1)) == 0:
        raise SystemExit("successful CTest completion is missing from the CTest log")
    ctest_count = int(summary.group(1))

    performance = json.loads(performance_path.read_text(encoding="utf-8"))
    if (performance.get("schema") != 1 or performance.get("status") != "pass" or
            performance.get("commit") != args.commit or
            performance.get("runner") != args.runner or
            performance.get("architecture") != args.architecture):
        raise SystemExit("performance record does not match this runner and commit")

    output_dir = (ROOT / args.output_dir).resolve()
    if ROOT not in output_dir.parents:
        raise SystemExit("output directory must remain under the repository root")
    output_dir.mkdir(parents=True, exist_ok=True)
    (output_dir / f"ctest-{args.slug}.log").write_text(ctest, encoding="utf-8")
    (output_dir / f"performance-{args.slug}.json").write_text(
        json.dumps(performance, indent=2) + "\n", encoding="utf-8")
    runner = {
        "testId": "interface1-wait", "status": "pass", "commit": args.commit,
        "platform": args.platform, "jobName": args.job_name,
        "runner": args.runner,
        "architecture": args.architecture,
        "requiredCases": 75, "observedOutput": WAIT_MARKER,
        "ctest": f"{ctest_count}/{ctest_count} passed",
    }
    (output_dir / f"runner-{args.slug}.json").write_text(
        json.dumps(runner, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
