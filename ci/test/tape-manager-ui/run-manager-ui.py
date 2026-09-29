#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

import csv
import hashlib
import json
import os
import pathlib
import subprocess
import sys
import time


def run(command, environment=None):
    result = subprocess.run(command, check=False, capture_output=True,
                            text=True, env=environment)
    if result.returncode != 0:
        raise RuntimeError(f"command failed ({result.returncode}): {command}: "
                           f"{result.stderr.strip()}")
    return result.stdout.strip()


def focus_application(environment):
    deadline = time.monotonic() + 30
    while time.monotonic() < deadline:
        result = subprocess.run(
            ["xdotool", "search", "--onlyvisible", "--name",
             "Warajevo ZX Spectrum Next"], check=False,
            capture_output=True, text=True, env=environment)
        if result.returncode == 0:
            for window in result.stdout.splitlines():
                focused = subprocess.run(
                    ["xdotool", "windowfocus", "--sync", window],
                    check=False, capture_output=True, text=True,
                    env=environment)
                if focused.returncode == 0:
                    return window
        time.sleep(0.1)
    raise RuntimeError("application window not found or could not be focused")


def capture_words(window, destination, environment):
    run(["import", "-window", window, str(destination)], environment)
    output = run(["tesseract", str(destination), "stdout", "tsv",
                  "--psm", "11"], environment)
    return list(csv.DictReader(output.splitlines(), delimiter="\t"))


def click_word(window, words, expected, environment):
    match = next((word for word in words
                  if word.get("level") == "5" and
                  expected.casefold() in word.get("text", "").casefold()), None)
    if match is None:
        observed = " ".join(word.get("text", "") for word in words
                            if word.get("level") == "5")
        raise RuntimeError(f"visible text not found: {expected}; OCR: {observed}")
    x = int(match["left"]) + int(match["width"]) // 2
    y = int(match["top"]) + int(match["height"]) // 2
    run(["xdotool", "mousemove", "--sync", "--window", window, str(x),
         str(y), "click", "1"], environment)


def main():
    binary = pathlib.Path(sys.argv[1]).resolve()
    root = pathlib.Path(sys.argv[2]).resolve()
    output = pathlib.Path(sys.argv[3]).resolve()
    output.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    environment.update({
        "GDK_BACKEND": "x11",
        "GTK_USE_PORTAL": "0",
        "LIBGL_ALWAYS_SOFTWARE": "1",
        "WZSN_ROM_PATH": str(root / "roms" / "48.rom"),
        "WZSN_TAPE_PATH": str(root / "test-media" / "DIZZY4K.TAP"),
    })
    log_path = output / "application.log"
    with log_path.open("w", encoding="utf-8") as log:
        process = subprocess.Popen([str(binary)], env=environment,
                                   stdout=log, stderr=subprocess.STDOUT)
    try:
        window = focus_application(environment)
        time.sleep(2.0)
        initial = output / "initial.png"
        initial_words = capture_words(window, initial, environment)
        click_word(window, initial_words, "Media", environment)
        time.sleep(0.4)
        menu = output / "media-menu.png"
        menu_words = capture_words(window, menu, environment)
        click_word(window, menu_words, "Manager", environment)
        time.sleep(0.8)
        opened = output / "tape-manager.png"
        manager_words = capture_words(window, opened, environment)
        visible = " ".join(word.get("text", "") for word in manager_words
                           if word.get("level") == "5").casefold()
        required = ("tape", "manager", "format", "tap", "transport",
                    "signal", "segment", "selected", "blocks")
        missing = [word for word in required if word not in visible]
        if missing:
            raise RuntimeError("Tape Manager content missing: " + ", ".join(missing))
        result = {
            "result": "pass",
            "window": window,
            "visibleText": visible,
            "screenshot": opened.name,
            "screenshotSha256": hashlib.sha256(opened.read_bytes()).hexdigest(),
        }
        (output / "gui-result.json").write_text(
            json.dumps(result, indent=2) + "\n", encoding="utf-8")
        print(json.dumps(result))
    finally:
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=5)
        if process.poll() is None:
            raise RuntimeError("application process did not terminate")


if __name__ == "__main__":
    main()
