# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

"""Canonical deterministic WZXN hosted-lane evidence verifier, schema v2."""
from __future__ import annotations
import argparse, json
from pathlib import Path
from typing import Any

from evidence_schema_v2 import (
    EXPECTED_FUSE_MANIFESTS, SOKOL_REQUIRED_LANES, expected_lanes, file_digest,
    load_unresolved_baseline, validate_lane_result,
)


def load_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:
        raise SystemExit(f"invalid evidence JSON: {path}") from exc


def _baseline(project_root: Path) -> tuple[set[str], dict[str, Any]]:
    try:
        names, meta = load_unresolved_baseline(project_root / "tools/harness/fuse-unresolved-baseline.json")
    except ValueError as exc:
        raise SystemExit(str(exc)) from exc
    meta = {**meta, "path": "tools/harness/fuse-unresolved-baseline.json"}
    return names, meta


def inspect_lane(artifact_dir: Path, lane_id: str | None, visual_supported: bool) -> dict[str, Any]:
    lane = lane_id or artifact_dir.name
    project_root = Path.cwd().resolve()
    baseline_names, baseline_meta = _baseline(project_root)
    summary_path = artifact_dir / "summary.json"
    inventory_path = artifact_dir / "inventory.json"
    summary = load_json(summary_path)
    inventory = load_json(inventory_path)
    if summary.get("status") != "passed":
        raise SystemExit(f"lane summary is not passed: {lane}")
    if summary.get("missing_tools"):
        raise SystemExit(f"lane has missing tools: {lane}")

    source_files: dict[str, Any] = {
        "summary": file_digest(summary_path, artifact_dir),
        "inventory": file_digest(inventory_path, artifact_dir),
        "unresolved_baseline": baseline_meta,
    }
    fuse_manifests: list[dict[str, Any]] = []
    for name, (_selection, _total) in EXPECTED_FUSE_MANIFESTS.items():
        path = artifact_dir / name
        if not path.is_file():
            raise SystemExit(f"required Fuse manifest absent: {lane}/{name}")
        payload = load_json(path)
        rec = file_digest(path, artifact_dir)
        source_files[f"fuse:{name}"] = rec
        fuse_manifests.append({"name": name, "digest": rec, "payload": payload})

    screenshots: list[dict[str, Any]] = []
    traces: list[dict[str, Any]] = []
    for path in artifact_dir.rglob("*"):
        if path.is_symlink():
            raise SystemExit(f"evidence symlink forbidden: {path}")
        if not path.is_file() or path.name in {"result-manifest.json", "evidence-inspection.json"}:
            continue
        if path.suffix.lower() in {".png", ".jpg", ".jpeg", ".bmp"}:
            screenshots.append(file_digest(path, artifact_dir))
        elif "trace" in path.name.lower() or path.suffix.lower() in {".trace", ".jsonl"}:
            traces.append(file_digest(path, artifact_dir))
    if visual_supported and not screenshots:
        raise SystemExit(f"visual evidence required but absent: {lane}")

    sokol_dir = artifact_dir / "sokol-host"
    sokol: dict[str, Any] = {"required": lane in SOKOL_REQUIRED_LANES, "status": "not_applicable"}
    if lane in SOKOL_REQUIRED_LANES:
        sokol_summary_path = sokol_dir / "summary.json"
        if not sokol_summary_path.is_file():
            raise SystemExit(f"required Sokol evidence absent: {lane}")
        sokol_summary = load_json(sokol_summary_path)
        if sokol_summary.get("status") != "passed":
            raise SystemExit(f"Sokol host smoke did not pass: {lane}")
        sokol = {"required": True, "status": "passed", "summary": sokol_summary,
                 "summary_digest": file_digest(sokol_summary_path, artifact_dir)}
        source_files["sokol_summary"] = sokol["summary_digest"]

    inspection = {
        "lane_id": lane,
        "summary_status": "passed",
        "platform": inventory.get("platform", {}),
        "visual_supported": visual_supported,
        "screenshots": screenshots,
        "traces": traces,
        "screenshot_status": "present_and_hash_verified" if screenshots else "not_produced",
        "trace_status": "present_and_hash_verified" if traces else "not_produced",
    }
    (artifact_dir / "evidence-inspection.json").write_text(
        json.dumps(inspection, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    result = {
        "schema_version": 2,
        "lane_id": lane,
        "summary": summary,
        "inventory": inventory,
        # Backward navigational field; canonical semantics are the full family below.
        "fuse_manifest": next(x["payload"] for x in fuse_manifests if x["name"] == "fuse-complete-manifest.json"),
        "fuse_manifests": fuse_manifests,
        "unresolved_baseline": baseline_meta,
        "source_files": source_files,
        "inspection": inspection,
        "sokol": sokol,
    }
    try:
        validate_lane_result(result, lane, baseline_names)
    except ValueError as exc:
        raise SystemExit(f"invalid lane evidence semantics {lane}: {exc}") from exc
    (artifact_dir / "result-manifest.json").write_text(
        json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    return result


def verify_manifest(path: Path, project_root: Path) -> dict[str, Any]:
    result = load_json(path)
    baseline_names, baseline_meta = _baseline(project_root)
    try:
        validate_lane_result(result, unresolved_baseline=baseline_names)
    except ValueError as exc:
        raise SystemExit(f"invalid lane result manifest {path}: {exc}") from exc
    if result.get("unresolved_baseline", {}).get("sha256") != baseline_meta["sha256"]:
        raise SystemExit(f"lane unresolved-baseline binding mismatch: {path}")
    source_files = result.get("source_files")
    if not isinstance(source_files, dict) or not source_files:
        raise SystemExit(f"manifest lacks structured source hashes: {path}")
    for key, rec in source_files.items():
        if not isinstance(rec, dict) or not isinstance(rec.get("path"), str):
            raise SystemExit(f"malformed source digest: {path}")
        if key == "unresolved_baseline":
            target = project_root / rec["path"]
        else:
            target = path.parent / rec["path"]
        if not target.is_file() or file_digest(target, allow_empty=True)["sha256"] != rec.get("sha256"):
            raise SystemExit(f"structured source hash mismatch: {target}")
    inspection = result.get("inspection", {})
    for rec in inspection.get("screenshots", []) + inspection.get("traces", []):
        target = path.parent / rec["path"]
        if not target.is_file() or file_digest(target, allow_empty=True)["sha256"] != rec.get("sha256"):
            raise SystemExit(f"retained evidence hash mismatch: {target}")
    return result


def verify_tree(tree: Path, workflow: Path | None, expected_count: int) -> None:
    project_root = Path.cwd().resolve()
    manifests = sorted(tree.rglob("result-manifest.json"))
    if len(manifests) != expected_count:
        raise SystemExit(f"expected {expected_count} lane manifests, found {len(manifests)}")
    results = [verify_manifest(path, project_root) for path in manifests]
    lanes = [result["lane_id"] for result in results]
    if len(lanes) != len(set(lanes)):
        raise SystemExit("duplicate embedded lane IDs")
    if workflow:
        try:
            expected = expected_lanes(workflow.read_text(encoding="utf-8"))
        except ValueError as exc:
            raise SystemExit(str(exc)) from exc
        if sorted(lanes) != sorted(expected):
            raise SystemExit("retained lane IDs do not match authoritative 20-lane workflow")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--artifact-dir", type=Path)
    parser.add_argument("--lane-id")
    parser.add_argument("--visual-supported", action="store_true")
    parser.add_argument("--verify-tree", type=Path)
    parser.add_argument("--workflow-file", type=Path)
    parser.add_argument("--expected-count", type=int, default=20)
    args = parser.parse_args()
    if args.artifact_dir:
        inspect_lane(args.artifact_dir, args.lane_id, args.visual_supported)
        return 0
    if args.verify_tree:
        verify_tree(args.verify_tree, args.workflow_file, args.expected_count)
        return 0
    parser.error("one of --artifact-dir or --verify-tree is required")
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
