#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.
"""Exercise production GUI output failure without changing the machine."""

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


helpers = runpy.run_path(str(Path(__file__).with_name("gui-screenshot-save.py")))
xdotool = helpers["xdotool"]
window = helpers["window"]
connect = helpers["connect"]
command = helpers["command"]
screenshot = helpers["screenshot"]


def activate(host_window, index):
    xdotool("windowfocus", "--sync", host_window)
    xdotool("key", "F1")
    time.sleep(0.3)
    for _ in range(index):
        xdotool("key", "Down")
    xdotool("key", "Return")


def type_name(dialog, name):
    xdotool("windowfocus", "--sync", dialog)
    xdotool("mousemove", "--window", dialog, "300", "26")
    xdotool("click", "1")
    xdotool("key", "ctrl+a")
    xdotool("type", "--clearmodifiers", name)
    time.sleep(0.3)


def submit_save(dialog):
    geometry = xdotool("getwindowgeometry", "--shell", dialog)
    width_match = re.search(r"^WIDTH=(\d+)$", geometry, re.MULTILINE)
    height_match = re.search(r"^HEIGHT=(\d+)$", geometry, re.MULTILINE)
    if width_match is None or height_match is None:
        raise RuntimeError(f"could not read chooser geometry: {geometry}")
    width = int(width_match.group(1))
    height = int(height_match.group(1))
    xdotool("mousemove", "--window", dialog,
            str(width - 49), str(height - 21))
    xdotool("click", "1")


def wait_for_file(path, timeout=12):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if path.exists():
            return path.read_bytes()
        time.sleep(0.2)
    raise RuntimeError(f"expected saved file missing: {path.name}")


def wait_closed(title, timeout=12):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        result = subprocess.run(["xdotool", "search", "--onlyvisible",
                                 "--name", title], capture_output=True)
        if result.returncode != 0:
            return
        time.sleep(0.2)
    raise RuntimeError(f"native chooser did not close: {title}")


def reserve_temporary_names(path, process_id):
    reservations = []
    for attempt in range(1, 129):
        temporary = Path(f"{path}.tmp.{process_id}.{attempt}")
        temporary.write_bytes(b"reserved temporary path")
        reservations.append(temporary)
    return reservations


def check_reservations(reservations):
    if any(path.read_bytes() != b"reserved temporary path"
           for path in reservations):
        raise RuntimeError("an existing temporary path was changed")


def run(args):
    with tempfile.TemporaryDirectory(prefix="wz-gui-output-failure-") as temporary:
        root = Path(temporary)
        saved_snapshot = root / "baseline.z80"
        rejected_snapshot = root / "failed-state.z80"
        rejected_png = root / "failed-screen.png"
        environment = os.environ.copy()
        environment["WZSN_ROM_PATH"] = str(args.rom.resolve())
        environment["XDG_CONFIG_HOME"] = str(root / "config")
        process = None
        sock = None
        try:
            with (root / "application.log").open("w", encoding="utf-8") as log:
                process = subprocess.Popen([str(args.binary.resolve())], cwd=root,
                    env=environment, stdout=log, stderr=subprocess.STDOUT)
                host_window = window(helpers["TITLE"])
                sock = connect(process)
                if "OK" not in command(sock, "PAUSE"):
                    raise RuntimeError("could not pause the host")
                before_png = screenshot(sock)

                activate(host_window, 3)
                dialog = window("Save Snapshot")
                type_name(dialog, "baseline.z80")
                submit_save(dialog)
                before_state = wait_for_file(saved_snapshot)
                wait_closed("Save Snapshot")
                if not before_state:
                    raise RuntimeError("baseline snapshot was empty")

                activate(host_window, 3)
                dialog = window("Save Snapshot")
                type_name(dialog, "failed-state.z80")
                snapshot_reservations = reserve_temporary_names(
                    rejected_snapshot, process.pid)
                submit_save(dialog)
                wait_closed("Save Snapshot")
                if rejected_snapshot.exists():
                    raise RuntimeError("failed snapshot write left an output file")
                check_reservations(snapshot_reservations)

                dialog = helpers["click_screenshot"](host_window)
                type_name(dialog, "failed-screen")
                png_reservations = reserve_temporary_names(
                    rejected_png, process.pid)
                submit_save(dialog)
                wait_closed("Save Screenshot")
                if rejected_png.exists():
                    raise RuntimeError("failed screenshot write left an output file")
                check_reservations(png_reservations)

                if "STATE=PAUSED" not in command(sock, "STATUS"):
                    raise RuntimeError("output failures changed pause state")
                if screenshot(sock) != before_png:
                    raise RuntimeError("output failures changed the paused raster")

                saved_snapshot.unlink()
                activate(host_window, 2)
                if wait_for_file(saved_snapshot) != before_state:
                    raise RuntimeError("output failure changed machine state")

                args.proof.parent.mkdir(parents=True, exist_ok=True)
                args.proof.write_text(json.dumps({
                    "testId": "gui-output-failure", "status": "pass",
                    "commit": os.environ.get("GITHUB_SHA", ""),
                    "runId": int(os.environ.get("GITHUB_RUN_ID", "0")),
                    "runner": os.environ.get("RUNNER_OS", ""),
                    "snapshotCollisionRejected": True,
                    "screenshotCollisionRejected": True,
                    "temporaryFilesPreserved": True,
                    "snapshotDestinationPreserved": True,
                    "snapshotBytesUnchanged": True,
                    "pausedRasterUnchanged": True,
                    "snapshotSha256": hashlib.sha256(before_state).hexdigest(),
                    "rasterPngSha256": hashlib.sha256(before_png).hexdigest(),
                }, indent=2) + "\n", encoding="utf-8")
        except Exception:
            args.proof.parent.mkdir(parents=True, exist_ok=True)
            if process is not None:
                print("desktop process exit status:", process.poll(), flush=True)
            log_path = root / "application.log"
            if log_path.exists():
                log_text = log_path.read_text(encoding="utf-8", errors="replace")
                print("desktop log tail:", log_text[-4000:], flush=True)
                (args.proof.parent / "application-diagnostic.log").write_text(
                    log_text, encoding="utf-8")
            subprocess.run(["import", "-window", "root",
                            str(args.proof.parent / "desktop-diagnostic.png")],
                           check=False, capture_output=True)
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
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--proof", type=Path, required=True)
    run(parser.parse_args())
