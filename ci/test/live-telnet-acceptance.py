#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.
"""Exercise the production control socket and retain path-free aggregate proof."""

import argparse
import json
import os
from pathlib import Path
import select
import socket
import subprocess
import time


def connect(process):
    deadline = time.monotonic() + 120
    while time.monotonic() < deadline:
        if process.poll() is not None:
            raise RuntimeError("application exited before the listener was ready")
        for port in range(30740, 32788):
            connection = socket.socket()
            connection.settimeout(0.1)
            try:
                connection.connect(("127.0.0.1", port))
                connection.settimeout(10)
                return connection
            except OSError:
                connection.close()
        time.sleep(0.25)
    raise RuntimeError("application listener did not become available")


class Control:
    def __init__(self, connection):
        self.connection = connection
        self.buffer = bytearray()
        while select.select([connection], [], [], 0.2)[0]:
            data = connection.recv(4096)
            if not data:
                raise RuntimeError("listener closed during greeting")
        self.buffer.clear()

    def line(self):
        while b"\n" not in self.buffer:
            chunk = self.connection.recv(4096)
            if not chunk:
                raise RuntimeError("listener closed during a response")
            self.buffer.extend(chunk)
            if len(self.buffer) > 131072:
                raise RuntimeError("response exceeded the bounded proof reader")
        line, _, remainder = self.buffer.partition(b"\n")
        self.buffer = bytearray(remainder)
        return line.rstrip(b"\r").decode("ascii", "strict")

    def send(self, value, multiline=False):
        self.connection.sendall((value + "\n").encode("ascii"))
        lines = []
        while True:
            line = self.line()
            lines.append(line)
            if multiline and line == "END":
                return lines
            if not multiline and (line.startswith(("OK", "ERR ", "DENIED ", "STATUS "))):
                return lines


def require(actual, expected):
    if actual[-1] != expected:
        raise RuntimeError(f"unexpected control response for {expected.split()[0]}")


def no_host_path(lines):
    body = "\n".join(lines)
    if any(fragment in body for fragment in ("/Users/", "/home/", "C:\\", "D:\\")):
        raise RuntimeError("remote discovery disclosed a host path")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--proof", type=Path, required=True)
    args = parser.parse_args()
    if not args.binary.is_file() or not args.rom.is_file():
        raise RuntimeError("required runner input is unavailable")
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    environment["WZSN_ROM_PATH"] = str(args.rom.resolve())
    process = None
    connection = None
    assertions = 0
    with (args.proof.parent / "application.log").open("w", encoding="utf-8") as log:
        try:
            process = subprocess.Popen([str(args.binary.resolve())], env=environment,
                                       stdout=log, stderr=subprocess.STDOUT)
            connection = connect(process)
            control = Control(connection)

            require(control.send("KEY DOWN A"), "OK")
            assertions += 1
            require(control.send("KEY PRESS A"), "ERR BAD_STATE")
            assertions += 1
            require(control.send("KEY UP A"), "OK")
            assertions += 1
            require(control.send("KEY PRESS A"), "OK")
            assertions += 1
            require(control.send("KEY DOWN NOT_A_KEY"), "ERR BAD_KEY")
            assertions += 1
            require(control.send("RELEASE ALL"), "OK")
            assertions += 1

            tree = control.send("MENU TREE", multiline=True)
            if not any(line.startswith("ITEM file.open_run ") and "REMOTE=DENIED" in line
                       for line in tree):
                raise RuntimeError("remote tree omitted the denied file command")
            no_host_path(tree)
            assertions += 2

            description = control.send("DESCRIBE file.open_run", multiline=True)
            if "ID=file.open_run" not in description:
                raise RuntimeError("remote description omitted its stable ID")
            no_host_path(description)
            assertions += 2

            status = control.send("STATUS")
            if "STATE=RUNNING" not in status[-1]:
                raise RuntimeError("initial application state was not running")
            no_host_path(status)
            assertions += 2

            denied = control.send("DO application.quit")
            if not denied[-1].startswith("DENIED application.quit ") or process.poll() is not None:
                raise RuntimeError("remote quit crossed the permission boundary")
            assertions += 1

            paused = control.send("PAUSE")
            if not paused[-1].startswith("OK"):
                raise RuntimeError("remote pause failed")
            if "STATE=PAUSED" not in control.send("STATUS")[-1]:
                raise RuntimeError("remote pause was not reflected in status")
            assertions += 2
            reset = control.send("RESET")
            if not reset[-1].startswith("OK") or "STATE=PAUSED" not in control.send("STATUS")[-1]:
                raise RuntimeError("reset changed the paused host state")
            assertions += 1
            resumed = control.send("RESUME")
            if not resumed[-1].startswith("OK") or "STATE=RUNNING" not in control.send("STATUS")[-1]:
                raise RuntimeError("remote resume failed")
            assertions += 1

            args.proof.write_text(json.dumps({
                "testId": "production-live-telnet-acceptance",
                "status": "pass", "commit": os.environ.get("GITHUB_SHA", ""),
                "runId": int(os.environ.get("GITHUB_RUN_ID", "0")),
                "runner": os.environ.get("RUNNER_OS", ""),
                "architecture": os.environ.get("RUNNER_ARCH", ""),
                "assertions": assertions,
                "checks": ["keyboard-responses", "path-free-discovery", "remote-denial",
                           "pause-reset-resume"],
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
