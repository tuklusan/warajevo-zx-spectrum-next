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
import runpy
import subprocess
import tempfile
import time

from PIL import Image, ImageEnhance, ImageOps


TITLE = "Warajevo ZX Spectrum Next"
gui_helpers = runpy.run_path(
    str(Path(__file__).with_name("gui-screenshot-save.py")))
connect_control = gui_helpers["connect"]
click_toolbar = gui_helpers["click_toolbar"]


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


def capture(root):
    image = root / "screen.png"
    run("import", "-window", "root", str(image))
    return image


def prepare_ocr(root, image):
    with Image.open(image) as source:
        enlarged = source.resize((source.width * 3, source.height * 3),
                                 Image.Resampling.LANCZOS)
    enhanced = ImageEnhance.Contrast(ImageOps.grayscale(enlarged)).enhance(2.5)
    prepared = root / "ocr.png"
    enhanced.save(prepared)
    return prepared


def screenshot_text(root):
    image = capture(root)
    prepared = prepare_ocr(root, image)
    return run("tesseract", str(prepared), "stdout", "--psm", "6")


def debugger_origin(window):
    geometry = run("xdotool", "getwindowgeometry", "--shell", window)
    app_width = int(re.search(r"^WIDTH=(\d+)$", geometry,
                              re.MULTILINE).group(1))
    app_height = int(re.search(r"^HEIGHT=(\d+)$", geometry,
                               re.MULTILINE).group(1))
    x0 = int(re.search(r"^X=(\d+)$", geometry, re.MULTILINE).group(1))
    y0 = int(re.search(r"^Y=(\d+)$", geometry, re.MULTILINE).group(1))
    return (x0 + max(0, (app_width - 760) // 2),
            y0 + max(0, (app_height - 740) // 2))


def main_af(root, debugger_left, debugger_top):
    observations = []
    for _ in range(3):
        image = capture(root)
        with Image.open(image) as source:
            line = source.crop((debugger_left + 360, debugger_top + 80,
                                debugger_left + 450, debugger_top + 112))
            line = line.resize((line.width * 4, line.height * 4),
                               Image.Resampling.LANCZOS)
            line = ImageEnhance.Contrast(ImageOps.grayscale(line)).enhance(2.5)
        prepared = root / "af-ocr.png"
        line.save(prepared)
        text = run("tesseract", str(prepared), "stdout", "--psm", "7",
                   "-c", "tessedit_char_whitelist=NABCDEF0123456789")
        observations.append(text)
        match = re.search(r"N?AF\s*([0-9A-F]{4})", text)
        if match is not None:
            return match.group(1), text
        time.sleep(0.2)
    raise RuntimeError(f"main AF value is not readable; OCR={observations!r}")


def click_at(x, y):
    run("xdotool", "mousemove", "--sync", str(x), str(y))
    run("xdotool", "mousedown", "1")
    time.sleep(1.0)
    run("xdotool", "mouseup", "1")
    time.sleep(0.5)


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
            with (args.capture_dir / "application.log").open("w", encoding="utf-8") as log:
                process = subprocess.Popen([str(args.binary.resolve())], cwd=root,
                    env=environment, stdout=log, stderr=subprocess.STDOUT)
                window = find_window()
                control = connect_control(process)
                control.close()
                click_toolbar(window, 10)
                screen = screenshot_text(root)
                (args.capture_dir / "debugger-open.png").write_bytes(
                    (root / "screen.png").read_bytes())
                if "register" not in screen.casefold() or "editor" not in screen.casefold():
                    raise RuntimeError(
                        f"production debugger register editor is not visible; OCR={screen!r}")
                assertions.append("debugger-register-controls-visible")

                # Pause through the debugger control itself; the toolbar
                # button is covered by the debugger window.
                run("xdotool", "mousemove", "--sync", "--window", window,
                    "260", "60", "click", "1")
                time.sleep(0.25)
                before = screenshot_text(root)
                debugger_left, debugger_top = debugger_origin(window)
                original_af, _ = main_af(root, debugger_left, debugger_top)
                (args.capture_dir / "debugger-paused.png").write_bytes(
                    (root / "screen.png").read_bytes())
                if "paused" not in before.casefold():
                    raise RuntimeError(
                        f"debugger did not report a paused machine; OCR={before!r}")
                assertions.append("machine-paused-in-debugger")

                # Register editor is directly below its label; value input is
                # the right-hand field, while the left combo defaults to A.
                edit_y = debugger_top + 368
                edit_x = debugger_left + 560
                run("xdotool", "mousemove", str(edit_x), str(edit_y), "click", "1")
                run("xdotool", "key", "ctrl+a", "type", "--clearmodifiers", "100")
                click_at(debugger_left + 190, debugger_top + 400)
                invalid = screenshot_text(root)
                (args.capture_dir / "debugger-register-invalid.png").write_bytes(
                    (root / "screen.png").read_bytes())
                invalid_af, _ = main_af(root, debugger_left, debugger_top)
                if ("Invalid debugger register value" not in invalid or
                        invalid_af != original_af):
                    raise RuntimeError(
                        "out-of-range byte register was not rejected without "
                        f"changing AF={original_af} (read {invalid_af}); "
                        f"screen OCR={invalid!r}")
                assertions.append("byte-register-width-rejected")

                run("xdotool", "mousemove", str(edit_x), str(edit_y), "click", "1")
                run("xdotool", "key", "ctrl+a", "type", "--clearmodifiers", "5A")
                click_at(debugger_left + 190, debugger_top + 400)
                updated = screenshot_text(root)
                (args.capture_dir / "debugger-register-edited.png").write_bytes(
                    (root / "screen.png").read_bytes())
                updated_af, _ = main_af(root, debugger_left, debugger_top)
                if ("Debugger register updated" not in updated or
                        not re.search(r"\bA\s+\S+\s+5A\b", updated) or
                        updated_af != "5A" + original_af[2:]):
                    raise RuntimeError(
                        "main A register edit was not reflected in UI; "
                        f"AF OCR={updated_af!r}; screen OCR={updated!r}")
                assertions.append("paused-main-register-edit-reflected")

                click_at(debugger_left + 570, debugger_top + 400)
                restored = screenshot_text(root)
                restored_af, _ = main_af(root, debugger_left, debugger_top)
                if ("Debugger registers restored" not in restored or
                        restored_af != original_af):
                    raise RuntimeError("register undo did not restore the prior AF value")
                assertions.append("register-undo-restored-prior-af")
                (args.capture_dir / "debugger-register-undo.png").write_bytes(
                    (root / "screen.png").read_bytes())

                monitor_y = debugger_top + 448
                run("xdotool", "mousemove", "--sync",
                    str(debugger_left + 190), str(monitor_y), "click", "1")
                run("xdotool", "key", "ctrl+a")
                run("xdotool", "type", "--clearmodifiers", "--delay", "80",
                    "R SETREG A 5A")
                click_at(debugger_left + 570, monitor_y)
                monitor_result = screenshot_text(root)
                (args.capture_dir / "debugger-monitor-register-edited.png").write_bytes(
                    (root / "screen.png").read_bytes())
                monitor_af, _ = main_af(root, debugger_left, debugger_top)
                if ("Monitor command executed" not in monitor_result or
                        monitor_af != "5A" + original_af[2:]):
                    raise RuntimeError(
                        "R SETREG monitor command did not edit main A; "
                        f"AF OCR={monitor_af!r}; screen OCR={monitor_result!r}")
                assertions.append("monitor-r-setreg-command-updated-register")

                click_at(debugger_left + 570, debugger_top + 400)
                monitor_restored_af, _ = main_af(root, debugger_left, debugger_top)
                if monitor_restored_af != original_af:
                    raise RuntimeError("monitor register undo did not restore AF")
                assertions.append("monitor-r-setreg-undo-restored-register")
                (args.capture_dir / "debugger-monitor-register-undo.png").write_bytes(
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
