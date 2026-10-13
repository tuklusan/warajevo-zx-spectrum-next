#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Commercial Use and use for AI/ML model training are
# prohibited unless separately authorized. Attribution is required. See
# LICENSE for full terms.
"""Validate and pin a hosted Fuse screen-reference capture."""

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
import struct
import zlib
from pathlib import Path


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--result", type=Path, required=True)
    parser.add_argument("--screen", type=Path, required=True)
    parser.add_argument("--proof-out", type=Path, required=True)
    parser.add_argument("--fuse-revision", required=True)
    parser.add_argument("--libspectrum-revision", required=True)
    args = parser.parse_args()

    rom = args.rom.read_bytes()
    result = json.loads(args.result.read_text(encoding="utf-8"))
    screen_bytes = args.screen.read_bytes()
    if len(rom) != 16384:
        raise SystemExit("certified 48K ROM image must be exactly 16384 bytes")
    if len(screen_bytes) < 24 or screen_bytes[:8] != b"\x89PNG\r\n\x1a\n":
        raise SystemExit("Fuse screen artifact is not a valid PNG")
    width, height = struct.unpack(">II", screen_bytes[16:24])
    if (width, height) != (320, 240):
        raise SystemExit(
            f"pinned Fuse full-display PNG must be 320 by 240 pixels; "
            f"received {width} by {height}")

    execution = result.get("execution", {})
    if execution.get("actual_machine") != "48":
        raise SystemExit("Fuse did not run the 48K machine")
    if execution.get("termination", {}).get("type") != "frames":
        raise SystemExit("Fuse reference did not finish at its frame limit")
    if execution.get("frames_completed") != 200:
        raise SystemExit("Fuse reference did not complete exactly 200 frames")
    expected_rom_crc = f"{zlib.crc32(rom) & 0xffffffff:08x}"
    active_roms = result.get("identity", {}).get("active_roms", [])
    if active_roms != [{"page": 0, "size": 16384, "crc32": expected_rom_crc}]:
        raise SystemExit("Fuse loaded ROM identity does not match the certified image")
    screen_info = result.get("artifacts", {}).get("screen", {})
    if screen_info.get("path") != "screen.png" or \
            screen_info.get("size") != len(screen_bytes):
        raise SystemExit("Fuse result metadata does not identify the screen PNG")
    if screen_info.get("width") != width or screen_info.get("height") != height:
        raise SystemExit("Fuse result metadata does not match its PNG dimensions")
    if screen_info.get("file_crc32") != f"{zlib.crc32(screen_bytes) & 0xffffffff:08x}":
        raise SystemExit("Fuse result metadata does not match the screen PNG")
    if any(item.get("severity") == "error" for item in result.get("diagnostics", [])):
        raise SystemExit("Fuse reported an error during reference capture")

    proof = {
        "status": "pass",
        "projectCommit": os.environ["GITHUB_SHA"],
        "runId": os.environ["GITHUB_RUN_ID"],
        "capturedAt": datetime.now(timezone.utc).isoformat(),
        "runner": f"{os.environ['RUNNER_OS']}-{os.environ['RUNNER_ARCH']}",
        "fuseRevision": args.fuse_revision,
        "libspectrumRevision": args.libspectrum_revision,
        "machine": "48",
        "framesCompleted": 200,
        "romSha256": sha256(args.rom),
        "screenSha256": sha256(args.screen),
        "screenSize": [width, height],
        "screenPixelCrc32": screen_info.get("pixel_crc32"),
        "captureResult": args.result.name,
        "screenCapture": args.screen.name,
    }
    args.proof_out.parent.mkdir(parents=True, exist_ok=True)
    args.proof_out.write_text(json.dumps(proof, indent=2) + "\n",
                              encoding="utf-8")
    print(json.dumps(proof, sort_keys=True))


if __name__ == "__main__":
    main()
