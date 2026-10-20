#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Validate the five-target cassette-EAR trace proof and exact-source fixtures."""

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
        args.commit, "test-drivers/timing-trace-ear.driver.json"))
    output = args.run_log.read_text(encoding="utf-8-sig", errors="replace")
    marker = "PASS tape EAR transitions follow CPU master-tick advancement"
    if marker not in output:
        raise SystemExit("cassette-EAR success marker is missing")

    fixtures = []
    for item in driver["fixtures"]:
        relative = PurePosixPath(item["path"])
        if relative.is_absolute() or ".." in relative.parts:
            raise SystemExit(f"unsafe fixture path: {item['path']}")
        fixture_bytes = git_bytes(args.commit, relative.as_posix())
        digest = hashlib.sha256(fixture_bytes).hexdigest()
        checkout_digest = hashlib.sha256(Path(relative.as_posix()).read_bytes()).hexdigest()
        checkout_is_clean = subprocess.run(
            ["git", "diff", "--quiet", args.commit, "--", relative.as_posix()],
            check=False,
        ).returncode == 0
        # The driver may hash Windows checkout bytes (including mixed legacy
        # line endings); require Git to confirm that checkout maps to this blob.
        if item["sha256"] != digest and not (
            item["sha256"] == checkout_digest and checkout_is_clean
        ):
            raise SystemExit(f"fixture hash mismatch: {item['path']}")
        fixtures.append({"path": item["path"], "sha256": digest})

    run = json.loads(subprocess.check_output(
        ["gh", "run", "view", str(args.run_id), "--json", "jobs"], text=True))
    expected_platforms = ["Windows x64", "Linux x64", "Linux ARM64",
                          "macOS ARM64", "macOS Intel"]
    runner_results = []
    for platform in expected_platforms:
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
        "runner": "; ".join(expected_platforms),
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "caseCount": driver["requiredCases"] * len(runner_results),
        "caseCountPerRunner": driver["requiredCases"],
        "observedOutput": marker,
        "workflowRun": f"https://github.com/tuklusan/warajevo-zx-spectrum-next/actions/runs/{args.run_id}",
        "runnerResults": runner_results,
        "nativeBuildConfiguration": "Release; WZSN_BUILD_SOKOL_HOST=ON",
        "ctest": "all integrated CTests passed on each native runner",
        "fixtures": fixtures,
    }
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    args.proof.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
