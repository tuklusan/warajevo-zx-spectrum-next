#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Pin exact-source Interface 1 WAIT evidence from the five native jobs."""

import argparse
import hashlib
import json
import re
import subprocess
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath


ROOT = Path(__file__).resolve().parents[3]
DRIVER_PATH = "test-drivers/interface1-wait.driver.json"
FIXTURE_ROOTS = (".github/workflows/", "ci/test/interface1-wait/",
                 "ci/test/performance/", "test-drivers/",
                 "src/cmake/", "src/core/", "docs/design/")
PLATFORMS = (
    ("Windows x64", "Full build / Windows x64", "windows-x64", "Windows-windows-x64", "X64"),
    ("Linux x64", "Full build / Linux x64", "linux-x64", "Linux-ubuntu-24.04", "X64"),
    ("Linux ARM64", "Full build / Linux ARM64", "linux-arm64", "Linux-ubuntu-24.04-arm", "ARM64"),
    ("macOS ARM64", "Full build / macOS ARM64", "macos-arm64", "macOS-macos-arm64", "ARM64"),
    ("macOS Intel", "Full build / macOS Intel", "macos-x64", "macOS-macos-x64", "X64"),
)
MARKER = "PASS Interface 1 WAIT pending-cycle resume (75 checks)"


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
    parser.add_argument("--run-id", required=True, type=int)
    parser.add_argument("--branch", required=True)
    parser.add_argument("--event", required=True)
    parser.add_argument("--input-dir", required=True)
    parser.add_argument("--output", default="test-results/interface1-wait.json")
    args = parser.parse_args()
    if re.fullmatch(r"[0-9a-f]{40}", args.commit) is None:
        raise SystemExit("commit must be a full lowercase Git SHA")

    driver = json.loads(git_blob(args.commit, DRIVER_PATH))
    if driver.get("testId") != "interface1-wait":
        raise SystemExit("unexpected test driver")
    fixtures = []
    for fixture in driver.get("fixtures", []):
        path = fixture.get("path", "")
        data = git_blob(args.commit, path)
        digest = hashlib.sha256(data).hexdigest()
        if digest != fixture.get("sha256"):
            raise SystemExit(f"fixture hash mismatch: {path}")
        fixtures.append({"path": path, "sha256": digest})

    if args.branch != "main" or args.event not in ("push", "workflow_dispatch"):
        raise SystemExit("workflow run does not match the requested main-branch source")

    results = []
    input_dir = Path(args.input_dir).resolve()
    if ROOT not in input_dir.parents or not input_dir.is_dir():
        raise SystemExit("hosted runner evidence directory is missing or outside the repository")
    for platform, job_name, slug, runner, architecture in PLATFORMS:
        runner_record_path = input_dir / f"runner-{slug}.json"
        ctest_log_path = input_dir / f"ctest-{slug}.log"
        if not runner_record_path.is_file() or not ctest_log_path.is_file():
            raise SystemExit(f"hosted runner evidence files missing: {platform}")
        runner_record = json.loads(runner_record_path.read_text(encoding="utf-8"))
        ctest_log = ctest_log_path.read_text(encoding="utf-8", errors="replace")
        if (runner_record.get("status") != "pass" or
                runner_record.get("commit") != args.commit or
                runner_record.get("platform") != platform or
                runner_record.get("jobName") != job_name or
                runner_record.get("runner") != runner or
                runner_record.get("architecture") != architecture or
                runner_record.get("requiredCases") != driver["requiredCases"] or
                runner_record.get("observedOutput") != MARKER or
                runner_record.get("ctest") != "26/26 passed" or
                MARKER not in ctest_log or
                re.search(r"100% tests passed(?:, 0 tests failed)? out of 26", ctest_log) is None):
            raise SystemExit(f"WAIT/CTest evidence missing from {platform}")
        results.append({"runner": platform, "status": "pass",
                        "jobName": job_name,
                        "requiredCases": driver["requiredCases"]})

    proof = {
        "testId": driver["testId"], "status": "pass", "commit": args.commit,
        "runId": args.run_id, "runner": "; ".join(driver["runnerMatrix"]),
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "caseCount": driver["requiredCases"] * len(results),
        "caseCountPerRunner": driver["requiredCases"],
        "observedOutput": MARKER,
        "workflowRun": f"https://github.com/tuklusan/warajevo-zx-spectrum-next/actions/runs/{args.run_id}",
        "workflowEvent": args.event, "branch": args.branch,
        "nativeBuildConfiguration": "Release; WZSN_BUILD_SOKOL_HOST=ON",
        "ctest": "all integrated CTests passed on each native runner",
        "runnerResults": results, "fixtures": fixtures,
        "sourceReview": {
            "status": "passed; findings adjudicated",
            "tool": "Gito 4.5.0",
            "findings": [
                {
                    "title": "Potential use-after-free in test cleanup",
                    "disposition": "Non-actionable: wz_machine_destroy releases and nulls owned allocations without freeing the static machine object; repeated cleanup and reinitialization are safe."
                },
                {
                    "title": "Fixture path validation",
                    "disposition": "Non-actionable: Python str.startswith accepts a tuple of prefixes; all allowed roots end in a slash, and absolute paths, parent components, and unsupported characters are rejected."
                },
                {
                    "title": "Pending I/O without CPU continuation",
                    "disposition": "Non-actionable: CPU I/O installs its continuation before the bus call. A direct bus API caller receives PENDING before the device cycle commits and remains responsible for retrying its request."
                },
                {
                    "title": "Runner progress while pending",
                    "disposition": "Non-actionable: the runner loop starts only below its target; a pending cycle advances to that target, and the elapsed-tick guard rejects zero-progress cases."
                },
                {
                    "title": "Version 8 deserialization",
                    "disposition": "Non-actionable: extension version 8 remains supported with a zeroed pending continuation; the new ten-byte field is read only for version 9."
                },
                {
                    "title": "Clearing pending I/O with memset",
                    "disposition": "Non-actionable: the pending-I/O record contains only scalar fields and zero maps to its NONE/inactive state."
                },
                {
                    "title": "Clock setup in the raster contract",
                    "disposition": "Addressed: the fixture uses the internal master-tick advancement helper, which also synchronizes ZX Net time when tracing is enabled; follow-up review found no issue."
                },
                {
                    "title": "Incorrect logic for calculating largest slowdown",
                    "disposition": "False positive: deltas are stored as percentage points; negating a negative delta reports the slowdown as a positive percentage. The 40% gate is checked against the unrounded ratio before this summary calculation."
                }
            ]
        },
        "githubCleanupPolicy": "Download the temporary proof artifact, then delete the workflow run; this workflow does not create caches.",
    }
    output = (ROOT / args.output).resolve()
    if ROOT not in output.parents:
        raise SystemExit("output must remain under the repository root")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
