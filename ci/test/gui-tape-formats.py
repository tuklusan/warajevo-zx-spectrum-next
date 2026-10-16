#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.
"""Verify production TZX and WAV Tape Manager presentation."""

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
import wave

helpers = runpy.run_path(str(Path(__file__).with_name("gui-tape-actions.py")))
xdotool = helpers["xdotool"]
window = helpers["window"]
connect = helpers["connect"]
command = helpers["command"]
capture = helpers["capture"]
choose = helpers["choose"]
expect_text = helpers["expect_text"]


def make_media(root):
    tzx = root / "synthetic.tzx"
    tzx.write_bytes(b"ZXTape!\x1a\x01\x14" +
                    bytes((0x10, 0xe8, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00)))
    wav = root / "synthetic.wav"
    with wave.open(str(wav), "wb") as output:
        output.setnchannels(1)
        output.setsampwidth(1)
        output.setframerate(8000)
        output.writeframes(bytes(192 if (index // 40) % 2 else 64
                                 for index in range(8000)))
    return (tzx, wav)


def open_manager(host_window):
    geometry = xdotool("getwindowgeometry", "--shell", host_window)
    match = re.search(r"^WIDTH=(\d+)$", geometry, re.MULTILINE)
    if match is None:
        raise RuntimeError("host window width unavailable")
    menu_x = int(int(match.group(1)) * 0.39)
    xdotool("windowfocus", "--sync", host_window)
    xdotool("mousemove", "--sync", "--window", host_window, str(menu_x), "42")
    xdotool("click", "1")
    xdotool("mousemove", "--sync", "--window", host_window, str(menu_x), "130")
    xdotool("click", "--repeat", "3", "5")
    time.sleep(0.4)
    xdotool("mousemove", "--sync", "--window", host_window, str(menu_x), "158")
    xdotool("click", "1")


def one_case(binary, rom, root, media, proof_dir):
    environment = os.environ.copy()
    environment["WZSN_ROM_PATH"] = str(rom.resolve())
    environment["XDG_CONFIG_HOME"] = str(root / "config")
    process = None
    sock = None
    log_path = root / "application.log"
    try:
        with log_path.open("w", encoding="utf-8") as log:
            process = subprocess.Popen([str(binary.resolve())], cwd=root,
                env=environment, stdout=log, stderr=subprocess.STDOUT)
            host_window = window("Warajevo ZX Spectrum Next")
            sock = connect(process)
            expect_text(host_window, proof_dir / f"{media.suffix[1:]}-initial.png",
                        r"Empty\s*/\s*Normal")
            choose(host_window, 0)
            dialog = window("Open / Run")
            xdotool("windowfocus", "--sync", dialog)
            xdotool("key", "ctrl+l")
            xdotool("type", "--clearmodifiers", str(media))
            xdotool("key", "Return")
            time.sleep(0.5)
            xdotool("key", "Return")
            expect_text(host_window, proof_dir / f"{media.suffix[1:]}-mounted.png",
                        r"Tape\W+Normal")
            open_manager(host_window)
            manager_path = proof_dir / f"{media.suffix[1:]}-manager.png"
            deadline = time.monotonic() + 10
            manager = ""
            while time.monotonic() < deadline:
                capture(host_window, manager_path)
                manager = subprocess.run(["tesseract", str(manager_path),
                    "stdout", "--psm", "11"], check=True,
                    capture_output=True, text=True).stdout.strip()
                if re.search(r"Tape Manager", manager, re.IGNORECASE):
                    break
                time.sleep(0.5)
            if not re.search(r"Tape Manager", manager, re.IGNORECASE):
                raise RuntimeError(f"Tape Manager unavailable: {manager!r}")
            if not re.search(rf"Format\s+{media.suffix[1:]}", manager,
                             re.IGNORECASE):
                raise RuntimeError(f"Tape format absent in manager: {manager!r}")
            if media.suffix == ".tzx" and not re.search(r"Block\s+1", manager,
                                                          re.IGNORECASE):
                raise RuntimeError(f"TZX block absent: {manager!r}")
            if media.suffix == ".wav" and not re.search(r"[Ss]ignal", manager):
                raise RuntimeError(f"WAV signal view absent: {manager!r}")
            if "STATE=RUNNING" not in command(sock, "STATUS"):
                raise RuntimeError("Tape format operation changed run state")
            return {
                "format": media.suffix[1:].upper(),
                "mediaSha256": hashlib.sha256(media.read_bytes()).hexdigest(),
                "managerOcr": manager,
                "managerScreenshotSha256": hashlib.sha256(
                    manager_path.read_bytes()).hexdigest(),
            }
    except Exception:
        if process is not None:
            subprocess.run(["import", "-window", "root",
                str(proof_dir / f"{media.suffix[1:]}-diagnostic.png")],
                check=False, capture_output=True)
            (proof_dir / f"{media.suffix[1:]}-application.log").write_text(
                log_path.read_text(encoding="utf-8", errors="replace"),
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


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--proof", type=Path, required=True)
    args = parser.parse_args()
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="wz-gui-tape-formats-") as temporary:
        root = Path(temporary)
        results = []
        for media in make_media(root):
            case_root = root / media.suffix[1:]
            case_root.mkdir()
            results.append(one_case(args.binary, args.rom, case_root, media,
                                    args.proof.parent))
        args.proof.write_text(json.dumps({
            "testId": "gui-tape-formats", "status": "pass",
            "commit": os.environ.get("GITHUB_SHA", ""),
            "runId": int(os.environ.get("GITHUB_RUN_ID", "0")),
            "runner": os.environ.get("RUNNER_OS", ""),
            "cases": results,
        }, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
