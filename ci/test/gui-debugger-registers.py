#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.
"""Exercise the production debugger register editor and undo controls."""

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import time


TITLE = "Warajevo ZX Spectrum Next"


def run(*args):
    return subprocess.run(args, check=True, capture_output=True,
                          text=True).stdout.strip()


def find_window(timeout=20):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        found = subprocess.run(["xdotool", "search", "--onlyvisible", "--name",
                                TITLE], capture_output=True, text=True)
        if found.returncode == 0 and found.stdout.strip():
            return found.stdout.splitlines()[-1]
        time.sleep(0.2)
    raise RuntimeError("production application window was not shown")


def screenshot_text(root):
    image = root / "screen.png"
    run("scrot", "--silent", str(image))
    output = run("tesseract", str(image), "stdout")
    return output


def find_text(root, label, timeout=5):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        image = root / "screen.png"
        run("scrot", "--silent", str(image))
        data = subprocess.run(["tesseract", str(image), "stdout", "tsv"],
                              check=True, capture_output=True, text=True).stdout
        rows = [line.split("\t") for line in data.splitlines()]
        matches = [row for row in rows if len(row) >= 12 and
                   row[11].strip().casefold() == label.casefold() and
                   row[10].strip().lstrip("-").isdigit() and int(row[10]) > 35]
        if matches:
            row = matches[-1]
            return (int(row[6]), int(row[7]), int(row[8]), int(row[9]))
        time.sleep(0.25)
    raise RuntimeError(f"visible UI label not found: {label}")


def click_text(root, label, timeout=5):
    x0, y0, width, height = find_text(root, label, timeout)
    x = x0 + width // 2
    y = y0 + height // 2
    run("xdotool", "mousemove", "--sync", str(x), str(y), "click", "1")
    time.sleep(0.25)


def fixture_records(driver_path, commit):
    driver = json.loads(driver_path.read_text(encoding="utf-8"))
    records = []
    for item in driver["fixtures"]:
        content = subprocess.check_output(["git", "show", f"{commit}:{item['path']}"])
        actual = hashlib.sha256(content).hexdigest()
        if actual != item["sha256"]:
            raise RuntimeError(f"fixture hash mismatch: {item['path']}")
        records.append({"path": item["path"], "sha256": actual})
    return records


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--proof", type=Path, required=True)
    parser.add_argument("--driver", type=Path, required=True)
    parser.add_argument("--capture-dir", type=Path, required=True)
    args = parser.parse_args()
    if not args.binary.is_file() or not args.rom.is_file():
        raise RuntimeError("production application or certified ROM is unavailable")
    commit = os.environ.get("GITHUB_SHA", "")
    if not re.fullmatch(r"[0-9a-f]{40}", commit):
        raise RuntimeError("workflow commit SHA is unavailable")
    fixtures = fixture_records(args.driver, commit)
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    args.capture_dir.mkdir(parents=True, exist_ok=True)
    assertions = []
    with tempfile.TemporaryDirectory(prefix="wz-debugger-registers-") as directory:
        root = Path(directory)
        environment = os.environ.copy()
        environment["WZSN_ROM_PATH"] = str(args.rom.resolve())
        environment["XDG_CONFIG_HOME"] = str(root / "config")
        process = None
        try:
            with (root / "application.log").open("w", encoding="utf-8") as log:
                process = subprocess.Popen([str(args.binary.resolve())], cwd=root,
                    env=environment, stdout=log, stderr=subprocess.STDOUT)
                window = find_window()
                run("xdotool", "windowfocus", "--sync", window)
                run("xdotool", "key", "--clearmodifiers", "F12")
                time.sleep(0.5)
                screen = screenshot_text(root)
                if "Register editor" not in screen:
                    raise RuntimeError("production debugger register editor is not visible")
                assertions.append("debugger-register-controls-visible")
                (args.capture_dir / "debugger-open.png").write_bytes(
                    (root / "screen.png").read_bytes())

                click_text(root, "Pause")
                before = screenshot_text(root)
                match = re.search(r"\bAF\s+([0-9A-F]{4})", before)
                if match is None:
                    raise RuntimeError("main AF value is not readable in debugger")
                original_af = match.group(1)
                assertions.append("machine-paused-in-debugger")

                # Register editor is directly below its label; value input is
                # the right-hand field, while the left combo defaults to A.
                geometry = run("xdotool", "getwindowgeometry", "--shell", window)
                x0 = int(re.search(r"^X=(\d+)$", geometry, re.MULTILINE).group(1))
                app_width = int(re.search(r"^WIDTH=(\d+)$", geometry,
                                          re.MULTILINE).group(1))
                debugger_left = x0 + max(0, (app_width - 760) // 2)
                register_label = find_text(root, "Register")
                edit_y = register_label[1] + register_label[3] + 16
                edit_x = debugger_left + 560
                run("xdotool", "mousemove", str(edit_x), str(edit_y), "click", "1",
                    "key", "ctrl+a", "type", "--clearmodifiers", "0100")
                click_text(root, "Set")
                invalid = screenshot_text(root)
                if "Invalid debugger register value" not in invalid:
                    raise RuntimeError("out-of-range byte register edit was not rejected")
                assertions.append("byte-register-width-rejected")

                run("xdotool", "mousemove", str(edit_x), str(edit_y), "click", "1",
                    "key", "ctrl+a", "type", "--clearmodifiers", "5A")
                click_text(root, "Set")
                updated = screenshot_text(root)
                if not re.search(r"\bAF\s+5A[0-9A-F]{2}", updated):
                    raise RuntimeError("main A register edit was not reflected in UI")
                assertions.append("paused-main-register-edit-reflected")
                (args.capture_dir / "debugger-register-edited.png").write_bytes(
                    (root / "screen.png").read_bytes())

                click_text(root, "Undo")
                restored = screenshot_text(root)
                if not re.search(rf"\bAF\s+{original_af}\b", restored):
                    raise RuntimeError("register undo did not restore the prior AF value")
                assertions.append("register-undo-restored-prior-af")
                (args.capture_dir / "debugger-register-undo.png").write_bytes(
                    (root / "screen.png").read_bytes())

                args.proof.write_text(json.dumps({
                    "testId": "gui-debugger-registers",
                    "status": "pass",
                    "commit": commit,
                    "runId": int(os.environ.get("GITHUB_RUN_ID", "0")),
                    "runner": os.environ.get("RUNNER_OS", ""),
                    "architecture": os.environ.get("RUNNER_ARCH", ""),
                    "timestamp": datetime.now(timezone.utc).isoformat(),
                    "assertions": assertions,
                    "assertionCount": len(assertions),
                    "originalAf": original_af,
                    "fixtures": fixtures,
                    "captures": sorted(path.name for path in args.capture_dir.glob("*.png")),
                }, indent=2) + "\n", encoding="utf-8")
        finally:
            if process is not None and process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()


if __name__ == "__main__":
    main()
