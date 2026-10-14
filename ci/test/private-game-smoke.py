#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.
"""Observe authentic-ROM game loading without publishing private media frames."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import socket
import subprocess
import time

from PIL import Image, ImageChops, ImageStat


def connect(process):
    deadline = time.monotonic() + 120.0
    while time.monotonic() < deadline:
        if process.poll() is not None:
            raise RuntimeError("application exited before its control port was ready")
        for port in range(30740, 32788):
            sock = socket.socket()
            sock.settimeout(0.25)
            try:
                sock.connect(("127.0.0.1", port))
                sock.settimeout(1.0)
                return sock
            except OSError:
                sock.close()
            if time.monotonic() >= deadline:
                break
        time.sleep(0.5)
    raise RuntimeError("application control port did not become available")


def command(sock, value):
    sock.sendall((value + "\n").encode("ascii"))
    response = bytearray()
    deadline = time.monotonic() + 10.0
    while time.monotonic() < deadline:
        try:
            part = sock.recv(4096)
        except socket.timeout:
            continue
        if not part:
            break
        response.extend(part.replace(b"\xff", b""))
        if b"\n" in response and (b"OK " in response or b"ERR " in response):
            break
    text = response.decode("utf-8", "replace")
    if "ERR " in text or "OK " not in text:
        reason = next((line for line in text.splitlines() if line.startswith("ERR ")), "no response")
        raise RuntimeError("control command failed: " + value + ": " + reason)
    return text


def screenshot(sock):
    deadline = time.monotonic() + 20.0
    while True:
        try:
            response = command(sock, "SCREENSHOT")
            break
        except RuntimeError as error:
            if "ERR SCREENSHOT no-raster" not in str(error) or time.monotonic() >= deadline:
                raise
            time.sleep(0.25)
    match = re.search(
        r'OK (?:SCREENSHOT\s+|DO host\.screenshot\.temp\s+PATH=)"?([^"\r\n]+)',
        response,
    )
    if match is None:
        raise RuntimeError("screenshot response omitted its destination")
    path = Path(match.group(1).strip())
    data = path.read_bytes()
    with Image.open(path) as image:
        frame = image.convert("RGB")
    return frame, hashlib.sha256(data).hexdigest()


def sample(frame, digest, baseline, elapsed):
    if frame.size != baseline.size:
        raise RuntimeError("Spectrum frame dimensions changed during loading")
    difference = ImageChops.difference(frame, baseline)
    histogram = difference.convert("L").histogram()
    pixels = frame.width * frame.height
    changed = pixels - histogram[0]
    return {
        "elapsedSeconds": elapsed,
        "frameSha256": digest,
        "width": frame.width,
        "height": frame.height,
        "changedPixelsFromBoot": changed,
        "changedPercentFromBoot": round(100.0 * changed / pixels, 6),
        "meanChannelDifferenceFromBoot": round(
            sum(ImageStat.Stat(difference).mean) / 3.0, 6
        ),
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--tape", type=Path, required=True)
    parser.add_argument("--proof", type=Path, required=True)
    args = parser.parse_args()
    for path in (args.binary, args.rom, args.tape):
        if not path.is_file() or path.stat().st_size < 1000:
            raise RuntimeError("required runner input is unavailable")
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    environment["WZSN_ROM_PATH"] = str(args.rom.resolve())
    environment["WZSN_TAPE_PATH"] = str(args.tape.resolve())
    process = None
    sock = None
    with (args.proof.parent / "application.log").open("w", encoding="utf-8") as log:
        try:
            process = subprocess.Popen(
                [str(args.binary.resolve())], env=environment,
                stdout=log, stderr=subprocess.STDOUT,
            )
            sock = connect(process)
            command(sock, "HELP")
            boot, _ = screenshot(sock)
            command(sock, "SPEED 400")
            command(sock, "KEY PRESS J")
            command(sock, "KEY DOWN SYMBOL_SHIFT")
            command(sock, "KEY PRESS P")
            command(sock, "KEY PRESS P")
            command(sock, "KEY UP SYMBOL_SHIFT")
            command(sock, "KEY PRESS ENTER")
            started = time.monotonic()
            samples = []
            for target in (30.0, 90.0, 180.0):
                remaining = target - (time.monotonic() - started)
                if remaining > 0:
                    time.sleep(remaining)
                frame, digest = screenshot(sock)
                samples.append(sample(
                    frame, digest, boot, round(time.monotonic() - started, 3)
                ))
            proof = {
                "testId": "private-game-rom-loading-observation",
                "status": "observation",
                "commit": os.environ.get("GITHUB_SHA", ""),
                "runId": int(os.environ.get("GITHUB_RUN_ID", "0")),
                "runner": "macOS ARM64",
                "mediaClass": "private ZX Spectrum game TAP",
                "romSha256": hashlib.sha256(args.rom.read_bytes()).hexdigest(),
                "speedPercent": 400,
                "normalTapeTransport": True,
                "samples": samples,
                "limitations": [
                    "Aggregate frame metrics are an observation, not gameplay or Fuse visual equivalence proof.",
                    "Private media bytes, media paths, and screenshots are not uploaded.",
                ],
            }
            args.proof.write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
        finally:
            if sock is not None:
                sock.close()
            if process is not None and process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()


if __name__ == "__main__":
    main()
