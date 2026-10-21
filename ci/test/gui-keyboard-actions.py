#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.
"""Exercise the production keyboard command menu and verify live machine state."""

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import socket
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


def connect_control(process):
    deadline = time.monotonic() + 25
    while time.monotonic() < deadline:
        if process.poll() is not None:
            raise RuntimeError("production application exited before Control Port startup")
        for port in range(30740, 32788):
            connection = socket.socket()
            connection.settimeout(0.05)
            try:
                connection.connect(("127.0.0.1", port))
                connection.settimeout(5)
                while True:
                    try:
                        if not connection.recv(4096):
                            break
                    except socket.timeout:
                        break
                return connection
            except OSError:
                connection.close()
        time.sleep(0.25)
    raise RuntimeError("production Control Port did not become available")


def send(connection, command):
    connection.sendall((command + "\n").encode("ascii"))
    received = bytearray()
    deadline = time.monotonic() + 10
    while time.monotonic() < deadline:
        try:
            received.extend(connection.recv(4096).replace(b"\xff", b""))
        except socket.timeout:
            continue
        lines = received.split(b"\n")
        if any(line.startswith((b"STATUS ", b"OK", b"ERR ")) for line in lines):
            return received.decode("utf-8", "replace")
    raise RuntimeError(f"Control Port response timed out: {command}")


def status_field(status, field, expected):
    match = re.search(rf"(?:^|\s){re.escape(field)}=([^\s]+)", status)
    if match is None or match.group(1) != expected:
        raise RuntimeError(f"expected {field}={expected}, got {status.strip()!r}")


def focus_viewport(window):
    geometry = run("xdotool", "getwindowgeometry", "--shell", window)
    width = int(re.search(r"^WIDTH=(\d+)$", geometry, re.MULTILINE).group(1))
    height = int(re.search(r"^HEIGHT=(\d+)$", geometry, re.MULTILINE).group(1))
    run("xdotool", "windowfocus", "--sync", window)
    run("xdotool", "mousemove", "--sync", "--window", window,
        str(width // 2), str(height // 2))
    run("xdotool", "click", "1")
    time.sleep(0.25)


def keyboard_action(window, index):
    run("xdotool", "key", "--clearmodifiers", "F1")
    if index:
        run("xdotool", "key", "--clearmodifiers", "--repeat", str(index),
            "--delay", "40", "Down")
    run("xdotool", "key", "--clearmodifiers", "Return")
    time.sleep(0.35)


def fixture_records(driver_path, commit):
    driver = json.loads(driver_path.read_text(encoding="utf-8"))
    records = []
    for item in driver["fixtures"]:
        content = subprocess.check_output(
            ["git", "show", f"{commit}:{item['path']}"])
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
    args = parser.parse_args()
    if not args.binary.is_file() or not args.rom.is_file():
        raise RuntimeError("production application or certified ROM is unavailable")

    commit = os.environ.get("GITHUB_SHA", "")
    if not re.fullmatch(r"[0-9a-f]{40}", commit):
        raise RuntimeError("workflow commit SHA is unavailable")
    fixtures = fixture_records(args.driver, commit)
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    assertions = []
    with tempfile.TemporaryDirectory(prefix="wz-keyboard-actions-") as directory:
        root = Path(directory)
        environment = os.environ.copy()
        environment["WZSN_ROM_PATH"] = str(args.rom.resolve())
        environment["XDG_CONFIG_HOME"] = str(root / "config")
        process = None
        connection = None
        try:
            with (root / "application.log").open("w", encoding="utf-8") as log:
                process = subprocess.Popen([str(args.binary.resolve())], cwd=root,
                    env=environment, stdout=log, stderr=subprocess.STDOUT)
                window = find_window()
                connection = connect_control(process)
                focus_viewport(window)

                initial = send(connection, "STATUS")
                status_field(initial, "MODEL", "48K")
                status_field(initial, "STATE", "RUNNING")
                status_field(initial, "SPEED", "100")
                assertions.append("initial-supported-machine")

                run("xdotool", "key", "--clearmodifiers", "F1")
                run("xdotool", "key", "--clearmodifiers", "Escape")
                time.sleep(0.2)
                closed = send(connection, "STATUS")
                status_field(closed, "MODEL", "48K")
                assertions.append("keyboard-menu-cancel")

                speed_values = ("25", "50", "100", "200", "400", "800",
                                "UNLIMITED")
                for index, speed in enumerate(speed_values, start=6):
                    keyboard_action(window, index)
                    current = send(connection, "STATUS")
                    status_field(current, "SPEED", speed)
                    status_field(current, "STATE", "RUNNING")
                    assertions.append(f"keyboard-speed-{speed.casefold()}")

                keyboard_action(window, 14)
                current = send(connection, "STATUS")
                status_field(current, "MODEL", "128K")
                assertions.append("keyboard-model-128k")
                keyboard_action(window, 13)
                current = send(connection, "STATUS")
                status_field(current, "MODEL", "48K")
                assertions.append("keyboard-model-48k")

                keyboard_action(window, 4)
                current = send(connection, "STATUS")
                status_field(current, "STATE", "PAUSED")
                assertions.append("keyboard-pause")
                keyboard_action(window, 4)
                current = send(connection, "STATUS")
                status_field(current, "STATE", "RUNNING")
                assertions.append("keyboard-resume")

                keyboard_action(window, 4)
                keyboard_action(window, 5)
                current = send(connection, "STATUS")
                status_field(current, "STATE", "PAUSED")
                status_field(current, "MODEL", "48K")
                assertions.append("keyboard-reset-preserves-paused-state")
                keyboard_action(window, 4)
                status_field(send(connection, "STATUS"), "STATE", "RUNNING")
                assertions.append("keyboard-resume-after-reset")

                args.proof.write_text(json.dumps({
                    "testId": "gui-keyboard-actions",
                    "status": "pass",
                    "commit": commit,
                    "runId": int(os.environ.get("GITHUB_RUN_ID", "0")),
                    "runner": os.environ.get("RUNNER_OS", ""),
                    "architecture": os.environ.get("RUNNER_ARCH", ""),
                    "timestamp": datetime.now(timezone.utc).isoformat(),
                    "assertions": assertions,
                    "assertionCount": len(assertions),
                    "fixtures": fixtures,
                    "finalStatus": current.strip(),
                }, indent=2) + "\n", encoding="utf-8")
        finally:
            if connection is not None:
                connection.close()
            if process is not None and process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()


if __name__ == "__main__":
    main()
