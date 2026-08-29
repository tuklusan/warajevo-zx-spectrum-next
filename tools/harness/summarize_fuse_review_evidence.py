#!/usr/bin/env python3
# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

"""Produce bounded, provenance-bound review evidence from pulled Fuse runs."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any

PROJECT_NOTICE = [
    "Warajevo ZX Spectrum Next",
    "Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.",
    "New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.",
    "Upstream Warajevo and third-party material retain their applicable copyrights and licenses.",
    "See LICENSE.txt and NOTICE.md for complete terms and provenance.",
]


def load_object(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise ValueError(f"invalid JSON evidence: {path}") from exc
    if not isinstance(value, dict):
        raise ValueError(f"JSON evidence must be an object: {path}")
    return value


def sha256_file(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require_project_path(root: Path, path: Path) -> Path:
    resolved = path.resolve()
    if resolved != root and root not in resolved.parents:
        raise ValueError(f"path must stay inside the project directory: {path}")
    return resolved


def required_string(value: Any, field: str) -> str:
    if not isinstance(value, str) or not value:
        raise ValueError(f"missing or invalid {field}")
    return value


def required_nonnegative_int(value: Any, field: str) -> int:
    if not isinstance(value, int) or isinstance(value, bool) or value < 0:
        raise ValueError(f"missing or invalid {field}")
    return value


def unique_strings(value: Any, field: str) -> set[str]:
    if not isinstance(value, list) or any(not isinstance(item, str) or not item for item in value):
        raise ValueError(f"missing or invalid {field}")
    result = set(value)
    if len(result) != len(value):
        raise ValueError(f"duplicate value in {field}")
    return result


def load_baseline(path: Path) -> tuple[str, set[str]]:
    baseline = load_object(path)
    return required_string(baseline.get("commit"), "baseline commit"), unique_strings(
        baseline.get("case_names"), "baseline case_names"
    )


def find_manifest(run_dir: Path) -> Path:
    candidates = sorted((run_dir / "unzipped").rglob("fuse-complete-manifest.json"))
    if len(candidates) != 1:
        raise ValueError(f"expected exactly one complete Fuse manifest under {run_dir}")
    return candidates[0]


def summarize_run(run_dir: Path, baseline_commit: str, baseline_cases: set[str],
                  required_cases: set[str]) -> dict[str, Any]:
    session_path = run_dir / "session.json"
    session = load_object(session_path)
    machine = required_string(session.get("machine"), "session machine")
    if session.get("action") != "smoke":
        raise ValueError(f"run is not a smoke run: {run_dir}")
    if session.get("primary_returncode") != 0 or session.get("pull_returncode") != 0:
        raise ValueError(f"run did not complete successfully: {run_dir}")

    manifest_path = find_manifest(run_dir)
    manifest = load_object(manifest_path)
    if required_string(manifest.get("commit"), "manifest commit") != baseline_commit:
        raise ValueError(f"manifest commit does not match baseline: {manifest_path}")
    total = required_nonnegative_int(manifest.get("total"), "manifest total")
    passed = required_nonnegative_int(manifest.get("passed"), "manifest passed")
    failed = required_nonnegative_int(manifest.get("failed"), "manifest failed")
    silent_skips = required_nonnegative_int(manifest.get("silent_skips"), "manifest silent_skips")
    cases = manifest.get("cases")
    if not isinstance(cases, list) or len(cases) != total:
        raise ValueError(f"case count does not match total: {manifest_path}")
    if passed + failed != total:
        raise ValueError(f"passed and failed totals do not match total: {manifest_path}")

    statuses: dict[str, str] = {}
    for case in cases:
        if not isinstance(case, dict):
            raise ValueError(f"invalid case record: {manifest_path}")
        name = required_string(case.get("name"), "case name")
        status = case.get("status")
        if status not in ("passed", "failed") or name in statuses:
            raise ValueError(f"invalid or duplicate case result: {manifest_path}")
        statuses[name] = status
    failed_cases = {name for name, status in statuses.items() if status == "failed"}
    if len(failed_cases) != failed:
        raise ValueError(f"failed case count does not match failed total: {manifest_path}")
    known_unresolved = unique_strings(manifest.get("known_unresolved"), "manifest known_unresolved")
    unexpected_failures = unique_strings(manifest.get("unexpected_failures"), "manifest unexpected_failures")
    if known_unresolved != failed_cases & baseline_cases:
        raise ValueError(f"known unresolved cases do not match baseline: {manifest_path}")
    if unexpected_failures != failed_cases - baseline_cases:
        raise ValueError(f"unexpected failures do not match baseline: {manifest_path}")

    missing_required = required_cases - statuses.keys()
    if missing_required:
        raise ValueError(f"required cases are absent: {sorted(missing_required)}")
    return {
        "name": machine,
        "run_id": required_string(session.get("run_id"), "session run_id"),
        "ctest": "passed",
        "total": total,
        "passed": passed,
        "known_unresolved": len(known_unresolved),
        "source_artifacts": {
            "session": {"path": str(session_path), "sha256": sha256_file(session_path)},
            "manifest": {"path": str(manifest_path), "sha256": sha256_file(manifest_path)},
        },
        "acceptance": {
            "required_cases": {name: statuses[name] for name in sorted(required_cases)},
            "unexpected_failures": sorted(unexpected_failures),
            "silent_skips": silent_skips,
            "remaining_failures_are_exactly_baseline": failed_cases == baseline_cases,
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cr", required=True)
    parser.add_argument("--build-id", required=True)
    parser.add_argument("--baseline", required=True, type=Path)
    parser.add_argument("--run-dir", required=True, type=Path, action="append")
    parser.add_argument("--required-case", required=True, action="append")
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    if not args.cr.startswith("CR-") or not args.build_id:
        parser.error("--cr and --build-id must be nonempty project identities")
    required_cases = set(args.required_case)
    if len(required_cases) != len(args.required_case) or any(not item for item in required_cases):
        parser.error("--required-case values must be unique nonempty identifiers")
    if len(set(args.run_dir)) != len(args.run_dir):
        parser.error("--run-dir values must be unique")
    project_root = Path.cwd().resolve()
    baseline_path = require_project_path(project_root, args.baseline)
    output_path = require_project_path(project_root, args.output)
    run_dirs = [require_project_path(project_root, path) for path in args.run_dir]
    baseline_commit, baseline_cases = load_baseline(baseline_path)
    machines = [summarize_run(path, baseline_commit, baseline_cases, required_cases)
                for path in run_dirs]
    names = [item["name"] for item in machines]
    if len(set(names)) != len(names):
        raise ValueError("duplicate machine evidence")

    output = {
        "project_notice": PROJECT_NOTICE,
        "schema_version": 1,
        "cr_number": args.cr,
        "build_id": args.build_id,
        "baseline": {"path": str(baseline_path), "sha256": sha256_file(baseline_path),
                     "commit": baseline_commit, "case_count": len(baseline_cases)},
        "machines": machines,
    }
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(output, indent=2, sort_keys=True) + "\n", encoding="ascii")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
