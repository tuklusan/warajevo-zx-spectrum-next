#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.
"""Inspect canonical menu groups and dispatch fixed choices in the live GUI."""

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
TITLE = helpers["TITLE"]


def capture(host_window, destination):
    subprocess.run(["import", "-window", host_window, str(destination)],
                   check=True, capture_output=True)
    with Image.open(destination) as image:
        enlarged = image.resize((image.width * 2, image.height * 2),
                                Image.Resampling.LANCZOS)
        with tempfile.TemporaryDirectory(prefix="wz-menu-ocr-") as temporary:
            ocr_path = Path(temporary) / "capture.png"
            enlarged.save(ocr_path)
            return subprocess.run(["tesseract", str(ocr_path), "stdout",
                "--psm", "6"], check=True, capture_output=True,
                text=True).stdout.strip()


def root_x(host_window, index):
    geometry = xdotool("getwindowgeometry", "--shell", host_window)
    width_match = re.search(r"^WIDTH=(\d+)$", geometry, re.MULTILINE)
    height_match = re.search(r"^HEIGHT=(\d+)$", geometry, re.MULTILINE)
    if width_match is None or height_match is None:
        raise RuntimeError(f"host window geometry unavailable: {geometry!r}")
    width = int(width_match.group(1))
    return int(width * (index + 0.5) / 7)


def open_root(host_window, index):
    x = root_x(host_window, index)
    xdotool("windowfocus", "--sync", host_window)
    geometry = xdotool("getwindowgeometry", "--shell", host_window)
    height_match = re.search(r"^HEIGHT=(\d+)$", geometry, re.MULTILINE)
    if height_match is None:
        raise RuntimeError(f"host window height unavailable: {geometry!r}")
    xdotool("mousemove", "--sync", "--window", host_window,
            "4", str(int(height_match.group(1)) - 4))
    xdotool("click", "1")
    time.sleep(0.1)
    xdotool("mousemove", "--sync", "--window", host_window, str(x), "10")
    xdotool("click", "1")
    time.sleep(0.35)
    return x


def hover(host_window, x, row):
    xdotool("mousemove", "--sync", "--window", host_window,
            str(x), str(42 + row * 28))
    time.sleep(0.3)


def require_text(text, expected, label):
    missing = [value for value in expected if value.casefold() not in text.casefold()]
    if missing:
        raise RuntimeError(f"{label} omitted {missing}; OCR was {text!r}")


def save_capture(host_window, proof_dir, name):
    destination = proof_dir / f"{name}.png"
    text = capture(host_window, destination)
    return destination, text


def click_model_choice(host_window, control, proof_dir):
    x = open_root(host_window, 1)
    hover(host_window, x, 0)
    image, text = save_capture(host_window, proof_dir, "machine-model")
    require_text(text, ("48K", "128K"), "Machine > Model")
    xdotool("mousemove", "--sync", "--window", host_window,
            str(x + 240), "70")
    time.sleep(0.2)
    xdotool("click", "1")
    time.sleep(0.7)
    status = command(control, "STATUS")
    if "MODEL=128K" not in status:
        raise RuntimeError(f"128K GUI action did not dispatch: {status.strip()}")
    return image, text, status


def click_speed_choice(host_window, control, proof_dir):
    x = open_root(host_window, 1)
    hover(host_window, x, 1)
    image, text = save_capture(host_window, proof_dir, "machine-speed")
    require_text(text, ("25%", "400%", "Unlimited"), "Machine > Emulation Speed")
    xdotool("mousemove", "--sync", "--window", host_window,
            str(x + 240), str(42 + 4 * 28))
    time.sleep(0.2)
    xdotool("click", "1")
    time.sleep(0.5)
    status = command(control, "STATUS")
    if "SPEED=400" not in status:
        raise RuntimeError(f"400% GUI action did not dispatch: {status.strip()}")
    return image, text, status


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--proof", type=Path, required=True)
    args = parser.parse_args()
    if not args.binary.is_file() or not args.rom.is_file():
        raise RuntimeError("required runner input is unavailable")

    args.proof.parent.mkdir(parents=True, exist_ok=True)
    assertions = []
    captures = []
    with tempfile.TemporaryDirectory(prefix="wz-gui-menu-groups-") as temporary:
        root = Path(temporary)
        environment = os.environ.copy()
        environment["WZSN_ROM_PATH"] = str(args.rom.resolve())
        environment["XDG_CONFIG_HOME"] = str(root / "config")
        process = None
        control = None
        try:
            with (root / "application.log").open("w", encoding="utf-8") as log:
                process = subprocess.Popen([str(args.binary.resolve())],
                    cwd=root, env=environment, stdout=log,
                    stderr=subprocess.STDOUT)
                host_window = window(TITLE)
                control = connect(process)

                root_specs = (
                    (0, "file-root", ("Recent",)),
                    (1, "machine-root", ("Model", "Emulation Speed")),
                    (2, "media-root", ("Tape", "Microdrive", "Printer")),
                    (4, "tools-root", ("Debugger", "Compatibility Tools",
                                        "Diagnostics", "Snapshot Inspector")),
                    (5, "settings-root", ("Peripherals", "Display", "Audio")),
                )
                for index, name, required in root_specs:
                    open_root(host_window, index)
                    image, text = save_capture(host_window, args.proof.parent, name)
                    require_text(text, required, name)
                    captures.append(image)
                    assertions.append(name)

                x = open_root(host_window, 2)
                hover(host_window, x, 1)
                image, text = save_capture(host_window, args.proof.parent,
                                           "media-microdrive")
                require_text(text, ("Microdrive Manager", "Mount Microdrive"),
                             "Media > Microdrive")
                captures.append(image)
                assertions.append("media-microdrive-children")

                x = open_root(host_window, 4)
                hover(host_window, x, 1)
                image, text = save_capture(host_window, args.proof.parent,
                                           "tools-compatibility")
                require_text(text, ("Tape Converter", "Snapshot Converter",
                                    "Microdrive Tools"),
                             "Tools > Compatibility Tools")
                captures.append(image)
                assertions.append("tools-compatibility-children")

                x = open_root(host_window, 5)
                hover(host_window, x, 0)
                image, text = save_capture(host_window, args.proof.parent,
                                           "settings-peripherals")
                require_text(text, ("Networking", "Printer",
                                    "Telnet Keyboard"),
                             "Settings > Peripherals")
                captures.append(image)
                assertions.append("settings-peripherals-children")

                model_image, model_text, model_status = click_model_choice(
                    host_window, control, args.proof.parent)
                captures.append(model_image)
                assertions.extend(("machine-model-actions", "model-128k-dispatch"))
                speed_image, speed_text, speed_status = click_speed_choice(
                    host_window, control, args.proof.parent)
                captures.append(speed_image)
                assertions.extend(("machine-speed-actions", "speed-400-dispatch"))

                proof = {
                    "testId": "gui-menu-groups",
                    "status": "pass",
                    "commit": os.environ.get("GITHUB_SHA", ""),
                    "runId": int(os.environ.get("GITHUB_RUN_ID", "0")),
                    "runner": os.environ.get("RUNNER_OS", ""),
                    "architecture": os.environ.get("RUNNER_ARCH", ""),
                    "assertionCount": len(assertions),
                    "assertions": assertions,
                    "machineStatusAfterModelChoice": model_status.strip(),
                    "machineStatusAfterSpeedChoice": speed_status.strip(),
                    "captures": [
                        {"path": image.name,
                         "sha256": hashlib.sha256(image.read_bytes()).hexdigest()}
                        for image in captures
                    ],
                }
                args.proof.write_text(json.dumps(proof, indent=2) + "\n",
                                      encoding="utf-8")
        finally:
            if control is not None:
                control.close()
            if process is not None and process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()


if __name__ == "__main__":
    main()
