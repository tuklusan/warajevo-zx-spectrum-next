#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.
"""Exercise the production View menu actions and status-panel presentation."""

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

from PIL import Image

helpers = runpy.run_path(str(Path(__file__).with_name("gui-screenshot-save.py")))
xdotool = helpers["xdotool"]
window = helpers["window"]
connect = helpers["connect"]
control_helpers = runpy.run_path(str(Path(__file__).with_name(
    "live-telnet-acceptance.py")))
Control = control_helpers["Control"]


def capture(host_window, destination, bottom_only=False):
    subprocess.run(["import", "-window", host_window, str(destination)],
                   check=True, capture_output=True)
    with Image.open(destination) as image:
        if bottom_only:
            image = image.crop((0, max(0, image.height - 75),
                                image.width, image.height))
        enlarged = image.resize((image.width * 2, image.height * 2),
                                Image.Resampling.LANCZOS)
        with tempfile.TemporaryDirectory(prefix="wz-view-ocr-") as temporary:
            ocr_path = Path(temporary) / "capture.png"
            enlarged.save(ocr_path)
            return subprocess.run(["tesseract", str(ocr_path), "stdout",
                "--psm", "11" if bottom_only else "6"], check=True,
                capture_output=True, text=True).stdout.strip()


def open_view_menu(host_window):
    geometry = xdotool("getwindowgeometry", "--shell", host_window)
    width_match = re.search(r"^WIDTH=(\d+)$", geometry, re.MULTILINE)
    if width_match is None:
        raise RuntimeError("host window width unavailable")
    x = int(int(width_match.group(1)) * 0.50)
    xdotool("windowfocus", "--sync", host_window)
    xdotool("mousemove", "--sync", "--window", host_window, str(x), "10")
    xdotool("click", "1")
    time.sleep(0.4)
    return x


def view_choice(host_window, row):
    x = open_view_menu(host_window)
    xdotool("mousemove", "--sync", "--window", host_window,
            str(x), str(42 + row * 28))
    xdotool("click", "1")
    time.sleep(0.7)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--proof", type=Path, required=True)
    args = parser.parse_args()
    if not args.binary.is_file() or not args.rom.is_file():
        raise RuntimeError("required runner input is unavailable")
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="wz-view-menu-actions-") as temporary:
        root = Path(temporary)
        environment = os.environ.copy()
        environment["WZSN_ROM_PATH"] = str(args.rom.resolve())
        environment["XDG_CONFIG_HOME"] = str(root / "config")
        process = None
        sock = None
        try:
            with (root / "application.log").open("w", encoding="utf-8") as log:
                process = subprocess.Popen([str(args.binary.resolve())],
                    cwd=root, env=environment, stdout=log,
                    stderr=subprocess.STDOUT)
                host_window = window(helpers["TITLE"])
                sock = connect(process)
                control = Control(sock)
                tree = control.send("MENU TREE", multiline=True)
                ids = ("view.machine_media_status", "view.display_settings")
                assertions = 0
                for action_id in ids:
                    entries = [line for line in tree
                               if line.startswith(f"ITEM {action_id} ")]
                    if len(entries) != 1 or "CLASS=LOCAL_ONLY" not in entries[0]:
                        raise RuntimeError(
                            f"View action metadata missing or unsafe: {action_id}")
                    assertions += 1
                    denial = control.send(f"DO {action_id}")
                    if denial[-1] != f"DENIED {action_id} LOCAL_ONLY":
                        raise RuntimeError(
                            f"remote View action was not denied: {action_id}: {denial[-1]}")
                    assertions += 1

                open_view_menu(host_window)
                view_menu_text = capture(host_window,
                    args.proof.parent / "view-menu.png")
                if "Fullscreen" not in view_menu_text or \
                        "Machine / Media Status" not in view_menu_text or \
                        "Display Settings" not in view_menu_text:
                    raise RuntimeError(
                        f"canonical View menu unavailable: {view_menu_text!r}")
                assertions += 1
                xdotool("key", "Escape")
                initial_status = capture(host_window,
                    args.proof.parent / "status-visible.png", True)
                if "Keyboard" not in initial_status:
                    raise RuntimeError(
                        f"initial status panel is not visible: {initial_status!r}")
                assertions += 1
                view_choice(host_window, 1)
                hidden_status = capture(host_window,
                    args.proof.parent / "status-hidden.png", True)
                if "Keyboard" in hidden_status:
                    raise RuntimeError(
                        f"status panel remained visible after toggle: {hidden_status!r}")
                assertions += 1
                view_choice(host_window, 1)
                restored_status = capture(host_window,
                    args.proof.parent / "status-restored.png", True)
                if "Keyboard" not in restored_status:
                    raise RuntimeError(
                        f"status panel did not restore: {restored_status!r}")
                assertions += 1
                view_choice(host_window, 2)
                display_settings = capture(host_window,
                    args.proof.parent / "display-settings.png")
                if not re.search(r"Display Settings", display_settings, re.I):
                    raise RuntimeError(
                        f"display settings panel did not open: {display_settings!r}")
                assertions += 1
                machine_status = control.send("STATUS")[-1]
                if "STATE=RUNNING" not in machine_status:
                    raise RuntimeError("View actions changed machine run state")
                assertions += 1
                images = {}
                for name in ("view-menu", "status-visible", "status-hidden",
                             "status-restored", "display-settings"):
                    image_path = args.proof.parent / f"{name}.png"
                    images[name] = hashlib.sha256(
                        image_path.read_bytes()).hexdigest()
                args.proof.write_text(json.dumps({
                    "testId": "ui-view-menu-actions", "status": "pass",
                    "commit": os.environ.get("GITHUB_SHA", ""),
                    "runId": int(os.environ.get("GITHUB_RUN_ID", "0")),
                    "runner": os.environ.get("RUNNER_OS", ""),
                    "architecture": os.environ.get("RUNNER_ARCH", ""),
                    "assertions": assertions,
                    "remoteDeniedIds": list(ids),
                    "statusPanel": ["visible", "hidden", "visible"],
                    "displaySettingsOpened": True,
                    "screenshotsSha256": images,
                }, indent=2) + "\n", encoding="utf-8")
        except Exception:
            if process is not None:
                subprocess.run(["import", "-window", "root",
                    str(args.proof.parent / "desktop-diagnostic.png")],
                    check=False, capture_output=True)
                (args.proof.parent / "application-diagnostic.log").write_text(
                    (root / "application.log").read_text(
                        encoding="utf-8", errors="replace"), encoding="utf-8")
            raise
        finally:
            if sock is not None:
                sock.close()
            if process is not None:
                process.terminate()
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=10)


if __name__ == "__main__":
    main()
