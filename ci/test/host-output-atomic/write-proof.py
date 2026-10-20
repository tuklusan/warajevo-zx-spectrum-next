# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

import argparse
import hashlib
import json
import subprocess
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath

EXPECTED = {"windows-x64", "windows-arm64", "macos-intel", "macos-arm64"}
MARKERS = (
    "PASS atomic shared-path writes:",
    "PASS exclusive destination claim:",
    "PASS no temporary output remains after concurrent writes",
)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--project-commit", required=True)
    parser.add_argument("--run-id", required=True, type=int)
    parser.add_argument("--artifact-root", required=True, type=Path)
    parser.add_argument("--proof-out", required=True, type=Path)
    args = parser.parse_args()
    root = Path.cwd()
    driver = json.loads((root / "test-drivers/host-output-atomic-contract.driver.json")
                        .read_text(encoding="utf-8"))
    runners = []
    for runner_id in sorted(EXPECTED):
        folder = args.artifact_root / runner_id
        record = json.loads((folder / "runner.json").read_text(encoding="utf-8"))
        output = (folder / "run.log").read_text(encoding="utf-8")
        if record.get("label") != runner_id:
            raise SystemExit(f"runner identity mismatch: {runner_id}")
        if any(marker not in output for marker in MARKERS):
            raise SystemExit(f"required output-contract cases missing: {runner_id}")
        runners.append({**record, "caseCount": len(MARKERS),
                        "observedOutput": output.strip()})

    fixtures = []
    for item in driver["fixtures"]:
        relative = PurePosixPath(item["path"])
        if relative.is_absolute() or ".." in relative.parts:
            raise SystemExit(f"fixture path is unsafe: {item['path']}")
        source = subprocess.check_output(
            ["git", "show", f"{args.project_commit}:{relative.as_posix()}"],
            cwd=root, stderr=subprocess.PIPE)
        digest = hashlib.sha256(source).hexdigest()
        if item["sha256"] and digest != item["sha256"]:
            raise SystemExit(f"fixture hash mismatch: {item['path']}")
        fixtures.append({"path": item["path"], "sha256": digest})

    proof = {
        "testId": driver["testId"],
        "status": "pass",
        "commit": args.project_commit,
        "runner": "Windows x64/ARM64 and macOS Intel/ARM64 hosted matrix",
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "runId": args.run_id,
        "caseCount": len(MARKERS),
        "processesPerContentionGroup": 8,
        "runners": runners,
        "productionCallSites": [
            "src/app/wz_sokol_main.c: snapshot, printer export, and GUI screenshot outputs",
            "src/app/wz_telnet_keyboard_command.c: Telnet screenshot output",
        ],
        "fixtures": fixtures,
    }
    args.proof_out.parent.mkdir(parents=True, exist_ok=True)
    args.proof_out.write_text(json.dumps(proof, indent=2) + "\n",
                              encoding="utf-8")


if __name__ == "__main__":
    main()
