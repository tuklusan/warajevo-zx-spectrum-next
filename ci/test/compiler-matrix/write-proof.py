# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Create compiler-specific evidence after the hosted regression succeeds."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from datetime import datetime, timezone
from pathlib import Path
import re
import subprocess
import sys


REPO = Path(__file__).resolve().parents[3]


def digest_tracked_file(path: Path, commit: str) -> str:
    relative_path = path.relative_to(REPO).as_posix()
    contents = subprocess.check_output(
        ["git", "show", f"{commit}:{relative_path}"], cwd=REPO
    )
    return hashlib.sha256(contents).hexdigest()


def read_cache(path: Path) -> dict[str, str]:
    values = {}
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        if not line or line.startswith(("#", "//")):
            continue
        key_and_type, separator, value = line.partition("=")
        if not separator:
            continue
        key = key_and_type.partition(":")[0]
        if key:
            values[key] = value
    return values


def read_compiler_metadata(cache_path: Path) -> dict[str, str]:
    metadata_files = sorted(
        (cache_path.parent / "CMakeFiles").glob("*/CMakeCCompiler.cmake")
    )
    values = {}
    for name in ("CMAKE_C_COMPILER_ID", "CMAKE_C_COMPILER_VERSION"):
        pattern = rf'^\s*set\({name}\s+"([^"]+)"\s*\)'
        for metadata_path in metadata_files:
            match = re.search(
                pattern,
                metadata_path.read_text(encoding="utf-8", errors="replace"),
                flags=re.MULTILINE,
            )
            if match:
                values[name] = match.group(1)
                break
    return values


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cache", required=True, type=Path)
    parser.add_argument("--expected-id", required=True)
    parser.add_argument("--requested", required=True)
    parser.add_argument("--proof-out", required=True, type=Path)
    arguments = parser.parse_args()

    commit = os.environ.get("GITHUB_SHA") or subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=REPO, text=True
    ).strip()
    if not re.fullmatch(r"[0-9a-f]{40}", commit):
        print("Invalid pinned commit identifier", file=sys.stderr)
        return 1
    values = read_cache(arguments.cache)
    values.update(read_compiler_metadata(arguments.cache))
    compiler_id = values.get("CMAKE_C_COMPILER_ID", "")
    compiler_version = values.get("CMAKE_C_COMPILER_VERSION", "")
    compiler_path = values.get("CMAKE_C_COMPILER", "")
    generator = values.get("CMAKE_GENERATOR", "")
    errors = []
    if compiler_id != arguments.expected_id:
        errors.append(f"expected compiler ID {arguments.expected_id}, found {compiler_id or 'missing'}")
    if not compiler_version or not compiler_path or not generator:
        errors.append("CMake cache is missing compiler version, path, or generator")

    fixture_paths = (
        REPO / "ci/test/core-regression/canonical-fingerprints.h",
        REPO / "tests/canonical-core-regression.c",
        REPO / "ci/test/core-regression/CMakeLists.txt",
        REPO / "test-drivers/canonical-core-regression.driver.json",
    )
    fixtures = [
        {
            "path": path.relative_to(REPO).as_posix(),
            "sha256": digest_tracked_file(path, commit),
        }
        for path in fixture_paths
    ]
    proof = {
        "testId": "canonical-core-regression",
        "status": "pass" if not errors else "fail",
        "commit": commit,
        "runner": f"{os.environ.get('RUNNER_OS', 'unknown')}/{os.environ.get('RUNNER_ARCH', 'unknown')}",
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "runId": os.environ.get("GITHUB_RUN_ID", "unknown"),
        "compiler": {
            "requested": arguments.requested,
            "id": compiler_id,
            "version": compiler_version,
            "path": compiler_path,
            "generator": generator,
        },
        "fixtures": fixtures,
        "errors": errors,
    }
    arguments.proof_out.parent.mkdir(parents=True, exist_ok=True)
    arguments.proof_out.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    for error in errors:
        print(f"FAIL {error}", file=sys.stderr)
    if errors:
        return 1
    print(f"PASS {compiler_id} {compiler_version} ({arguments.requested})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
