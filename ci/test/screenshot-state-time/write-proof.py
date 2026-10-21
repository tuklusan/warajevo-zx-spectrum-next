#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Pin five-target screenshot state/time evidence from hosted build logs."""

import argparse
import hashlib
import json
import re
import subprocess
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath


ROOT = Path(__file__).resolve().parents[3]
DRIVER_PATH = "test-drivers/screenshot-state-time.driver.json"
FIXTURE_ROOTS = (".github/workflows/", "ci/test/screenshot-state-time/",
                 "docs/design/", "src/app/", "src/cmake/", "src/core/",
                 "test-drivers/", "tests/")
PLATFORMS = (
    ("Windows x64", "windows-x64", "Windows-windows-x64", "X64"),
    ("Linux x64", "linux-x64", "Linux-ubuntu-24.04", "X64"),
    ("Linux ARM64", "linux-arm64", "Linux-ubuntu-24.04-arm", "ARM64"),
    ("macOS ARM64", "macos-arm64", "macOS-macos-arm64", "ARM64"),
    ("macOS Intel", "macos-x64", "macOS-macos-x64", "X64"),
)
MARKER = ("PASS screenshot success and failure preserve canonical machine hash "
          "and master tick (20 assertions)")


def commit_file(commit: str, path: str) -> bytes:
    relative = PurePosixPath(path)
    if (not path.startswith(FIXTURE_ROOTS) or
            re.fullmatch(r"[A-Za-z0-9._/-]+", path) is None or
            relative.is_absolute() or ".." in relative.parts):
        raise SystemExit(f"unsafe fixture path: {path}")
    try:
        return subprocess.check_output(["git", "show", f"{commit}:{path}"],
                                       cwd=ROOT, stderr=subprocess.PIPE)
    except subprocess.CalledProcessError:
        # Proof-only dispatches use a shallow checkout while reading an older
        # completed runner commit. Fetch that exact reachable commit as needed.
        subprocess.run(["git", "fetch", "--no-tags", "origin", commit],
                       cwd=ROOT, check=True, stdout=subprocess.PIPE,
                       stderr=subprocess.PIPE)
        return subprocess.check_output(["git", "show", f"{commit}:{path}"],
                                       cwd=ROOT, stderr=subprocess.PIPE)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--commit", required=True)
    parser.add_argument("--driver-commit")
    parser.add_argument("--run-id", required=True, type=int)
    parser.add_argument("--branch", required=True)
    parser.add_argument("--event", required=True)
    parser.add_argument("--input-dir", required=True)
    parser.add_argument("--output", default="test-results/screenshot-state-time.json")
    args = parser.parse_args()
    if re.fullmatch(r"[0-9a-f]{40}", args.commit) is None:
        raise SystemExit("commit must be a full lowercase Git SHA")
    driver_commit = args.driver_commit or args.commit
    if re.fullmatch(r"[0-9a-f]{40}", driver_commit) is None:
        raise SystemExit("driver commit must be a full lowercase Git SHA")
    if args.branch != "main" or args.event != "workflow_dispatch":
        raise SystemExit("screenshot state/time proof requires a main dispatch")

    driver = json.loads(commit_file(driver_commit, DRIVER_PATH))
    if (driver.get("testId") != "screenshot-state-time" or
            driver.get("requiredAssertions") != 20):
        raise SystemExit("unexpected screenshot state/time test driver")
    fixtures = []
    for fixture in driver.get("fixtures", []):
        path = fixture.get("path", "")
        digest = hashlib.sha256(commit_file(driver_commit, path)).hexdigest()
        if digest != fixture.get("sha256"):
            raise SystemExit(f"fixture hash mismatch: {path}")
        fixtures.append({"path": path, "sha256": digest})

    input_dir = Path(args.input_dir).resolve()
    if ROOT not in input_dir.parents or not input_dir.is_dir():
        raise SystemExit("hosted runner evidence directory is missing")

    targets = []
    for platform, slug, runner, architecture in PLATFORMS:
        evidence_path = input_dir / f"runner-{slug}.json"
        build_path = input_dir / f"build-{slug}.log"
        if not evidence_path.is_file() or not build_path.is_file():
            raise SystemExit(f"hosted evidence missing: {platform}")
        evidence = json.loads(evidence_path.read_text(encoding="utf-8"))
        build_log = build_path.read_text(encoding="utf-8", errors="replace")
        ctest_record = evidence.get("ctest", "")
        if (evidence.get("status") != "pass" or
                evidence.get("commit") != args.commit or
                evidence.get("jobName") != f"Full build / {platform}" or
                evidence.get("runner") != runner or
                evidence.get("architecture") != architecture or
                re.fullmatch(r"([1-9]\d*)/\1 passed", ctest_record) is None or
                MARKER not in build_log):
            raise SystemExit(f"screenshot contract did not pass: {platform}")
        targets.append({
            "name": platform, "runner": runner,
            "architecture": architecture, "status": "pass",
            "assertions": driver["requiredAssertions"],
            "observedOutput": MARKER, "ctest": ctest_record,
        })

    proof = {
        "testId": driver["testId"], "status": "pass",
        "commit": args.commit,
        "runner": "Windows x64, Linux x64, Linux ARM64, macOS ARM64, and macOS Intel hosted runners",
        "runId": args.run_id,
        "workflowRun": f"https://github.com/tuklusan/warajevo-zx-spectrum-next/actions/runs/{args.run_id}",
        "workflowEvent": args.event, "branch": args.branch,
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "assertionsPerRunner": driver["requiredAssertions"],
        "totalAssertions": driver["requiredAssertions"] * len(targets),
        "nativeBuildConfiguration": "Release; WZSN_BUILD_SOKOL_HOST=ON",
        "targets": targets, "fixtures": fixtures,
    }
    output = (ROOT / args.output).resolve()
    if ROOT not in output.parents:
        raise SystemExit("proof output is outside the repository")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    print(f"PASS screenshot state/time proof: {len(targets)} platforms")


if __name__ == "__main__":
    main()
