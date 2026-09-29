#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Pin aggregate private-media playback evidence without exposing media IDs."""

import argparse
import datetime
import hashlib
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--commit", required=True)
    parser.add_argument("--run-id", required=True, type=int)
    parser.add_argument("--runner", required=True)
    parser.add_argument("--summary", required=True, type=Path)
    parser.add_argument("--proof", required=True, type=Path)
    args = parser.parse_args()
    driver = json.loads(Path("test-drivers/private-media-playback.driver.json")
                        .read_text(encoding="utf-8"))
    fixtures = []
    for fixture in driver["fixtures"]:
        path = Path(fixture["path"])
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        if digest != fixture["sha256"]:
            raise SystemExit("private-media playback source fixture mismatch")
        fixtures.append({"path": fixture["path"], "sha256": digest})
    summary = json.loads(args.summary.read_text(encoding="utf-8"))
    keys = ("failures", "files", "tapFiles", "tzxFiles", "segments",
            "signalEdges", "motorStops", "ticks")
    if summary.get("status") != "pass" or summary.get("failures") != 0 or \
            summary.get("files", 0) < driver["requiredMediaFiles"] or \
            any(key not in summary for key in keys):
        raise SystemExit("private-media playback did not pass the pinned contract")
    proof = {
        "testId": driver["testId"],
        "status": "pass",
        "commit": args.commit,
        "runId": args.run_id,
        "runner": args.runner,
        "timestamp": datetime.datetime.now(datetime.timezone.utc)
        .isoformat(timespec="seconds"),
        "caseCount": summary["files"],
        "playbackSummary": {key: summary[key] for key in keys},
        "fixtures": fixtures,
    }
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    args.proof.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
