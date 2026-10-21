#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.
"""Pin aggregate evidence for one transient private-media playback run."""

import argparse
import datetime
import hashlib
import json
from pathlib import Path
from pathlib import PurePosixPath
import re
import subprocess


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--commit", required=True)
    parser.add_argument("--run-id", required=True, type=int)
    parser.add_argument("--runner", required=True)
    parser.add_argument("--summary", required=True, type=Path)
    parser.add_argument("--proof", required=True, type=Path)
    args = parser.parse_args()
    if not re.fullmatch(r"[0-9a-f]{40}", args.commit):
        raise SystemExit("invalid source commit")
    driver = json.loads(Path("test-drivers/private-extra-media.driver.json")
                        .read_text(encoding="utf-8"))
    fixtures = []
    for fixture in driver["fixtures"]:
        path = PurePosixPath(fixture["path"])
        if not path.parts or path.is_absolute() or ":" in fixture["path"] or "\\" in \
                fixture["path"] or any(part in ("", ".", "..")
                                        for part in path.parts):
            raise SystemExit("invalid source fixture path")
        content = subprocess.check_output(
            ["git", "show", f"{args.commit}:{path.as_posix()}"])
        digest = hashlib.sha256(content).hexdigest()
        if digest != fixture["sha256"]:
            raise SystemExit("private-media regression source fixture mismatch")
        fixtures.append({"path": fixture["path"], "sha256": digest})
    summary = json.loads(args.summary.read_text(encoding="utf-8"))
    required = ("failures", "files", "tapFiles", "tzxFiles", "segments",
                "signalEdges", "motorStops", "ticks")
    if summary.get("status") != "pass" or summary.get("failures") != 0 or \
            summary.get("files") != 1 or summary.get("tapFiles") != 1 or \
            summary.get("tzxFiles") != 0 or \
            any(key not in summary for key in required) or \
            summary.get("segments", 0) == 0:
        raise SystemExit("transient private-media playback did not pass")
    proof = {
        "testId": driver["testId"],
        "status": "pass",
        "commit": args.commit,
        "runId": args.run_id,
        "runner": args.runner,
        "timestamp": datetime.datetime.now(datetime.timezone.utc)
        .isoformat(timespec="seconds"),
        "caseCount": 1,
        "playbackSummary": {key: summary[key] for key in required},
        "fixtures": fixtures,
    }
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    args.proof.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
