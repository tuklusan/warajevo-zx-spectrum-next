#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Create an aggregate proof without exporting media paths or bytes."""

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
    driver = json.loads(Path("test-drivers/media-corpus-ingestion.driver.json").read_text())
    fixtures = []
    for fixture in driver["fixtures"]:
        path = Path(fixture["path"])
        digest_state = hashlib.sha256()
        with path.open("rb") as source:
            for chunk in iter(lambda: source.read(65536), b""):
                digest_state.update(chunk)
        digest = digest_state.hexdigest()
        if digest != fixture["sha256"]:
            raise SystemExit("media-corpus contract fixture hash mismatch")
        fixtures.append({"path": fixture["path"], "sha256": digest})
    summary = json.loads(args.summary.read_text())
    required = ("files", "tapFiles", "tzxFiles", "supported", "unsupported",
                "unsupportedTzxBlocks", "malformed")
    if summary.get("status") != "pass" or any(key not in summary for key in required):
        raise SystemExit("media-corpus contract did not produce a complete aggregate")
    if summary["files"] < driver["requiredCases"]:
        raise SystemExit("media-corpus contract found fewer tapes than the pinned inventory minimum")
    proof = {
        "testId": driver["testId"],
        "status": "pass",
        "commit": args.commit,
        "runId": args.run_id,
        "runner": args.runner,
        "timestamp": datetime.datetime.now(datetime.timezone.utc).isoformat(timespec="seconds"),
        "caseCount": summary["files"],
        "mediaSummary": {key: summary[key] for key in required},
        "fixtures": fixtures,
    }
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    args.proof.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
