# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.
"""Exercise the production Tape toolbar's Normal and Instant choices."""

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
command = helpers["command"]


def capture(host_window, destination):
    subprocess.run(["import", "-window", host_window, str(destination)],
                   check=True)
    with Image.open(destination) as image:
        top = image.crop((0, 0, image.width, min(image.height, 110)))
        enlarged = top.resize((top.width * 5, top.height * 5),
                              Image.Resampling.NEAREST)
        enlarged.save(destination.with_name(destination.stem + "-ocr.png"))
    result = subprocess.run(
        ["tesseract", str(destination.with_name(destination.stem + "-ocr.png")),
         "stdout", "--psm", "6"], check=True, capture_output=True, text=True)
    return result.stdout.strip()


def expect_mode(host_window, destination, mode):
    deadline = time.monotonic() + 10
    last_text = ""
    while time.monotonic() < deadline:
        last_text = capture(host_window, destination)
        if re.search(rf"\b{mode}\b", last_text, re.IGNORECASE):
            return last_text
        time.sleep(0.5)
    raise RuntimeError(f"Tape mode {mode} not visible: {last_text!r}")


def choose(host_window, row):
    geometry = xdotool("getwindowgeometry", "--shell", host_window)
    width = int(re.search(r"^WIDTH=(\d+)$", geometry, re.MULTILINE).group(1))
    x = int(width * 0.39)
    xdotool("windowfocus", "--sync", host_window)
    xdotool("mousemove", "--sync", "--window", host_window, str(x), "42")
    xdotool("click", "1")
    time.sleep(0.4)
    xdotool("mousemove", "--sync", "--window", host_window,
            str(x), str(68 + row * 24))
    xdotool("click", "1")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--proof", type=Path, required=True)
    args = parser.parse_args()
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="wz-gui-tape-mode-") as temporary:
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
                initial = expect_mode(host_window,
                    args.proof.parent / "normal-initial.png", "Normal")
                choose(host_window, 3)
                instant = expect_mode(host_window,
                    args.proof.parent / "instant-selected.png", "Instant")
                choose(host_window, 2)
                restored = expect_mode(host_window,
                    args.proof.parent / "normal-restored.png", "Normal")
                if "STATE=RUNNING" not in command(sock, "STATUS"):
                    raise RuntimeError("Tape mode selection changed run state")
                images = {}
                for name in ("normal-initial", "instant-selected",
                             "normal-restored"):
                    path = args.proof.parent / f"{name}.png"
                    images[name] = hashlib.sha256(path.read_bytes()).hexdigest()
                args.proof.write_text(json.dumps({
                    "testId": "gui-tape-mode", "status": "pass",
                    "commit": os.environ.get("GITHUB_SHA", ""),
                    "runId": int(os.environ.get("GITHUB_RUN_ID", "0")),
                    "runner": os.environ.get("RUNNER_OS", ""),
                    "initialMode": "Normal", "selectedMode": "Instant",
                    "restoredMode": "Normal", "ocr": [initial, instant, restored],
                    "screenshotsSha256": images,
                }, indent=2) + "\n", encoding="utf-8")
        except Exception:
            if process is not None:
                subprocess.run(["import", "-window", "root",
                    str(args.proof.parent / "desktop-diagnostic.png")],
                    check=False, capture_output=True)
                (args.proof.parent / "application-diagnostic.log").write_text(
                    (root / "application.log").read_text(
                        encoding="utf-8", errors="replace"),
                    encoding="utf-8")
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
