#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Write source-pinned hosted evidence for audio-independent frame advance."""

import argparse
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path


FIXTURES = (
    ".github/workflows/host-machine-frame-regression.yml",
    "ci/test/host-frame-audio/frame-audio-regression.c",
    "ci/test/host-frame-audio/write-proof.py",
    "src/app/wz_host_machine_frame.c",
    "src/app/wz_host_machine_frame.h",
    "src/app/wz_host_audio_policy.c",
    "src/app/wz_host_audio_policy.h",
    "src/app/wz_sokol_main.c",
    "src/app/wz_speed_policy.c",
    "src/app/wz_speed_policy.h",
    "src/core/audio/wz_ay.c",
    "src/core/audio/wz_ay.h",
    "src/core/wz_machine.h",
    "src/core/wz_runner.c",
    "src/cmake/CMakeLists.txt",
    "test-drivers/host-machine-frame.driver.json",
)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--commit", required=True)
    parser.add_argument("--run-id", required=True, type=int)
    parser.add_argument("--runner", required=True)
    parser.add_argument("--proof", required=True, type=Path)
    args = parser.parse_args()

    fixtures = []
    for name in FIXTURES:
        fixtures.append(
            {"path": name, "sha256": hashlib.sha256(Path(name).read_bytes()).hexdigest()}
        )
    proof = {
        "testId": "host-machine-frame-audio-continuity",
        "status": "pass",
        "commit": args.commit,
        "runId": args.run_id,
        "runner": args.runner,
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "caseCount": 56,
        "fixtures": fixtures,
    }
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    args.proof.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
