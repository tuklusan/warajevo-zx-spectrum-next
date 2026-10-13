#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Commercial Use and use for AI/ML model training are
# prohibited unless separately authorized. Attribution is required. See
# LICENSE for full terms.
"""Hosted regression for the Fuse display comparator's safety checks."""

import json
import subprocess
import sys
import tempfile
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[3]
COMPARATOR = ROOT / "ci/test/fuse-visual-equivalence/compare.py"
FUSE_SIZE = (320, 240)
APP_SIZE = (448, 312)
ACTIVE_SIZE = (256, 192)


def active_pattern():
    image = Image.new("RGB", ACTIVE_SIZE)
    pixels = image.load()
    for y in range(ACTIVE_SIZE[1]):
        for x in range(ACTIVE_SIZE[0]):
            pixels[x, y] = ((x * 13) % 256, (y * 17) % 256,
                            ((x // 8 + y // 8) * 31) % 256)
    return image


def frames():
    palette = (("#132435", "#314253", "#516273"),
               ("#17394b", "#ffffff", "#537587"),
               ("#1b4d5f", "#6b8d9f", "#8bacbd"))
    fuse = Image.new("RGB", FUSE_SIZE)
    application = Image.new("RGB", APP_SIZE)
    fuse_x = ((0, 32), (32, 288), (288, 320))
    fuse_y = ((0, 24), (24, 216), (216, 240))
    app_x = ((0, 96), (96, 352), (352, 448))
    app_y = ((0, 60), (60, 252), (252, 312))
    for row in range(3):
        for column in range(3):
            color = palette[row][column]
            fuse.paste(color, (fuse_x[column][0], fuse_y[row][0],
                               fuse_x[column][1], fuse_y[row][1]))
            application.paste(color, (app_x[column][0], app_y[row][0],
                                      app_x[column][1], app_y[row][1]))
    active = active_pattern()
    fuse.paste(active, (32, 24))
    application.paste(active, (96, 60))
    return fuse, application


def compare(reference, candidate, output, extra_args=()):
    return subprocess.run(
        [sys.executable, str(COMPARATOR), "--reference", str(reference),
         "--candidate", str(candidate), "--output", str(output), *extra_args],
        capture_output=True, text=True, check=False)


def main():
    with tempfile.TemporaryDirectory(prefix="wz-fuse-visual-contract-") as temp:
        root = Path(temp)
        reference = root / "reference.png"
        candidate = root / "candidate.png"
        output = root / "metrics.json"
        fuse, application = frames()
        fuse.save(reference)
        application.save(candidate)

        result = compare(reference, candidate, output)
        if result.returncode != 0:
            raise SystemExit(f"identical full-frame captures rejected: {result.stderr}")
        metrics = json.loads(output.read_text(encoding="utf-8"))
        if metrics["status"] != "pass" or metrics["mismatchedPixels"] != 0:
            raise SystemExit("identical full-frame captures did not report zero mismatch")

        application.transpose(Image.Transpose.FLIP_LEFT_RIGHT).save(candidate)
        result = compare(reference, candidate, output)
        if result.returncode == 0 or "visual-equivalence gate failed" not in result.stderr:
            raise SystemExit("mirrored display was not rejected")

        application.transpose(Image.Transpose.FLIP_TOP_BOTTOM).save(candidate)
        result = compare(reference, candidate, output)
        if result.returncode == 0 or "visual-equivalence gate failed" not in result.stderr:
            raise SystemExit("vertically flipped display was not rejected")

        application.crop((96, 60, 352, 252)).save(candidate)
        result = compare(reference, candidate, output)
        if result.returncode == 0 or "active-area-only captures are not accepted" not in result.stderr:
            raise SystemExit("active-area-only display crop was not rejected")

        application.save(candidate)
        result = compare(reference, candidate, output,
                        ("--display-size", "256", "192"))
        if result.returncode == 0 or "must preserve the full 448 by 312 frame" not in result.stderr:
            raise SystemExit("reduced comparison frame was not rejected")

    print("PASS Fuse comparator aligns full frames and rejects horizontal/vertical flips, active-only crops, and reduced output")


if __name__ == "__main__":
    main()
