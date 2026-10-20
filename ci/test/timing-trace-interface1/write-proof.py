#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Validate the five-target Interface 1 trace result and pin exact fixtures."""

import argparse
import hashlib
import json
import re
import subprocess
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath


def git_bytes(commit: str, path: str) -> bytes:
    return subprocess.check_output(["git", "show", f"{commit}:{path}"],
                                   stderr=subprocess.PIPE)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--commit", required=True)
    parser.add_argument("--run-id", required=True, type=int)
    parser.add_argument("--run-log", required=True, type=Path)
    parser.add_argument("--proof", required=True, type=Path)
    args = parser.parse_args()
    if re.fullmatch(r"[0-9a-f]{40}", args.commit) is None:
        raise SystemExit("commit must be a full lowercase SHA-1 object ID")

    driver = json.loads(git_bytes(
        args.commit, "test-drivers/timing-trace-interface1.driver.json"))
    output = args.run_log.read_text(encoding="utf-8-sig", errors="replace")
    marker = (
        "PASS Interface 1, Microdrive, and ZX Net port accesses retain exact "
        "ticks and latch context")
    if marker not in output:
        raise SystemExit("Interface 1 trace success marker is missing")

    fixtures = []
    for item in driver["fixtures"]:
        relative = PurePosixPath(item["path"])
        if relative.is_absolute() or ".." in relative.parts:
            raise SystemExit(f"unsafe fixture path: {item['path']}")
        digest = hashlib.sha256(
            git_bytes(args.commit, relative.as_posix())).hexdigest()
        if digest != item["sha256"]:
            raise SystemExit(f"fixture hash mismatch: {item['path']}")
        fixtures.append({"path": item["path"], "sha256": digest})

    run = json.loads(subprocess.check_output(
        ["gh", "run", "view", str(args.run_id), "--json", "jobs"], text=True))
    platforms = ["Windows x64", "Linux x64", "Linux ARM64",
                 "macOS ARM64", "macOS Intel"]
    runner_results = []
    for platform in platforms:
        job_name = f"Full build / {platform}"
        job = next((entry for entry in run["jobs"]
                    if entry["name"] == job_name), None)
        if job is None or job["conclusion"] != "success":
            raise SystemExit(f"missing passing native runner: {job_name}")
        section = "\n".join(line for line in output.splitlines()
                              if line.startswith(job_name + "\t"))
        if marker not in section or "100% tests passed" not in section:
            raise SystemExit(f"runner log does not prove full CTest success: {platform}")
        runner_results.append({
            "runner": platform,
            "status": "pass",
            "caseCount": driver["requiredCases"],
            "jobId": job["databaseId"],
            "timestamp": job["completedAt"],
        })

    proof = {
        "testId": driver["testId"],
        "status": "pass",
        "commit": args.commit,
        "runId": args.run_id,
        "runner": "; ".join(platforms),
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "caseCount": driver["requiredCases"] * len(runner_results),
        "caseCountPerRunner": driver["requiredCases"],
        "observedOutput": marker,
        "workflowRun": (
            "https://github.com/tuklusan/warajevo-zx-spectrum-next/actions/runs/"
            f"{args.run_id}"),
        "runnerResults": runner_results,
        "nativeBuildConfiguration": "Release; WZSN_BUILD_SOKOL_HOST=ON",
        "ctest": "all integrated CTests passed on each native runner",
        "fixtures": fixtures,
    }
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    args.proof.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
