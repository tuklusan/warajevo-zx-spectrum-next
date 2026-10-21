#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.
"""Verify production Telnet screenshot path, PNG, and state behavior."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import runpy
import struct
import subprocess
import tempfile
import time

helpers = runpy.run_path(str(Path(__file__).with_name("gui-screenshot-save.py")))
connect = helpers["connect"]
command = helpers["command"]
window = helpers["window"]


def status(sock):
    response = command(sock, "STATUS")
    match = re.search(r"(?m)^STATUS .*", response)
    if match is None:
        raise RuntimeError(f"STATUS response missing: {response.strip()}")
    return match.group(0)


def capture(sock, directory):
    deadline = time.monotonic() + 20
    while True:
        response = command(sock, "SCREENSHOT")
        match = re.search(r"(?m)^OK SCREENSHOT (.+)$", response)
        if match is not None:
            path = Path(match.group(1).strip())
            break
        if "ERR SCREENSHOT no-raster" not in response or time.monotonic() >= deadline:
            raise RuntimeError(f"SCREENSHOT did not return a completed path: {response.strip()}")
        time.sleep(0.2)

    if not path.is_absolute() or path.parent.resolve() != directory.resolve():
        raise RuntimeError(f"screenshot path is not absolute in OS temp: {path}")
    if re.fullmatch(r"ZX-Screen-\d{17}(?:-\d+)?\.png", path.name) is None:
        raise RuntimeError(f"screenshot filename violates the timestamp contract: {path.name}")
    png = path.read_bytes()
    if (len(png) < 24 or png[:8] != b"\x89PNG\r\n\x1a\n" or
            png[12:16] != b"IHDR" or struct.unpack(">II", png[16:24]) != (448, 312)):
        raise RuntimeError("screenshot is not a valid 448x312 PNG")
    path.unlink()
    return png


def stop(process, sock):
    if sock is not None:
        sock.close()
    if process is not None and process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=10)


def launch(binary, rom, root, environment):
    root.mkdir(parents=True, exist_ok=True)
    environment = environment.copy()
    environment["WZSN_ROM_PATH"] = str(rom.resolve())
    environment["XDG_CONFIG_HOME"] = str(root / "config")
    log_path = root / "application.log"
    log = log_path.open("w", encoding="utf-8")
    process = subprocess.Popen([str(binary.resolve())], cwd=root,
                               env=environment, stdout=log,
                               stderr=subprocess.STDOUT)
    try:
        window(helpers["TITLE"])
        return process, connect(process), log
    except Exception:
        stop(process, None)
        log.close()
        raise


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--proof", type=Path, required=True)
    args = parser.parse_args()
    if not args.binary.is_file() or not args.rom.is_file():
        raise RuntimeError("required runner input is unavailable")

    assertions = []
    with tempfile.TemporaryDirectory(prefix="wz-screenshot-runtime-") as temporary:
        root = Path(temporary)
        os_temp = root / "os-temp"
        os_temp.mkdir()
        environment = os.environ.copy()
        environment["TMPDIR"] = str(os_temp)
        environment.pop("WZSN_SCREENSHOT_DIR", None)
        process = sock = log = None
        try:
            process, sock, log = launch(args.binary, args.rom, root / "success",
                                       environment)
            running_before = status(sock)
            running_png = capture(sock, os_temp)
            if status(sock) != running_before or "STATE=RUNNING" not in running_before:
                raise RuntimeError("successful screenshot changed running state")
            assertions.extend(("running-state-preserved", "absolute-os-temp-path",
                               "timestamped-filename", "valid-448x312-png"))

            if not command(sock, "PAUSE").startswith("OK"):
                raise RuntimeError("could not pause before screenshot state check")
            paused_before = status(sock)
            paused_png = capture(sock, os_temp)
            if status(sock) != paused_before or "STATE=PAUSED" not in paused_before:
                raise RuntimeError("successful screenshot changed paused state")
            repeat_png = capture(sock, os_temp)
            if repeat_png != paused_png:
                raise RuntimeError("repeated paused screenshots changed raster bytes")
            assertions.extend(("paused-state-preserved", "paused-raster-repeatable"))

            if not command(sock, "RESUME").startswith("OK") or \
                    "STATE=RUNNING" not in status(sock):
                raise RuntimeError("screenshot workflow did not resume cleanly")
            assertions.append("pause-resume-state-preserved")
            running_hash = hashlib.sha256(running_png).hexdigest()
            paused_hash = hashlib.sha256(paused_png).hexdigest()
            stop(process, sock)
            log.close()
            process = sock = log = None

            failure_root = root / "failure"
            failure_root.mkdir()
            blocker = root / "not-a-directory"
            blocker.write_text("file blocks screenshot directory", encoding="ascii")
            failed_environment = environment.copy()
            failed_environment["WZSN_SCREENSHOT_DIR"] = str(blocker)
            failed, failed_sock, failed_log = launch(
                args.binary, args.rom, failure_root, failed_environment)
            try:
                before_failure = status(failed_sock)
                failure = command(failed_sock, "SCREENSHOT")
                if not failure.startswith("ERR SCREENSHOT ") or "PATH=" in failure:
                    raise RuntimeError(f"failed screenshot reported a path: {failure.strip()}")
                if status(failed_sock) != before_failure:
                    raise RuntimeError("failed screenshot changed machine status")
                assertions.extend(("failed-write-returns-error-without-path",
                                   "failed-write-preserves-status"))
            finally:
                stop(failed, failed_sock)
                failed_log.close()

        finally:
            stop(process, sock)
            if log is not None:
                log.close()

    args.proof.parent.mkdir(parents=True, exist_ok=True)
    args.proof.write_text(json.dumps({
        "testId": "telnet-screenshot-runtime",
        "status": "pass",
        "commit": os.environ.get("GITHUB_SHA", ""),
        "runId": int(os.environ.get("GITHUB_RUN_ID", "0")),
        "runner": os.environ.get("RUNNER_OS", ""),
        "architecture": os.environ.get("RUNNER_ARCH", ""),
        "assertions": assertions,
        "assertionCount": len(assertions),
        "runningPngSha256": running_hash,
        "pausedPngSha256": paused_hash,
    }, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
