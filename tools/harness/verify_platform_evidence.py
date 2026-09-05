# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any


def load_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise SystemExit(f"invalid evidence JSON: {path}") from exc


def file_digest(path: Path, relative_root: Path | None = None) -> dict[str, Any]:
    data = path.read_bytes()
    if not data:
        raise SystemExit(f"empty evidence file: {path}")
    display_path = path.relative_to(relative_root).as_posix() if relative_root else path.as_posix()
    return {"path": display_path, "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}


def inspect_lane(artifact_dir: Path, lane_id: str | None, visual_supported: bool) -> dict[str, Any]:
    summary = load_json(artifact_dir / "summary.json")
    inventory = load_json(artifact_dir / "inventory.json")
    fuse_manifest = load_json(artifact_dir / "fuse-complete-manifest.json")
    if summary.get("status") != "passed":
        raise SystemExit(f"lane summary is not passed: {artifact_dir}")
    if summary.get("missing_tools"):
        raise SystemExit(f"lane has missing tools: {artifact_dir}")
    if not isinstance(fuse_manifest, dict) or not fuse_manifest:
        raise SystemExit(f"invalid Fuse result manifest: {artifact_dir}")

    screenshots = []
    traces = []
    for path in artifact_dir.rglob("*"):
        if not path.is_file() or path.name in {"result-manifest.json", "evidence-inspection.json"}:
            continue
        if path.suffix.lower() in {".png", ".jpg", ".jpeg", ".bmp"}:
            screenshots.append(file_digest(path, artifact_dir))
        elif "trace" in path.name.lower() or path.suffix.lower() in {".trace", ".jsonl"}:
            traces.append(file_digest(path, artifact_dir))
    if visual_supported and not screenshots:
        raise SystemExit(f"visual evidence required but absent: {artifact_dir}")

    inspection = {
        "lane_id": lane_id or artifact_dir.name,
        "summary_status": summary.get("status"),
        "platform": inventory.get("platform", {}),
        "visual_supported": visual_supported,
        "screenshots": screenshots,
        "traces": traces,
        "screenshot_status": "inspected" if screenshots else "not_supported",
        "trace_status": "inspected" if traces else "not_produced",
    }
    (artifact_dir / "evidence-inspection.json").write_text(
        json.dumps(inspection, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    result = {
        "schema_version": 1,
        "lane_id": inspection["lane_id"],
        "summary": summary,
        "inventory": inventory,
        "fuse_manifest": fuse_manifest,
        "inspection": inspection,
    }
    (artifact_dir / "result-manifest.json").write_text(
        json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description="Verify and record hosted platform evidence.")
    parser.add_argument("--artifact-dir", type=Path)
    parser.add_argument("--lane-id")
    parser.add_argument("--visual-supported", action="store_true")
    parser.add_argument("--verify-tree", type=Path)
    args = parser.parse_args()
    if args.artifact_dir:
        inspect_lane(args.artifact_dir, args.lane_id, args.visual_supported)
        return 0
    if args.verify_tree:
        manifests = sorted(args.verify_tree.rglob("result-manifest.json"))
        if not manifests:
            raise SystemExit("no lane result manifests found")
        for manifest in manifests:
            result = load_json(manifest)
            if result.get("schema_version") != 1 or result.get("summary", {}).get("status") != "passed":
                raise SystemExit(f"invalid lane result manifest: {manifest}")
            inspection = result.get("inspection", {})
            for evidence in inspection.get("screenshots", []) + inspection.get("traces", []):
                path = manifest.parent / evidence["path"]
                if not path.exists() or file_digest(path)["sha256"] != evidence["sha256"]:
                    raise SystemExit(f"retained evidence hash mismatch: {path}")
        return 0
    parser.error("one of --artifact-dir or --verify-tree is required")
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
