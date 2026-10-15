#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.
"""Check production chooser cancellation and rejected file operations."""

import argparse
import hashlib
import json
import os
from pathlib import Path
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


def choose_name(dialog, name):
    xdotool("windowfocus", "--sync", dialog)
    xdotool("mousemove", "--window", dialog, "300", "26")
    xdotool("click", "1")
    xdotool("key", "ctrl+a")
    xdotool("type", "--clearmodifiers", name)
    xdotool("key", "Return")
    time.sleep(0.8)


def unchanged(sock, original, operation):
    if "STATE=PAUSED" not in command(sock, "STATUS"):
        raise RuntimeError(f"{operation} changed pause state")
    if screenshot(sock) != original:
        raise RuntimeError(f"{operation} changed the paused raster")


def run(args):
    with tempfile.TemporaryDirectory(prefix="wz-gui-file-state-") as temporary:
        root = Path(temporary)
        (root / "broken.z80").write_bytes(b"invalid snapshot")
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
                original = screenshot(sock)

                activate(host_window, 3)
                dialog = window("Save Snapshot")
                xdotool("windowfocus", "--sync", dialog)
                xdotool("key", "Escape")
                time.sleep(0.5)
                unchanged(sock, original, "snapshot-save cancellation")

                activate(host_window, 1)
                dialog = window("Open / Run")
                xdotool("windowfocus", "--sync", dialog)
                xdotool("key", "Escape")
                time.sleep(0.5)
                unchanged(sock, original, "snapshot-load cancellation")

                activate(host_window, 3)
                dialog = window("Save Snapshot")
                choose_name(dialog, "invalid.bad")
                unchanged(sock, original, "unsupported snapshot destination")
                if (root / "invalid.bad").exists():
                    raise RuntimeError("rejected snapshot destination was written")

                activate(host_window, 2)
                dialog = window("Save Snapshot")
                xdotool("windowfocus", "--sync", dialog)
                xdotool("key", "Escape")
                time.sleep(0.5)
                unchanged(sock, original, "snapshot destination after rejected save")

                activate(host_window, 1)
                dialog = window("Open / Run")
                xdotool("windowfocus", "--sync", dialog)
                xdotool("key", "ctrl+l")
                xdotool("type", "--clearmodifiers", str(root / "broken.z80"))
                xdotool("key", "Return")
                time.sleep(0.3)
                xdotool("key", "Return")
                time.sleep(1.0)
                unchanged(sock, original, "malformed snapshot load")

                digest = hashlib.sha256(original).hexdigest()
                args.proof.parent.mkdir(parents=True, exist_ok=True)
                args.proof.write_text(json.dumps({
                    "testId": "gui-file-state", "status": "pass",
                    "commit": os.environ.get("GITHUB_SHA", ""),
                    "runId": int(os.environ.get("GITHUB_RUN_ID", "0")),
                    "runner": os.environ.get("RUNNER_OS", ""),
                    "cases": ["snapshot-save-cancel", "snapshot-load-cancel",
                              "unsupported-save-destination",
                              "save-destination-rollback", "malformed-snapshot-load"],
                    "pausedRasterSha256": digest,
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
