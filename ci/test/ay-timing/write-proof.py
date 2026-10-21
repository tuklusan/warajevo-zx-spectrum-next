#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Pin five-target AY timing assertions from hosted build logs."""

import argparse
import hashlib
import json
import re
import subprocess
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath


ROOT = Path(__file__).resolve().parents[3]
DRIVER_PATH = "test-drivers/ay-timing-contract.driver.json"
FIXTURE_ROOTS = (".github/workflows/", "ci/test/ay-timing/",
                 "ci/test/performance/", "test-drivers/", "src/cmake/",
                 "src/core/audio/", "docs/design/")
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
    parser.add_argument("--output", default="test-results/ay-timing-contract.json")
    args = parser.parse_args()
    if re.fullmatch(r"[0-9a-f]{40}", args.commit) is None:
        raise SystemExit("commit must be a full lowercase Git SHA")
    driver_commit = args.driver_commit or args.commit
    if re.fullmatch(r"[0-9a-f]{40}", driver_commit) is None:
        raise SystemExit("driver commit must be a full lowercase Git SHA")
    if args.branch != "main" or args.event != "workflow_dispatch":
        raise SystemExit("AY proof requires a main-branch hosted dispatch")

    driver = json.loads(git_blob(driver_commit, DRIVER_PATH))
    if driver.get("testId") != "ay-timing-contract":
        raise SystemExit("unexpected AY test driver")
    fixtures = []
    for fixture in driver.get("fixtures", []):
        path = fixture.get("path", "")
        digest = hashlib.sha256(git_blob(driver_commit, path)).hexdigest()
        if digest != fixture.get("sha256"):
            raise SystemExit(f"fixture hash mismatch at {driver_commit}: {path}")
        fixtures.append({"path": path, "sha256": digest})

    input_dir = Path(args.input_dir).resolve()
    if ROOT not in input_dir.parents or not input_dir.is_dir():
        raise SystemExit("hosted runner evidence directory is missing or outside the repository")

    results = []
    marker = f"AY timing contract passed ({driver['requiredAssertions']} assertions)"
    for platform, job_name, slug, runner, architecture in PLATFORMS:
        runner_path = input_dir / f"runner-{slug}.json"
        ctest_path = input_dir / f"ctest-{slug}.log"
        build_path = input_dir / f"build-{slug}.log"
        if not all(path.is_file() for path in (runner_path, ctest_path, build_path)):
            raise SystemExit(f"hosted runner evidence files missing: {platform}")
        runner_record = json.loads(runner_path.read_text(encoding="utf-8"))
        ctest_log = ctest_path.read_text(encoding="utf-8", errors="replace")
        build_log = build_path.read_text(encoding="utf-8", errors="replace")
        ctest_summary = re.search(
            r"100% tests passed, 0 tests failed out of (\d+)", ctest_log)
        if ctest_summary is None or int(ctest_summary.group(1)) == 0:
            raise SystemExit(f"successful CTest completion is missing: {platform}")
        ctest_count = int(ctest_summary.group(1))
        if (runner_record.get("status") != "pass" or
                runner_record.get("commit") != args.commit or
                runner_record.get("platform") != platform or
                runner_record.get("jobName") != job_name or
                runner_record.get("runner") != runner or
                runner_record.get("architecture") != architecture or
                runner_record.get("ctest") != f"{ctest_count}/{ctest_count} passed" or
                marker not in build_log):
            raise SystemExit(f"AY timing or integrated CTest evidence missing from {platform}")
        results.append({"name": platform, "jobName": job_name,
                        "runner": runner, "architecture": architecture,
                        "status": "pass", "assertions": driver["requiredAssertions"],
                        "ctest": f"{ctest_count}/{ctest_count}"})

    proof = {
        "testId": driver["testId"], "status": "pass", "commit": args.commit,
        "runId": args.run_id,
        "workflowRun": f"https://github.com/tuklusan/warajevo-zx-spectrum-next/actions/runs/{args.run_id}",
        "workflowEvent": args.event, "branch": args.branch,
        "fixtureMetadataCommit": driver_commit,
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "assertionsPerRunner": driver["requiredAssertions"],
        "totalAssertions": driver["requiredAssertions"] * len(results),
        "observedOutput": marker,
        "nativeBuildConfiguration": "Release; WZSN_BUILD_SOKOL_HOST=ON",
        "targets": results, "fixtures": fixtures,
        "reference": driver["reference"],
        "scope": "Deterministic register and timing behavior; no analog waveform or physical audio claim."
    }
    output = (ROOT / args.output).resolve()
    if ROOT not in output.parents:
        raise SystemExit("output must remain under the repository root")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
