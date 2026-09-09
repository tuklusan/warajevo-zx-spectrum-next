#!/usr/bin/env python3
# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

"""Canonical deterministic hosted-evidence schema/invariants for WZXN protocol v4."""
from __future__ import annotations
import hashlib, json, re
from pathlib import Path
from typing import Any

PINNED_FUSE_COMMIT = "e7fe9a0f3625b0aef649b114fc98323c7d241840"
SOKOL_REQUIRED_LANES = {"windows-latest", "macos-arm64-14"}
EXPECTED_FUSE_MANIFESTS: dict[str, tuple[str, int | None]] = {
    "fuse-complete-manifest.json": ("every pinned Fuse Z80 vector from tests.in", None),
    "fuse-ed-manifest.json": ("all case names beginning with ed", 109),
    "fuse-indexed-cb-manifest.json": ("all case names beginning with ddcb or fdcb", 512),
    "fuse-cb-rotate-shift-manifest.json": ("all CB rotate/shift cases from cb00 through cb3f", 64),
    "fuse-stack-subroutine-manifest.json": ("all CALL, RET, RST, PUSH, and POP cases including conditional paths", 50),
    "fuse-branch-manifest.json": ("all JR, JP, and DJNZ cases including taken and not-taken paths", 28),
    "fuse-inc-dec-manifest.json": ("all primary INC and DEC register and memory cases", 16),
    "fuse-halt-manifest.json": ("the primary HALT state and timing case", 1),
}


def canonical_json(value: Any) -> str:
    return json.dumps(value, ensure_ascii=True, separators=(",", ":"), sort_keys=True)


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def file_digest(path: Path, root: Path | None = None, *, allow_empty: bool = False) -> dict[str, Any]:
    h = hashlib.sha256(); size = 0
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(chunk); size += len(chunk)
    if not size and not allow_empty:
        raise ValueError(f"empty evidence file: {path}")
    return {"path": path.relative_to(root).as_posix() if root else path.as_posix(),
            "bytes": size, "sha256": h.hexdigest()}


def load_unresolved_baseline(path: Path) -> tuple[set[str], dict[str, Any]]:
    try:
        data = path.read_bytes(); payload = json.loads(data.decode("utf-8"))
    except Exception as exc:
        raise ValueError("invalid Fuse unresolved baseline") from exc
    names = payload.get("case_names") if isinstance(payload, dict) else None
    if payload.get("commit") != PINNED_FUSE_COMMIT or not isinstance(names, list) or not all(isinstance(x, str) for x in names):
        raise ValueError("Fuse unresolved baseline identity/schema mismatch")
    if len(names) != len(set(names)):
        raise ValueError("Fuse unresolved baseline contains duplicate case names")
    return set(names), {"path": path.as_posix(), "sha256": sha256_bytes(data), "case_count": len(names)}


def validate_fuse_manifest(payload: Any, *, expected_selection: str | None = None,
                           expected_total: int | None = None,
                           unresolved_baseline: set[str] | None = None) -> None:
    if not isinstance(payload, dict):
        raise ValueError("Fuse manifest is not an object")
    required = ("schema_version", "corpus", "commit", "suite_path", "selection", "total", "passed", "failed",
                "silent_skips", "known_unresolved", "unexpected_failures", "cases")
    if any(k not in payload for k in required):
        raise ValueError("Fuse manifest lacks required fields")
    if payload.get("schema_version") != 1 or payload.get("corpus") != "Fuse Z80" or payload.get("suite_path") != "z80/tests":
        raise ValueError("Fuse manifest schema/corpus identity mismatch")
    if payload.get("commit") != PINNED_FUSE_COMMIT:
        raise ValueError("Fuse pinned commit mismatch")
    if expected_selection is not None and payload.get("selection") != expected_selection:
        raise ValueError("Fuse selection identity mismatch")
    if not all(isinstance(payload.get(k), int) and payload[k] >= 0 for k in ("total", "passed", "failed", "silent_skips")):
        raise ValueError("Fuse counters malformed")
    if expected_total is not None and payload["total"] != expected_total:
        raise ValueError("Fuse selection count mismatch")
    if payload["passed"] + payload["failed"] != payload["total"]:
        raise ValueError("Fuse totals inconsistent")
    if payload["silent_skips"] != 0:
        raise ValueError("Fuse silent skips are non-zero")
    if not isinstance(payload["unexpected_failures"], list) or payload["unexpected_failures"]:
        raise ValueError("Fuse has unexpected failures")
    if not isinstance(payload["known_unresolved"], list) or not isinstance(payload["cases"], list):
        raise ValueError("Fuse result lists malformed")
    if len(payload["cases"]) != payload["total"]:
        raise ValueError("Fuse case count inconsistent")
    names: list[str] = []
    failed: set[str] = set()
    passed = 0
    for case in payload["cases"]:
        if not isinstance(case, dict) or not isinstance(case.get("name"), str) or case.get("status") not in {"passed", "failed"}:
            raise ValueError("Fuse case record malformed")
        name = case["name"]
        if name in names:
            raise ValueError("Fuse case names are not unique")
        names.append(name)
        if case["status"] == "failed": failed.add(name)
        else: passed += 1
    if passed != payload["passed"] or len(failed) != payload["failed"]:
        raise ValueError("Fuse case statuses disagree with counters")
    known = set(map(str, payload["known_unresolved"]))
    if not known <= failed:
        raise ValueError("Fuse known-unresolved set is not a subset of failed cases")
    if unresolved_baseline is not None:
        if known != (failed & unresolved_baseline):
            raise ValueError("Fuse known-unresolved set disagrees with pinned unresolved baseline")
        if failed - unresolved_baseline:
            raise ValueError("Fuse failure is outside pinned unresolved baseline")


def validate_lane_result(result: Any, lane: str | None = None,
                         unresolved_baseline: set[str] | None = None) -> None:
    if not isinstance(result, dict) or result.get("schema_version") != 2:
        raise ValueError("lane result manifest schema is not v2")
    actual_lane = result.get("lane_id")
    if not isinstance(actual_lane, str) or not actual_lane or (lane is not None and actual_lane != lane):
        raise ValueError("lane result manifest lane identity mismatch")
    summary = result.get("summary")
    if not isinstance(summary, dict) or summary.get("status") != "passed" or summary.get("missing_tools"):
        raise ValueError("lane summary is not a complete pass")
    fuse_files = result.get("fuse_manifests")
    if not isinstance(fuse_files, list) or len(fuse_files) != len(EXPECTED_FUSE_MANIFESTS):
        raise ValueError("lane does not bind the complete Fuse manifest family")
    seen: set[str] = set()
    for item in fuse_files:
        if not isinstance(item, dict) or not isinstance(item.get("name"), str) or not isinstance(item.get("payload"), dict):
            raise ValueError("lane Fuse manifest binding is malformed")
        name = item["name"]
        if name in seen or name not in EXPECTED_FUSE_MANIFESTS:
            raise ValueError("lane Fuse manifest set contains duplicate/unknown member")
        seen.add(name)
        selection, total = EXPECTED_FUSE_MANIFESTS[name]
        validate_fuse_manifest(item["payload"], expected_selection=selection, expected_total=total,
                               unresolved_baseline=unresolved_baseline)
    if seen != set(EXPECTED_FUSE_MANIFESTS):
        raise ValueError("lane Fuse manifest family is incomplete")
    sokol = result.get("sokol", {})
    if actual_lane in SOKOL_REQUIRED_LANES:
        if not isinstance(sokol, dict) or sokol.get("required") is not True or sokol.get("status") != "passed":
            raise ValueError("required Sokol host evidence did not pass")


def semantic_result_signature(result: dict[str, Any]) -> str:
    summary = result.get("summary", {})
    fuse_items = result.get("fuse_manifests", [])
    fuse = [{"name": x.get("name"), "selection": x.get("payload", {}).get("selection"),
             "total": x.get("payload", {}).get("total"), "passed": x.get("payload", {}).get("passed"),
             "failed": x.get("payload", {}).get("failed"), "known_unresolved": x.get("payload", {}).get("known_unresolved"),
             "unexpected_failures": x.get("payload", {}).get("unexpected_failures")}
            for x in fuse_items if isinstance(x, dict)]
    normalized = {
        "status": summary.get("status"),
        "missing_tools": sorted(summary.get("missing_tools", [])) if isinstance(summary.get("missing_tools", []), list) else summary.get("missing_tools"),
        "fuse": sorted(fuse, key=lambda x: str(x.get("name"))),
        "visual_supported": result.get("inspection", {}).get("visual_supported"),
        "screenshots_present": bool(result.get("inspection", {}).get("screenshots")),
        "traces_present": bool(result.get("inspection", {}).get("traces")),
        "sokol_status": result.get("sokol", {}).get("status", "not_applicable"),
    }
    return sha256_bytes(canonical_json(normalized).encode())


def expected_lanes(workflow_text: str) -> list[str]:
    lanes = re.findall(r"^\s+- id:\s*([A-Za-z0-9][A-Za-z0-9-]*)\s*$", workflow_text, re.MULTILINE)
    if len(lanes) != 20 or len(set(lanes)) != 20:
        raise ValueError("authoritative workflow must define exactly 20 unique lanes")
    return lanes


def deterministic_tree_index(directory: Path) -> tuple[list[dict[str, Any]], str]:
    records: list[dict[str, Any]] = []
    for path in sorted(directory.rglob("*"), key=lambda p: p.relative_to(directory).as_posix()):
        if path.is_symlink():
            raise ValueError(f"evidence tree contains symlink: {path.relative_to(directory).as_posix()}")
        if not path.is_file():
            continue
        rec = file_digest(path, directory, allow_empty=True)
        records.append(rec)
    return records, sha256_bytes(canonical_json(records).encode())
