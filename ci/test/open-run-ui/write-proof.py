#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

import argparse
import hashlib
import json
import os
import platform
import subprocess
from datetime import datetime, timezone
from pathlib import Path


DRIVER = "test-drivers/open-run-ui.driver.json"


def blob(commit, path):
    return subprocess.check_output(["git", "show", f"{commit}:{path}"])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    commit = os.environ["GITHUB_SHA"]
    driver = json.loads(blob(commit, DRIVER))
    fixtures = []
    for fixture in driver["fixtures"]:
        digest = hashlib.sha256(blob(commit, fixture["path"])).hexdigest()
        if digest != fixture["sha256"]:
            raise SystemExit(f"fixture hash mismatch: {fixture['path']}")
        fixtures.append({"path": fixture["path"], "sha256": digest})

    proof_path = args.output / "open-run-ui.json"
    proof = json.loads(proof_path.read_text(encoding="utf-8"))
    screenshot = args.output / proof["desktopScreenshot"]
    screenshot_hash = hashlib.sha256(screenshot.read_bytes()).hexdigest()
    if screenshot_hash != proof.get("desktopScreenshotSha256"):
        raise SystemExit("desktop screenshot hash mismatch")
    if proof.get("status") != "pass" or proof.get("selectedFixture") != "open-run-smoke.tap":
        raise SystemExit("Open / Run selection proof is incomplete")
    proof.update({
        "testId": driver["testId"],
        "commit": commit,
        "runId": int(os.environ["GITHUB_RUN_ID"]),
        "runner": f"{platform.system()}-{os.environ['RUNNER_ARCH']}",
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "caseCount": 1,
        "fixtures": fixtures + [{
            "path": DRIVER,
            "sha256": hashlib.sha256(blob(commit, DRIVER)).hexdigest(),
        }],
    })
    proof_path.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
