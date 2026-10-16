#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.
"""Exercise production Tape Insert, Manager, and Eject through the GUI."""

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

helpers = runpy.run_path(str(Path(__file__).with_name("gui-tape-mode.py")))
xdotool = helpers["xdotool"]
window = helpers["window"]
connect = helpers["connect"]
command = helpers["command"]
capture = helpers["capture"]
choose = helpers["choose"]


def expect_text(host_window, destination, pattern):
    deadline = time.monotonic() + 15
    last_text = ""
    while time.monotonic() < deadline:
        last_text = capture(host_window, destination)
        if re.search(pattern, last_text, re.IGNORECASE):
            return last_text
        time.sleep(0.5)
    raise RuntimeError(f"GUI text {pattern!r} unavailable: {last_text!r}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--proof", type=Path, required=True)
    args = parser.parse_args()
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="wz-gui-tape-actions-") as temporary:
        root = Path(temporary)
        tape = root / "synthetic.tap"
        tape.write_bytes(bytes((3, 0, 0, 0, 0)))
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
                host_window = window("Warajevo ZX Spectrum Next")
                sock = connect(process)
                initial = expect_text(host_window,
                    args.proof.parent / "initial.png", r"Empty\s*/\s*Normal")
                choose(host_window, 0)
                dialog = window("Open / Run")
                xdotool("windowfocus", "--sync", dialog)
                xdotool("key", "ctrl+l")
                xdotool("type", "--clearmodifiers", str(tape))
                xdotool("key", "Return")
                time.sleep(0.5)
                xdotool("key", "Return")
                mounted = expect_text(host_window,
                    args.proof.parent / "mounted.png", r"Tape\W+Normal")
                geometry = xdotool("getwindowgeometry", "--shell", host_window)
                match = re.search(r"^WIDTH=(\d+)$", geometry, re.MULTILINE)
                if match is None:
                    raise RuntimeError("host window width unavailable")
                menu_x = int(int(match.group(1)) * 0.39)
                xdotool("windowfocus", "--sync", host_window)
                xdotool("mousemove", "--sync", "--window", host_window,
                        str(menu_x), "42")
                xdotool("click", "1")
                xdotool("mousemove", "--sync", "--window", host_window,
                        str(menu_x), "130")
                xdotool("click", "--repeat", "3", "5")
                time.sleep(0.5)
                xdotool("click", "1")
                manager_path = args.proof.parent / "manager.png"
                capture(host_window, manager_path)
                manager = subprocess.run(["tesseract", str(manager_path),
                    "stdout", "--psm", "11"], check=True, capture_output=True,
                    text=True).stdout.strip()
                if not re.search(r"Tape Manager", manager, re.IGNORECASE):
                    raise RuntimeError(f"Tape Manager unavailable: {manager!r}")
                choose(host_window, 1)
                ejected = expect_text(host_window,
                    args.proof.parent / "ejected.png", r"Empty\s*/\s*Normal")
                if "STATE=RUNNING" not in command(sock, "STATUS"):
                    raise RuntimeError("Tape actions changed run state")
                images = {}
                for name in ("initial", "mounted", "manager", "ejected"):
                    path = args.proof.parent / f"{name}.png"
                    images[name] = hashlib.sha256(path.read_bytes()).hexdigest()
                args.proof.write_text(json.dumps({
                    "testId": "gui-tape-actions", "status": "pass",
                    "commit": os.environ.get("GITHUB_SHA", ""),
                    "runId": int(os.environ.get("GITHUB_RUN_ID", "0")),
                    "runner": os.environ.get("RUNNER_OS", ""),
                    "tapeSha256": hashlib.sha256(tape.read_bytes()).hexdigest(),
                    "ocr": [initial, mounted, manager, ejected],
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
