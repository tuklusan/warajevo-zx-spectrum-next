#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Write source-pinned evidence for networking mode reconfiguration."""

import argparse
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path

FIXTURES = (
    ".github/workflows/ui-networking-reconfiguration.yml",
    "ci/test/ui-networking-reconfiguration/CMakeLists.txt",
    "ci/test/ui-networking-reconfiguration/ui-networking-reconfiguration-contract.c",
    "ci/test/ui-networking-reconfiguration/write-proof.py",
    "test-drivers/ui-networking-reconfiguration.driver.json",
    "src/app/wz_command_registry.c",
    "src/app/wz_command_registry.h",
    "src/app/wz_networking_commands.c",
    "src/app/wz_networking_commands.h",
    "src/app/wz_ui_layout.c",
    "src/app/wz_ui_layout.h",
    "src/core/wz_machine.c",
    "src/core/wz_machine.h",
    "src/core/wz_kempston.h",
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
        "testId": "ui-networking-reconfiguration",
        "status": "pass",
        "commit": args.commit,
        "runId": args.run_id,
        "runner": args.runner,
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "caseCount": 41,
        "fixtures": fixtures,
    }
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    args.proof.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()