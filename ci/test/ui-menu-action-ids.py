#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.
"""Verify the production fixed-choice menu IDs and their shared handlers."""

import argparse
import json
import os
from pathlib import Path
import runpy
import subprocess
import tempfile


helpers = runpy.run_path(str(Path(__file__).with_name(
    "live-telnet-acceptance.py")))
connect = helpers["connect"]
Control = helpers["Control"]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--proof", type=Path, required=True)
    args = parser.parse_args()
    if not args.binary.is_file() or not args.rom.is_file():
        raise RuntimeError("required runner input is unavailable")
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    assertions = 0
    process = None
    connection = None
    with tempfile.TemporaryDirectory(prefix="wz-menu-action-ids-") as temporary:
        environment = os.environ.copy()
        environment["WZSN_ROM_PATH"] = str(args.rom.resolve())
        environment["XDG_CONFIG_HOME"] = str(Path(temporary) / "config")
        try:
            with (Path(temporary) / "application.log").open(
                    "w", encoding="utf-8") as log:
                process = subprocess.Popen([str(args.binary.resolve())],
                    cwd=temporary, env=environment, stdout=log,
                    stderr=subprocess.STDOUT)
                connection = connect(process)
                control = Control(connection)
                tree = control.send("MENU TREE", multiline=True)
                ids = [
                    "machine.model.48k", "machine.model.128k",
                    "machine.speed.25", "machine.speed.50",
                    "machine.speed.100", "machine.speed.200",
                    "machine.speed.400", "machine.speed.800",
                    "machine.speed.unlimited",
                ]
                for action_id in ids:
                    prefix = f"ITEM {action_id} "
                    records = [line for line in tree if line.startswith(prefix)]
                    if len(records) != 1 or "TYPE=COMMAND" not in records[0] or \
                            "REMOTE=ALLOWED CLASS=REMOTE_SAFE" not in records[0]:
                        raise RuntimeError(
                            f"fixed menu action missing or unsafe: {action_id}")
                    assertions += 1

                for action_id, model in (("machine.model.128k", "128K"),
                                         ("machine.model.48k", "48K")):
                    response = control.send(f"DO {action_id}")
                    expected_result = f"OK DO {action_id} {model.lower()}"
                    if response[-1] != expected_result:
                        raise RuntimeError(
                            f"fixed model action failed: {action_id}: {response[-1]}")
                    status = control.send("STATUS")[-1]
                    if f"MODEL={model}" not in status:
                        raise RuntimeError(f"model action did not select {model}")
                    assertions += 2

                expected_speeds = (
                    ("25", "25%"), ("50", "50%"), ("100", "100%"),
                    ("200", "200%"), ("400", "400%"), ("800", "800%"),
                    ("UNLIMITED", "Unlimited"),
                )
                for action_id, (speed, result_label) in zip(ids[2:], expected_speeds):
                    response = control.send(f"DO {action_id}")
                    expected_result = f"OK DO {action_id} {result_label}"
                    if response[-1] != expected_result:
                        raise RuntimeError(
                            f"fixed speed action failed: {action_id}: {response[-1]}")
                    status = control.send("STATUS")[-1]
                    if f"SPEED={speed}" not in status:
                        raise RuntimeError(f"speed action did not select {speed}")
                    assertions += 2

                args.proof.write_text(json.dumps({
                    "testId": "ui-menu-action-ids", "status": "pass",
                    "commit": os.environ.get("GITHUB_SHA", ""),
                    "runId": int(os.environ.get("GITHUB_RUN_ID", "0")),
                    "runner": os.environ.get("RUNNER_OS", ""),
                    "architecture": os.environ.get("RUNNER_ARCH", ""),
                    "assertions": assertions,
                    "fixedActionIds": ids,
                }, indent=2) + "\n", encoding="utf-8")
        finally:
            if connection is not None:
                connection.close()
            if process is not None and process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()


if __name__ == "__main__":
    main()
