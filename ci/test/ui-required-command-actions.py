#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.
"""Verify required registered Microdrive and Settings action IDs."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import runpy
import subprocess
import tempfile
import time


helpers = runpy.run_path(str(Path(__file__).with_name(
    "live-telnet-acceptance.py")))
connect = helpers["connect"]
Control = helpers["Control"]
gui_helpers = runpy.run_path(str(Path(__file__).with_name(
    "gui-screenshot-save.py")))
xdotool = gui_helpers["xdotool"]
window = gui_helpers["window"]


def capture_text(host_window, destination):
    subprocess.run(["import", "-window", host_window, str(destination)],
                   check=True, capture_output=True)
    return subprocess.run(["tesseract", str(destination), "stdout"],
                          check=True, capture_output=True,
                          text=True).stdout


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--proof", type=Path, required=True)
    args = parser.parse_args()
    if not args.binary.is_file() or not args.rom.is_file():
        raise RuntimeError("required runner input is unavailable")
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    drive_ids = [f"media.microdrive.drive{slot}" for slot in range(1, 9)]
    settings_ids = ["settings.rom_firmware", "settings.peripherals.telnet"]
    assertions = 0
    process = None
    connection = None
    with tempfile.TemporaryDirectory(prefix="wz-required-command-actions-") as temporary:
        environment = os.environ.copy()
        environment["WZSN_ROM_PATH"] = str(args.rom.resolve())
        environment["XDG_CONFIG_HOME"] = str(Path(temporary) / "config")
        try:
            with (Path(temporary) / "application.log").open(
                    "w", encoding="utf-8") as log:
                process = subprocess.Popen([str(args.binary.resolve())],
                    cwd=temporary, env=environment, stdout=log,
                    stderr=subprocess.STDOUT)
                host_window = window(gui_helpers["TITLE"])
                connection = connect(process)
                control = Control(connection)
                tree = control.send("MENU TREE", multiline=True)

                for action_id in drive_ids:
                    prefix = f"ITEM {action_id} "
                    entries = [line for line in tree if line.startswith(prefix)]
                    if len(entries) != 1 or "TYPE=COMMAND" not in entries[0] or \
                            "REMOTE=ALLOWED CLASS=REMOTE_SAFE" not in entries[0]:
                        raise RuntimeError(
                            f"Microdrive action metadata missing: {action_id}")
                    assertions += 1
                    response = control.send(f"DO {action_id}")[-1]
                    expected = (
                        f"OK DO {action_id} MDV {drive_ids.index(action_id) + 1} "
                        "is the default drive")
                    if response != expected:
                        raise RuntimeError(
                            f"Microdrive action dispatch failed: {response!r}")
                    assertions += 1

                for action_id in settings_ids:
                    prefix = f"ITEM {action_id} "
                    entries = [line for line in tree if line.startswith(prefix)]
                    if len(entries) != 1 or "TYPE=COMMAND" not in entries[0] or \
                            "PARENT=settings" not in entries[0] or \
                            "REMOTE=DENIED CLASS=LOCAL_ONLY" not in entries[0]:
                        raise RuntimeError(
                            f"Settings action metadata missing or unsafe: {action_id}")
                    assertions += 1
                    response = control.send(f"DO {action_id}")[-1]
                    if response != f"DENIED {action_id} LOCAL_ONLY":
                        raise RuntimeError(
                            f"local-only Settings action was remotely dispatched: {response!r}")
                    assertions += 1

                geometry = xdotool("getwindowgeometry", "--shell", host_window)
                width = int(re.search(r"^WIDTH=(\d+)$", geometry,
                                      re.MULTILINE).group(1))
                height = int(re.search(r"^HEIGHT=(\d+)$", geometry,
                                       re.MULTILINE).group(1))
                xdotool("mousemove", "--sync", "--window", host_window,
                        str(width // 2), str(height - 14))
                xdotool("click", "1")
                time.sleep(0.5)
                screenshot = args.proof.parent / "telnet-settings.png"
                visible_text = capture_text(host_window, screenshot)
                normalized_text = re.sub(r"[^a-z0-9]+", "",
                                         visible_text.casefold())
                if "telnetkeyboardremotecontrol" not in normalized_text:
                    raise RuntimeError(
                        "status control did not open Telnet Settings: "
                        f"{visible_text!r}")
                assertions += 1

                args.proof.write_text(json.dumps({
                    "testId": "ui-required-command-actions",
                    "status": "pass",
                    "commit": os.environ.get("GITHUB_SHA", ""),
                    "runId": int(os.environ.get("GITHUB_RUN_ID", "0")),
                    "runner": os.environ.get("RUNNER_OS", ""),
                    "architecture": os.environ.get("RUNNER_ARCH", ""),
                    "assertions": assertions,
                    "microdriveActions": drive_ids,
                    "settingsActions": settings_ids,
                    "telnetSettingsScreenshotSha256": hashlib.sha256(
                        screenshot.read_bytes()).hexdigest(),
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
