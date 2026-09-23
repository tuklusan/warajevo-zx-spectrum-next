# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Write evidence for the pinned upstream Z80 instruction suite."""

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
UPSTREAM_REVISION = "c94a5e611bb0b795d62e904a0f5af4f59f2a4eee"
REQUIRED_CASES = 1356
EVENT_COMPARISON = (
    "Compare every MR/MW/PR/PW transfer and timestamp. MC/PC markers are "
    "excluded because upstream coretest emits synthetic contention-helper "
    "traces whose memory map differs from the 48K PAL profile; this suite "
    "does not claim profile contention conformance."
)
UPSTREAM_FILES = (
    "z80/tests/tests.in",
    "z80/tests/tests.expected",
)
PROJECT_FILES = (
    "tests/fuse-z80-conformance.c",
    "src/cmake/CMakeLists.txt",
    "ci/test/fuse-z80/CMakeLists.txt",
    "ci/test/fuse-z80/driver.json",
    ".github/workflows/fuse-z80-regression.yml",
    "ci/test/fuse-z80/write-proof.py",
    "tests/fuse-z80-conformance.md",
    "test-drivers/fuse-z80-conformance.driver.json",
)
EXPECTED_UPSTREAM_HASHES = {
    "z80/tests/tests.in": "9f36e866f22e72ff1f8bf2100bf70ffbf58edd97b453500aab60acf1f403ebbb",
    "z80/tests/tests.expected": "15a6946f4addcf97e137b5bdd1d5fdb08124ff91f1b169f36a8bf4afe4bab6e4",
}


def git_blob(repository: Path, revision: str, relative_path: str) -> bytes:
    return subprocess.check_output(
        ["git", "-C", str(repository), "show", f"{revision}:{relative_path}"]
    )


def sha256(contents: bytes) -> str:
    return hashlib.sha256(contents).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--project-commit", required=True)
    parser.add_argument("--upstream-commit", required=True)
    parser.add_argument("--run-log", required=True, type=Path)
    parser.add_argument("--proof-out", required=True, type=Path)
    arguments = parser.parse_args()

    if not re.fullmatch(r"[0-9a-f]{40}", arguments.project_commit):
        print("Invalid pinned project commit identifier", file=sys.stderr)
        return 1
    if arguments.upstream_commit != UPSTREAM_REVISION:
        print("Fetched upstream revision does not match the pinned revision", file=sys.stderr)
        return 1

    match = re.search(r"^PASS Fuse Z80 cases: (\d+)\s*$",
                      arguments.run_log.read_text(encoding="utf-8"),
                      flags=re.MULTILINE)
    if match is None or int(match.group(1)) != REQUIRED_CASES:
        print(f"Expected all {REQUIRED_CASES} Fuse cases to pass", file=sys.stderr)
        return 1

    fixtures = []
    for relative_path in PROJECT_FILES:
        contents = git_blob(REPO, arguments.project_commit, relative_path)
        fixtures.append({"path": relative_path, "sha256": sha256(contents)})

    upstream = Path("dist/fuse-upstream")
    for relative_path in UPSTREAM_FILES:
        actual_hash = sha256(git_blob(upstream, arguments.upstream_commit,
                                      relative_path))
        if actual_hash != EXPECTED_UPSTREAM_HASHES[relative_path]:
            print(f"Pinned upstream fixture hash mismatch: {relative_path}",
                  file=sys.stderr)
            return 1
        fixtures.append({
            "path": f"fuse-emulator/fuse@{UPSTREAM_REVISION}:{relative_path}",
            "sha256": actual_hash,
        })

    proof = {
        "testId": "fuse-z80-conformance",
        "status": "pass",
        "commit": arguments.project_commit,
        "runner": f"{os.environ.get('RUNNER_OS', 'unknown')}/{os.environ.get('RUNNER_ARCH', 'unknown')}",
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "runId": os.environ.get("GITHUB_RUN_ID", "unknown"),
        "upstreamRevision": UPSTREAM_REVISION,
        "caseCount": REQUIRED_CASES,
        "eventComparison": EVENT_COMPARISON,
        "fixtures": fixtures,
    }
    arguments.proof_out.parent.mkdir(parents=True, exist_ok=True)
    arguments.proof_out.write_text(json.dumps(proof, indent=2) + "\n",
                                   encoding="utf-8")
    print(f"PASS pinned conformance proof for {REQUIRED_CASES} cases")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
