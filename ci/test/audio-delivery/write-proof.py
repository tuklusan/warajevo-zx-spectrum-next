#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Write the pinned hosted Sokol audio backpressure regression proof."""

import argparse
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path


FIXTURES = (
    ".github/workflows/audio-delivery-regression.yml",
    "ci/test/audio-delivery/sokol-audio-delivery-regression.c",
    "ci/test/audio-delivery/sokol_audio_stub/sokol_audio.h",
    "ci/test/audio-delivery/write-proof.py",
    "test-drivers/sokol-audio-delivery.driver.json",
    "src/app/wz_host_audio_push.c",
    "src/app/wz_host_audio_push.h",
    "src/app/wz_host_audio_policy.c",
    "src/app/wz_host_audio_policy.h",
    "src/app/wz_host_config.c",
    "src/app/wz_command_registry.c",
    "src/app/wz_host_thread.c",
    "src/app/wz_ui_layout.c",
    "src/app/wz_ui_layout.h",
    "src/app/wz_sokol_audio.c",
    "src/app/wz_sokol_audio.h",
    "src/app/wz_sokol_main.c",
    "src/app/wz_telnet_status.c",
    "src/app/wz_telnet_status.h",
    "src/app/wz_speed_policy.c",
    "src/app/wz_speed_policy.h",
    "src/cmake/CMakeLists.txt",
    "docs/design/02-warajevo-zx-spectrum-next-ui-architecture.md",
)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--commit", required=True)
    parser.add_argument("--run-id", required=True, type=int)
    parser.add_argument("--runner", required=True)
    parser.add_argument("--proof", required=True, type=Path)
    args = parser.parse_args()

    fixtures = [
        {"path": name, "sha256": hashlib.sha256(Path(name).read_bytes()).hexdigest()}
        for name in FIXTURES
    ]
    proof = {
        "testId": "sokol-audio-delivery",
        "status": "pass",
        "commit": args.commit,
        "runId": args.run_id,
        "runner": args.runner,
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "caseCount": 15,
        "fixtures": fixtures,
    }
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    args.proof.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
