#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.
"""Observe authentic-ROM game loading without publishing private media frames."""

import argparse
from io import BytesIO
import hashlib
import json
import os
from pathlib import Path
import re
import socket
import subprocess
import time

from PIL import Image, ImageChops, ImageOps, ImageStat


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
        lines = response.split(b"\n")[:-1]
        if any(line.rstrip(b"\r") == b"OK" or line.startswith((b"OK ", b"ERR ")) for line in lines):
            break
    text = response.decode("utf-8", "replace")
    lines = text.splitlines()
    if any(line.startswith("ERR ") for line in lines) or not any(
        line == "OK" or line.startswith("OK ") for line in lines
    ):
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
            raster_pending = (
                "ERR SCREENSHOT no-raster" in str(error)
                or "ERR DO host.screenshot.temp screenshot-failed" in str(error)
            )
            if not raster_pending or time.monotonic() >= deadline:
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


def press_key(sock, key):
    deadline = time.monotonic() + 10.0
    while True:
        try:
            command(sock, "KEY PRESS " + key)
            break
        except RuntimeError as error:
            if "ERR BAD_STATE" not in str(error) or time.monotonic() >= deadline:
                raise
            time.sleep(0.25)
    time.sleep(0.5)


def sample(frame, digest, baseline, elapsed):
    if frame.size != baseline.size:
        raise RuntimeError("Spectrum frame dimensions changed during loading")
    difference = ImageChops.difference(frame, baseline)
    histogram = difference.convert("L").histogram()
    pixels = frame.width * frame.height
    changed = pixels - histogram[0]
    colors = frame.getcolors(pixels)
    palette = sorted(colors or [], reverse=True)[:5]
    ocr_image = ImageOps.autocontrast(frame.convert("L")).resize(
        (frame.width * 4, frame.height * 4), Image.Resampling.NEAREST
    )
    encoded = BytesIO()
    ocr_image.save(encoded, format="PNG")
    recognized = " ".join(
        subprocess.run(
            ["tesseract", "stdin", "stdout", "--psm", mode],
            input=encoded.getvalue(), capture_output=True, check=True,
        ).stdout.decode("utf-8", "replace")
        for mode in ("6", "11")
    ).upper()
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
        "dominantColors": [
            {"rgb": "#%02x%02x%02x" % color, "pixels": count}
            for count, color in palette
        ],
        "textSignals": {
            "romCopyright": "SINCLAIR" in recognized,
            "loadCommand": "LOAD" in recognized,
            "tapeError": "TAPE" in recognized or "ERROR" in recognized,
            "gameTitle": "ABU" in recognized or "SIMBEL" in recognized,
            "startPrompt": "START" in recognized or "PULSA" in recognized,
            "score": "SCORE" in recognized or "PUNTOS" in recognized,
        },
    }


def normalize_fuse(frame):
    if frame.size != (320, 240):
        raise RuntimeError("Fuse reference has unexpected display dimensions")
    normalized = Image.new("RGB", (448, 312))
    for source_y, target_y in zip(((0, 24), (24, 216), (216, 240)),
                                  ((0, 60), (60, 252), (252, 312))):
        for source_x, target_x in zip(((0, 32), (32, 288), (288, 320)),
                                      ((0, 96), (96, 352), (352, 448))):
            tile = frame.crop((source_x[0], source_y[0], source_x[1], source_y[1]))
            normalized.paste(tile.resize((target_x[1] - target_x[0],
                                          target_y[1] - target_y[0]),
                                         Image.Resampling.NEAREST),
                             (target_x[0], target_y[0]))
    return normalized


def compare_fuse(reference_path, frames):
    reference_bytes = reference_path.read_bytes()
    with Image.open(reference_path) as image:
        reference = normalize_fuse(image.convert("RGB"))
    comparisons = []
    for elapsed, frame in frames:
        difference = ImageChops.difference(reference, frame)
        mismatches = sum(
            max(pixel) > 16 for pixel in difference.getdata()
        )
        comparisons.append({
            "elapsedSeconds": elapsed,
            "mismatchPercent": round(100.0 * mismatches / (448 * 312), 6),
            "meanChannelError": round(sum(ImageStat.Stat(difference).mean) / 3.0, 6),
        })
    return {
        "inputSha256": hashlib.sha256(reference_bytes).hexdigest(),
        "sourceSize": [320, 240],
        "channelTolerance": 16,
        "comparisons": comparisons,
        "bestMismatchPercent": min(item["mismatchPercent"] for item in comparisons),
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--tape", type=Path, required=True)
    parser.add_argument("--proof", type=Path, required=True)
    parser.add_argument("--fuse-reference", type=Path)
    parser.add_argument("--sample-seconds", type=float, nargs="+",
                        default=[30.0, 90.0, 180.0, 240.0, 300.0])
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
            time.sleep(5.0)
            boot, boot_digest = screenshot(sock)
            command(sock, "SPEED 100")
            press_key(sock, "J")
            command(sock, "KEY DOWN SYMBOL_SHIFT")
            press_key(sock, "P")
            press_key(sock, "P")
            command(sock, "KEY UP SYMBOL_SHIFT")
            press_key(sock, "ENTER")
            command(sock, "SPEED 400")
            started = time.monotonic()
            samples = []
            frames = []
            for target in args.sample_seconds:
                remaining = target - (time.monotonic() - started)
                if remaining > 0:
                    time.sleep(remaining)
                frame, digest = screenshot(sock)
                samples.append(sample(
                    frame, digest, boot, round(time.monotonic() - started, 3)
                ))
                frames.append((samples[-1]["elapsedSeconds"], frame))
                if target == 240.0:
                    press_key(sock, "ENTER")
                    time.sleep(2.0)
                    for key in ("O", "P", "Q"):
                        press_key(sock, key)
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
                "bootFrameSha256": boot_digest,
                "post240SecondInput": ["ENTER", "O", "P", "Q"]
                if 240.0 in args.sample_seconds else [],
                "samples": samples,
                "limitations": [
                    "Aggregate frame metrics are an observation, not gameplay or Fuse visual equivalence proof.",
                    "Private media bytes, media paths, and screenshots are not uploaded.",
                ],
            }
            if args.fuse_reference is not None:
                proof["fuseReference"] = compare_fuse(args.fuse_reference, frames)
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
