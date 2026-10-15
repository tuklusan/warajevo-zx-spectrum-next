#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.
"""Exercise the production Linux screenshot chooser under a virtual display."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import socket
import struct
import subprocess
import tempfile
import time


TITLE = "Warajevo ZX Spectrum Next"


def xdotool(*arguments):
    return subprocess.run(["xdotool", *arguments], check=True,
                          capture_output=True, text=True).stdout.strip()


def window(pattern, timeout=20):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        result = subprocess.run(
            ["xdotool", "search", "--onlyvisible", "--name", pattern],
            capture_output=True, text=True)
        if result.returncode == 0 and result.stdout.strip():
            return result.stdout.splitlines()[-1]
        time.sleep(0.2)
    visible = subprocess.run(
        ["xdotool", "search", "--onlyvisible", "--name", "."],
        capture_output=True, text=True)
    names = []
    for identifier in visible.stdout.splitlines():
        name = subprocess.run(["xdotool", "getwindowname", identifier],
                              capture_output=True, text=True)
        names.append(name.stdout.strip())
    raise RuntimeError(f"window unavailable: {pattern}; visible windows: {names}")


def connect(process):
    deadline = time.monotonic() + 25
    while time.monotonic() < deadline:
        if process.poll() is not None:
            raise RuntimeError("desktop host exited before control was ready")
        for port in range(30740, 32788):
            sock = socket.socket()
            sock.settimeout(0.03)
            try:
                sock.connect(("127.0.0.1", port))
                sock.settimeout(5)
                while True:
                    try:
                        if not sock.recv(4096):
                            break
                    except socket.timeout:
                        break
                return sock
            except OSError:
                sock.close()
        time.sleep(0.25)
    raise RuntimeError("desktop control port unavailable")


def command(sock, value):
    sock.sendall((value + "\n").encode("ascii"))
    response = bytearray()
    deadline = time.monotonic() + 10
    while time.monotonic() < deadline:
        try:
            response.extend(sock.recv(4096).replace(b"\xff", b""))
        except socket.timeout:
            continue
        lines = [line.rstrip(b"\r") for line in response.split(b"\n")[:-1]]
        if any(line == b"OK" or line.startswith((b"OK ", b"ERR ", b"STATUS "))
               for line in lines):
            return response.decode("utf-8", "replace")
    raise RuntimeError(f"control response unavailable: {value}")


def screenshot(sock):
    deadline = time.monotonic() + 20
    while True:
        answer = command(sock, "SCREENSHOT")
        match = re.search(
            r'OK (?:SCREENSHOT\s+|DO host\.screenshot\.temp\s+PATH=)"?([^"\r\n]+)',
            answer)
        if match is not None:
            break
        if "ERR SCREENSHOT no-raster" not in answer or time.monotonic() >= deadline:
            raise RuntimeError(f"Telnet screenshot did not return a path: {answer.strip()}")
        time.sleep(0.25)
    path = Path(match.group(1))
    try:
        return path.read_bytes()
    finally:
        path.unlink(missing_ok=True)


def click_screenshot(host_window):
    xdotool("windowfocus", "--sync", host_window)
    geometry = xdotool("getwindowgeometry", "--shell", host_window)
    width = int(re.search(r"^WIDTH=(\d+)$", geometry, re.MULTILINE).group(1))
    xdotool("mousemove", "--window", host_window,
            str(int(width * 8.5 / 11)), "42")
    xdotool("click", "1")
    return window("Save Screenshot")


def png_dimensions(data):
    if data[:8] != b"\x89PNG\r\n\x1a\n" or data[12:16] != b"IHDR":
        raise RuntimeError("saved screenshot is not a PNG")
    return struct.unpack(">II", data[16:24])


def run(args):
    with tempfile.TemporaryDirectory(prefix="wz-gui-screenshot-") as temporary:
        root = Path(temporary)
        destination = root / "selected-screen.png"
        environment = os.environ.copy()
        environment["WZSN_ROM_PATH"] = str(args.rom.resolve())
        environment["XDG_CONFIG_HOME"] = str(root / "config")
        process = None
        sock = None
        try:
            with (root / "application.log").open("w", encoding="utf-8") as log:
                process = subprocess.Popen([str(args.binary.resolve())],
                    env=environment, stdout=log, stderr=subprocess.STDOUT)
                host_window = window(TITLE)
                sock = connect(process)
                if "OK" not in command(sock, "PAUSE"):
                    raise RuntimeError("could not pause the host")
                before = screenshot(sock)
                if png_dimensions(before) != (448, 312):
                    raise RuntimeError("Telnet capture did not contain the Spectrum raster")
                dialog = click_screenshot(host_window)
                xdotool("windowfocus", "--sync", dialog)
                xdotool("key", "Escape")
                time.sleep(0.5)
                if destination.exists() or "STATE=PAUSED" not in command(sock, "STATUS"):
                    raise RuntimeError("screenshot cancellation changed host state")
                if screenshot(sock) != before:
                    raise RuntimeError("screenshot cancellation changed the paused raster")
                dialog = click_screenshot(host_window)
                xdotool("windowfocus", "--sync", dialog)
                xdotool("key", "ctrl+l")
                xdotool("type", "--clearmodifiers", str(destination))
                xdotool("key", "Return")
                deadline = time.monotonic() + 15
                while time.monotonic() < deadline and not destination.exists():
                    time.sleep(0.2)
                if not destination.exists():
                    xdotool("key", "Return")
                    time.sleep(1)
                if not destination.exists():
                    raise RuntimeError("native chooser did not save the selected PNG")
                saved = destination.read_bytes()
                if saved != before or png_dimensions(saved) != (448, 312):
                    raise RuntimeError("GUI and Telnet PNGs differ for the same paused raster")
                if "STATE=PAUSED" not in command(sock, "STATUS"):
                    raise RuntimeError("screenshot save changed host state")
                args.proof.parent.mkdir(parents=True, exist_ok=True)
                args.proof.write_text(json.dumps({
                    "testId": "gui-screenshot-save", "status": "pass",
                    "commit": os.environ.get("GITHUB_SHA", ""),
                    "runId": int(os.environ.get("GITHUB_RUN_ID", "0")),
                    "runner": os.environ.get("RUNNER_OS", ""),
                    "cancelPreservedPausedRaster": True,
                    "selectedDestinationWritten": True,
                    "guiTelnetPngByteEquivalent": True,
                    "dimensions": [448, 312],
                    "pngSha256": hashlib.sha256(saved).hexdigest(),
                }, indent=2) + "\n", encoding="utf-8")
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
