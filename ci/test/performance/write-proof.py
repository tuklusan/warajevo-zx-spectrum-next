#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Generate exact-source Phase-16 evidence on a hosted runner."""

import argparse
import hashlib
import json
import re
import subprocess
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
DRIVER = ROOT / "ci/test/performance/driver.json"
PLATFORMS = (
    ("Windows x64", "windows-x64", "Windows-windows-x64", "X64"),
    ("Linux x64", "linux-x64", "Linux-ubuntu-24.04", "X64"),
    ("Linux ARM64", "linux-arm64", "Linux-ubuntu-24.04-arm", "ARM64"),
    ("macOS ARM64", "macos-arm64", "macOS-macos-arm64", "ARM64"),
    ("macOS Intel", "macos-x64", "macOS-macos-x64", "X64"),
)
WAIT_MARKER = "PASS Interface 1 WAIT pending-cycle resume (75 checks)"
MAX_SLOWDOWN = 0.40


def gh_json(*args: str) -> dict:
    return json.loads(subprocess.check_output(
        ["gh", *args], cwd=ROOT, text=True, stderr=subprocess.PIPE))


def compare(current: dict, baseline: dict) -> list[dict]:
    if (current.get("runner") != baseline.get("runner") or
            current.get("architecture") != baseline.get("architecture")):
        raise ValueError("performance record does not match its runner baseline")
    actual = {item["name"]: item for item in current["metrics"]}
    expected = {item["name"]: item for item in baseline["metrics"]}
    if not actual or actual.keys() != expected.keys():
        raise ValueError("performance metric sets differ")
    deltas = []
    for name, metric in actual.items():
        reference = expected[name]
        if (metric["unit"] != reference["unit"] or
                metric["operations"] != reference["operations"]):
            raise ValueError(f"{name}: workload shape differs from baseline")
        if metric["fingerprint"] != reference["fingerprint"]:
            raise ValueError(f"{name}: deterministic fingerprint changed")
        baseline_rate = float(reference["operations_per_second"])
        current_rate = float(metric["operations_per_second"])
        if baseline_rate <= 0 or current_rate <= 0:
            raise ValueError(f"{name}: throughput must be positive")
        delta = current_rate / baseline_rate - 1.0
        if delta < -MAX_SLOWDOWN:
            raise ValueError(f"{name}: {delta:.1%} throughput regression exceeds 40%")
        deltas.append({"metric": name, "deltaPercent": round(delta * 100, 1)})
    return deltas


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--commit", required=True)
    parser.add_argument("--run-id", required=True, type=int)
    parser.add_argument("--input-dir", required=True)
    args = parser.parse_args()
    if re.fullmatch(r"[0-9a-f]{40}", args.commit) is None:
        raise SystemExit("commit must be a full lowercase Git SHA")

    driver = json.loads(DRIVER.read_text(encoding="utf-8"))
    if driver.get("testId") != "phase16-performance-baseline":
        raise SystemExit("unexpected performance driver")
    fixtures = []
    for fixture in driver.get("fixtures", []):
        path = fixture["path"]
        source = (ROOT / path).resolve()
        if ROOT not in source.parents or not source.is_file():
            raise SystemExit(f"unsafe or missing performance fixture: {path}")
        digest = hashlib.sha256(source.read_bytes()).hexdigest()
        if digest != fixture["sha256"]:
            raise SystemExit(f"performance fixture hash mismatch: {path}")
        fixtures.append({"path": path, "sha256": digest})

    run = gh_json("run", "view", str(args.run_id), "--json",
                  "headSha,status,conclusion,event,headBranch,jobs")
    if (run.get("headSha") != args.commit or
            run.get("event") not in ("push", "workflow_dispatch") or
            run.get("headBranch") != "main"):
        raise SystemExit("workflow run does not match the requested main-branch source")

    input_dir = Path(args.input_dir).resolve()
    if ROOT not in input_dir.parents or not input_dir.is_dir():
        raise SystemExit("hosted runner evidence directory is missing or outside the repository")

    results = []
    raw_dir = ROOT / "test-results/performance-current"
    raw_dir.mkdir(parents=True, exist_ok=True)
    max_slowdown = 0.0
    for platform, slug, runner, architecture in PLATFORMS:
        job_name = f"Full build / {platform}"
        job = next((item for item in run["jobs"]
                    if item.get("name") == job_name), None)
        if job is None or job.get("conclusion") != "success":
            raise SystemExit(f"successful native job missing: {platform}")
        runner_path = input_dir / f"runner-{slug}.json"
        ctest_path = input_dir / f"ctest-{slug}.log"
        performance_path = input_dir / f"performance-{slug}.json"
        if not runner_path.is_file() or not ctest_path.is_file() or not performance_path.is_file():
            raise SystemExit(f"hosted runner evidence files missing: {platform}")
        runner_record = json.loads(runner_path.read_text(encoding="utf-8"))
        ctest = ctest_path.read_text(encoding="utf-8", errors="replace")
        if (runner_record.get("status") != "pass" or
                runner_record.get("commit") != args.commit or
                runner_record.get("runner") != runner or
                runner_record.get("architecture") != architecture or
                runner_record.get("requiredCases") != 75 or
                runner_record.get("observedOutput") != WAIT_MARKER or
                runner_record.get("ctest") != "26/26 passed" or
                WAIT_MARKER not in ctest or
                re.search(r"100% tests passed(?:, 0 tests failed)? out of 26", ctest) is None):
            raise SystemExit(f"Interface 1 WAIT/CTest evidence missing: {platform}")

        record = json.loads(performance_path.read_text(encoding="utf-8"))
        if record.get("runner") != runner or record.get("architecture") != architecture:
            raise SystemExit(f"unexpected performance runner identity: {platform}")
        if record.get("commit") != args.commit or record.get("status") != "pass":
            raise SystemExit(f"performance record does not match source: {platform}")
        output = raw_dir / f"{slug}.json"
        output.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
        baseline_path = ROOT / f"ci/test/performance/baselines/{slug}.json"
        baseline = json.loads(baseline_path.read_text(encoding="utf-8"))
        deltas = compare(record, baseline)
        largest = max((max(0.0, -item["deltaPercent"]) for item in deltas), default=0.0)
        max_slowdown = max(max_slowdown, largest)
        results.append({
            "performanceRecord": output.relative_to(ROOT).as_posix(),
            "comparison": {
                "gate": "all fingerprints match; no throughput regression exceeded 40%",
                "status": "pass", "largestSlowdownPercent": largest,
                "baseline": baseline_path.relative_to(ROOT).as_posix(),
                "metricDeltasPercent": deltas,
            },
            "ctest": "26/26 passed", "runner": runner,
            "timestamp": record["timestamp"], "jobId": job["databaseId"],
            "metricCount": len(record["metrics"]), "status": "pass",
        })

    wait_proof_path = ROOT / "test-results/interface1-wait.json"
    wait_proof = json.loads(wait_proof_path.read_text(encoding="utf-8"))
    if wait_proof.get("status") != "pass" or wait_proof.get("commit") != args.commit:
        raise SystemExit("hosted Interface 1 WAIT proof is missing or mismatched")

    proof = {
        "commit": args.commit, "sourceCommit": args.commit,
        "testId": driver["testId"], "status": "pass",
        "workflowRun": f"https://github.com/tuklusan/warajevo-zx-spectrum-next/actions/runs/{args.run_id}",
        "runId": args.run_id,
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "fixtures": fixtures, "runnerResults": results,
        "summary": {
            "nativeTargets": len(results), "ctestPerTarget": "26/26 passed",
            "fingerprintChecks": sum(item["metricCount"] for item in results),
            "throughputChecks": sum(item["metricCount"] for item in results),
            "largestSlowdownPercent": round(max_slowdown, 1),
        },
        "sourceReview": wait_proof["sourceReview"],
        "githubCleanupPolicy": "Download the temporary proof artifact, then delete the workflow run; this workflow does not create caches.",
    }
    (ROOT / "test-results/phase16-performance-current.json").write_text(
        json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
