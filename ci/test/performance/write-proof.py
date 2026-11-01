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
EXIT_MARKER = re.compile(
    r"debugger-monitor-exit-contract assertions=(\d+) status=pass")
FIND_MARKER = re.compile(
    r"debugger-monitor-find-contract assertions=(\d+) status=pass")
MAX_SLOWDOWN = 0.40


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
    parser.add_argument("--driver-commit")
    parser.add_argument("--run-id", required=True, type=int)
    parser.add_argument("--branch", required=True)
    parser.add_argument("--event", required=True)
    parser.add_argument("--input-dir", required=True)
    args = parser.parse_args()
    if re.fullmatch(r"[0-9a-f]{40}", args.commit) is None:
        raise SystemExit("commit must be a full lowercase Git SHA")
    driver_commit = args.driver_commit or args.commit
    if re.fullmatch(r"[0-9a-f]{40}", driver_commit) is None:
        raise SystemExit("driver commit must be a full lowercase Git SHA")

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

    if args.branch != "main" or args.event not in ("push", "workflow_dispatch"):
        raise SystemExit("workflow run does not match the requested main-branch source")

    input_dir = Path(args.input_dir).resolve()
    if ROOT not in input_dir.parents or not input_dir.is_dir():
        raise SystemExit("hosted runner evidence directory is missing or outside the repository")

    results = []
    exit_results = []
    find_results = []
    raw_dir = ROOT / "test-results/performance-current"
    raw_dir.mkdir(parents=True, exist_ok=True)
    max_slowdown = 0.0
    ctest_count = None
    for platform, slug, runner, architecture in PLATFORMS:
        job_name = f"Full build / {platform}"
        runner_path = input_dir / f"runner-{slug}.json"
        ctest_path = input_dir / f"ctest-{slug}.log"
        performance_path = input_dir / f"performance-{slug}.json"
        if not runner_path.is_file() or not ctest_path.is_file() or not performance_path.is_file():
            raise SystemExit(f"hosted runner evidence files missing: {platform}")
        runner_record = json.loads(runner_path.read_text(encoding="utf-8"))
        ctest = ctest_path.read_text(encoding="utf-8", errors="replace")
        ctest_summary = re.search(
            r"100% tests passed(?:, 0 tests failed)? out of (\d+)", ctest)
        if ctest_summary is None or int(ctest_summary.group(1)) == 0:
            raise SystemExit(f"successful CTest completion is missing: {platform}")
        runner_ctest_count = int(ctest_summary.group(1))
        if ctest_count is None:
            ctest_count = runner_ctest_count
        elif ctest_count != runner_ctest_count:
            raise SystemExit("native runners completed different CTest counts")
        ctest_record = f"{runner_ctest_count}/{runner_ctest_count} passed"
        exit_match = EXIT_MARKER.search(ctest)
        if exit_match is None or int(exit_match.group(1)) != 24:
            raise SystemExit(f"monitor E contract evidence missing: {platform}")
        find_match = FIND_MARKER.search(ctest)
        if find_match is None or int(find_match.group(1)) != 15:
            raise SystemExit(f"monitor F contract evidence missing: {platform}")
        if (runner_record.get("status") != "pass" or
                runner_record.get("commit") != args.commit or
                runner_record.get("jobName") != job_name or
                runner_record.get("runner") != runner or
                runner_record.get("architecture") != architecture or
                runner_record.get("requiredCases") != 75 or
                runner_record.get("observedOutput") != WAIT_MARKER or
                runner_record.get("ctest") != ctest_record or
                WAIT_MARKER not in ctest):
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
            "ctest": ctest_record, "runner": runner,
            "timestamp": record["timestamp"], "jobName": job_name,
            "metricCount": len(record["metrics"]), "status": "pass",
        })
        exit_results.append({
            "runner": platform, "jobName": job_name,
            "assertions": int(exit_match.group(1)), "ctest": ctest_record,
            "status": "pass",
        })
        find_results.append({
            "runner": platform, "jobName": job_name,
            "assertions": int(find_match.group(1)), "ctest": ctest_record,
            "status": "pass",
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
        "workflowEvent": args.event, "branch": args.branch,
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "fixtures": fixtures, "runnerResults": results,
        "summary": {
            "nativeTargets": len(results),
            "ctestPerTarget": f"{ctest_count}/{ctest_count} passed",
            "fingerprintChecks": sum(item["metricCount"] for item in results),
            "throughputChecks": sum(item["metricCount"] for item in results),
            "largestSlowdownPercent": round(max_slowdown, 1),
        },
        "sourceReview": wait_proof["sourceReview"],
        "githubCleanupPolicy": "Download the temporary proof artifact, then delete the workflow run; this workflow does not create caches.",
    }
    (ROOT / "test-results/phase16-performance-current.json").write_text(
        json.dumps(proof, indent=2) + "\n", encoding="utf-8")

    exit_driver = json.loads(
        (ROOT / "test-drivers/debugger-monitor-exit-contract.driver.json")
        .read_text(encoding="utf-8"))
    if (exit_driver.get("testId") != "debugger-monitor-exit-contract" or
            exit_driver.get("requiredAssertions") != 24 or
            exit_driver.get("runnerMatrix") != [item[0] for item in PLATFORMS]):
        raise SystemExit("unexpected monitor E driver")
    exit_fixtures = []
    for fixture in exit_driver.get("fixtures", []):
        path = fixture["path"]
        source = (ROOT / path).resolve()
        if ROOT not in source.parents or not source.is_file():
            raise SystemExit(f"unsafe or missing monitor E fixture: {path}")
        digest = hashlib.sha256(source.read_bytes()).hexdigest()
        if digest != fixture["sha256"]:
            raise SystemExit(f"monitor E fixture hash mismatch: {path}")
        exit_fixtures.append({"path": path, "sha256": digest})
    exit_proof = {
        "commit": args.commit, "sourceCommit": args.commit,
        "driverCommit": driver_commit,
        "testId": exit_driver["testId"], "status": "pass",
        "workflowRun": proof["workflowRun"], "runId": args.run_id,
        "workflowEvent": args.event, "branch": args.branch,
        "timestamp": proof["timestamp"], "fixtures": exit_fixtures,
        "build": {"configuration": "Release", "sokolHost": "ON"},
        "assertionsPerTarget": 24, "runnerResults": exit_results,
        "summary": {"nativeTargets": len(exit_results),
                    "assertions": sum(item["assertions"] for item in exit_results)},
        "sourceReview": wait_proof["sourceReview"],
    }
    (ROOT / "test-results/debugger-monitor-exit-contract.json").write_text(
        json.dumps(exit_proof, indent=2) + "\n", encoding="utf-8")

    find_driver = json.loads(
        (ROOT / "test-drivers/debugger-monitor-find-contract.driver.json")
        .read_text(encoding="utf-8"))
    if (find_driver.get("testId") != "debugger-monitor-find-contract" or
            find_driver.get("requiredAssertions") != 15 or
            find_driver.get("runnerMatrix") != [item[0] for item in PLATFORMS]):
        raise SystemExit("unexpected monitor F driver")
    find_fixtures = []
    for fixture in find_driver.get("fixtures", []):
        path = fixture["path"]
        source = (ROOT / path).resolve()
        if ROOT not in source.parents or not source.is_file():
            raise SystemExit(f"unsafe or missing monitor F fixture: {path}")
        digest = hashlib.sha256(source.read_bytes()).hexdigest()
        if digest != fixture["sha256"]:
            raise SystemExit(f"monitor F fixture hash mismatch: {path}")
        find_fixtures.append({"path": path, "sha256": digest})
    find_proof = {
        "commit": args.commit, "sourceCommit": args.commit,
        "driverCommit": driver_commit,
        "testId": find_driver["testId"], "status": "pass",
        "workflowRun": proof["workflowRun"], "runId": args.run_id,
        "workflowEvent": args.event, "branch": args.branch,
        "timestamp": proof["timestamp"], "fixtures": find_fixtures,
        "build": {"configuration": "Release", "sokolHost": "ON"},
        "assertionsPerTarget": 15, "runnerResults": find_results,
        "summary": {"nativeTargets": len(find_results),
                    "assertions": sum(item["assertions"] for item in find_results)},
        "sourceReview": wait_proof["sourceReview"],
        "githubCleanupPolicy": "Temporary runner/proof artifacts and the workflow run are deleted after evidence is copied into the repository; this workflow creates no caches.",
    }
    (ROOT / "test-results/debugger-monitor-find-contract.json").write_text(
        json.dumps(find_proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
