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
                 "src/cmake/", "src/core/", "docs/design/")
PLATFORMS = (
    ("Windows x64", "Full build / Windows x64"),
    ("Linux x64", "Full build / Linux x64"),
    ("Linux ARM64", "Full build / Linux ARM64"),
    ("macOS ARM64", "Full build / macOS ARM64"),
    ("macOS Intel", "Full build / macOS Intel"),
)
MARKER = "PASS Interface 1 WAIT pending-cycle resume (76 checks)"


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


def gh_json(*args: str) -> dict:
    return json.loads(subprocess.check_output(
        ["gh", *args], cwd=ROOT, text=True, stderr=subprocess.PIPE))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--commit", required=True)
    parser.add_argument("--run-id", required=True, type=int)
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

    run = gh_json("run", "view", str(args.run_id),
                  "--json", "headSha,conclusion,event,headBranch,jobs")
    if (run.get("headSha") != args.commit or
            run.get("conclusion") != "success" or
            run.get("event") != "push" or run.get("headBranch") != "main"):
        raise SystemExit("workflow run does not prove a successful main push at the requested source")

    results = []
    for platform, job_name in PLATFORMS:
        job = next((item for item in run.get("jobs", [])
                    if item.get("name") == job_name), None)
        if job is None or job.get("conclusion") != "success":
            raise SystemExit(f"successful native job missing: {platform}")
        log = subprocess.check_output(
            ["gh", "run", "view", str(args.run_id), "--job",
             str(job["databaseId"]), "--log"], cwd=ROOT, text=True,
            stderr=subprocess.PIPE)
        if MARKER not in log or "100% tests passed" not in log:
            raise SystemExit(f"WAIT/CTest evidence missing from {platform}")
        results.append({"runner": platform, "status": "pass",
                        "requiredCases": driver["requiredCases"],
                        "jobId": job["databaseId"],
                        "completedAt": job.get("completedAt")})

    proof = {
        "testId": driver["testId"], "status": "pass", "commit": args.commit,
        "runId": args.run_id, "runner": "; ".join(driver["runnerMatrix"]),
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "caseCount": driver["requiredCases"] * len(results),
        "caseCountPerRunner": driver["requiredCases"],
        "observedOutput": MARKER,
        "workflowRun": f"https://github.com/tuklusan/warajevo-zx-spectrum-next/actions/runs/{args.run_id}",
        "nativeBuildConfiguration": "Release; WZSN_BUILD_SOKOL_HOST=ON",
        "ctest": "all integrated CTests passed on each native runner",
        "runnerResults": results, "fixtures": fixtures,
        "githubArtifactsRetained": False, "githubCachesRetained": False,
    }
    output = (ROOT / args.output).resolve()
    if ROOT not in output.parents:
        raise SystemExit("output must remain under the repository root")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
