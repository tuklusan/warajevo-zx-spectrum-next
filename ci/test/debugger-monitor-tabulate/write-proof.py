#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Pin monitor T evidence from all five native build jobs."""

import argparse
import hashlib
import json
import re
import subprocess
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath


ROOT = Path(__file__).resolve().parents[3]
DRIVER_PATH = "test-drivers/debugger-monitor-tabulate-contract.driver.json"
FIXTURE_ROOTS = (".github/workflows/", "ci/test/debugger-monitor-tabulate/",
                 "ci/test/performance/", "docs/design/", "src/app/",
                 "src/cmake/", "src/core/", "test-drivers/")
PLATFORMS = (
    ("Windows x64", "Full build / Windows x64", "windows-x64",
     "Windows-windows-x64", "X64"),
    ("Linux x64", "Full build / Linux x64", "linux-x64",
     "Linux-ubuntu-24.04", "X64"),
    ("Linux ARM64", "Full build / Linux ARM64", "linux-arm64",
     "Linux-ubuntu-24.04-arm", "ARM64"),
    ("macOS ARM64", "Full build / macOS ARM64", "macos-arm64",
     "macOS-macos-arm64", "ARM64"),
    ("macOS Intel", "Full build / macOS Intel", "macos-x64",
     "macOS-macos-x64", "X64"),
)
MARKER = re.compile(
    r"debugger-monitor-tabulate-contract assertions=(\d+) status=pass")


def git_blob(commit: str, path: str) -> bytes:
    relative = PurePosixPath(path)
    if (not path.startswith(FIXTURE_ROOTS) or
            re.fullmatch(r"[A-Za-z0-9._/-]+", path) is None or
            relative.is_absolute() or ".." in relative.parts):
        raise SystemExit(f"unsafe fixture path: {path}")
    listing = subprocess.check_output(
        ["git", "ls-tree", "-r", "--full-tree", commit, "--", path],
        cwd=ROOT, text=True, stderr=subprocess.PIPE)
    found = []
    for line in listing.splitlines():
        metadata, separator, listed_path = line.partition("\t")
        fields = metadata.split()
        if separator and listed_path == path and len(fields) == 3:
            mode, kind, object_id = fields
            if kind == "blob" and re.fullmatch(r"[0-9a-f]{40}", object_id):
                found.append(object_id)
    if len(found) != 1:
        raise SystemExit(f"fixture missing or ambiguous at {commit}: {path}")
    return subprocess.check_output(["git", "cat-file", "blob", found[0]],
                                   cwd=ROOT, stderr=subprocess.PIPE)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--commit", required=True)
    parser.add_argument("--driver-commit")
    parser.add_argument("--run-id", required=True, type=int)
    parser.add_argument("--branch", required=True)
    parser.add_argument("--event", required=True)
    parser.add_argument("--input-dir", required=True)
    parser.add_argument("--output", default=
                        "test-results/debugger-monitor-tabulate-contract.json")
    args = parser.parse_args()
    if re.fullmatch(r"[0-9a-f]{40}", args.commit) is None:
        raise SystemExit("commit must be a full lowercase Git SHA")
    driver_commit = args.driver_commit or args.commit
    if re.fullmatch(r"[0-9a-f]{40}", driver_commit) is None:
        raise SystemExit("driver commit must be a full lowercase Git SHA")

    driver = json.loads(git_blob(driver_commit, DRIVER_PATH))
    expected_matrix = [item[0] for item in PLATFORMS]
    if (driver.get("testId") != "debugger-monitor-tabulate-contract" or
            driver.get("requiredAssertions") != 29 or
            driver.get("runnerMatrix") != expected_matrix):
        raise SystemExit("unexpected monitor T driver")
    fixtures = []
    for fixture in driver.get("fixtures", []):
        path = fixture.get("path", "")
        digest = hashlib.sha256(git_blob(driver_commit, path)).hexdigest()
        if digest != fixture.get("sha256"):
            raise SystemExit(f"fixture hash mismatch at {driver_commit}: {path}")
        fixtures.append({"path": path, "sha256": digest})

    if args.branch != "main" or args.event not in ("push", "workflow_dispatch"):
        raise SystemExit("workflow run does not match the requested main-branch source")
    input_dir = Path(args.input_dir).resolve()
    if ROOT not in input_dir.parents or not input_dir.is_dir():
        raise SystemExit("hosted runner evidence directory is missing or outside the repository")

    results = []
    for platform, job_name, slug, runner, architecture in PLATFORMS:
        runner_path = input_dir / f"runner-{slug}.json"
        ctest_path = input_dir / f"ctest-{slug}.log"
        if not runner_path.is_file() or not ctest_path.is_file():
            raise SystemExit(f"hosted runner evidence files missing: {platform}")
        runner_record = json.loads(runner_path.read_text(encoding="utf-8"))
        ctest_log = ctest_path.read_text(encoding="utf-8", errors="replace")
        ctest_summary = re.search(
            r"100% tests passed(?:, 0 tests failed)? out of (\d+)", ctest_log)
        marker = MARKER.search(ctest_log)
        required = driver.get("requiredAssertions")
        if (not isinstance(required, int) or required <= 0 or
                ctest_summary is None or int(ctest_summary.group(1)) == 0 or
                marker is None or int(marker.group(1)) != required):
            raise SystemExit(f"monitor T or successful CTest evidence missing: {platform}")
        ctest_count = int(ctest_summary.group(1))
        if (runner_record.get("status") != "pass" or
                runner_record.get("commit") != args.commit or
                runner_record.get("platform") != platform or
                runner_record.get("jobName") != job_name or
                runner_record.get("runner") != runner or
                runner_record.get("architecture") != architecture or
                runner_record.get("ctest") != f"{ctest_count}/{ctest_count} passed"):
            raise SystemExit(f"native runner identity or result mismatch: {platform}")
        results.append({
            "runner": platform, "jobName": job_name,
            "architecture": architecture, "status": "pass",
            "assertions": int(marker.group(1)),
            "ctest": f"{ctest_count}/{ctest_count} passed",
        })

    proof = {
        "testId": driver["testId"], "status": "pass",
        "commit": args.commit, "sourceCommit": args.commit,
        "driverCommit": driver_commit, "runId": args.run_id,
        "workflowRun": (
            "https://github.com/tuklusan/warajevo-zx-spectrum-next/"
            f"actions/runs/{args.run_id}"),
        "workflowEvent": args.event, "branch": args.branch,
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "fixtures": fixtures,
        "build": {"configuration": "Release", "sokolHost": "ON"},
        "assertionsPerTarget": driver["requiredAssertions"],
        "runnerResults": results,
        "summary": {
            "nativeTargets": len(results),
            "assertions": driver["requiredAssertions"] * len(results),
        },
    }
    output = (ROOT / args.output).resolve()
    if ROOT not in output.parents:
        raise SystemExit("output must remain under the repository root")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
