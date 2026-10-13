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
SIZE = (448, 312)


def pattern():
    image = Image.new("RGB", SIZE)
    pixels = image.load()
    for y in range(SIZE[1]):
        for x in range(SIZE[0]):
            pixels[x, y] = ((x * 13) % 256, (y * 17) % 256,
                            ((x // 8 + y // 8) * 31) % 256)
    return image


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
        image = pattern()
        image.save(reference)
        image.save(candidate)

        result = compare(reference, candidate, output)
        if result.returncode != 0:
            raise SystemExit(f"identical full-frame captures rejected: {result.stderr}")
        metrics = json.loads(output.read_text(encoding="utf-8"))
        if metrics["status"] != "pass" or metrics["mismatchedPixels"] != 0:
            raise SystemExit("identical full-frame captures did not report zero mismatch")

        image.transpose(Image.Transpose.FLIP_LEFT_RIGHT).save(candidate)
        result = compare(reference, candidate, output)
        if result.returncode == 0 or "visual-equivalence gate failed" not in result.stderr:
            raise SystemExit("mirrored display was not rejected")

        image.crop((0, 0, 256, 192)).save(candidate)
        result = compare(reference, candidate, output)
        if result.returncode == 0 or "active-area-only captures are not accepted" not in result.stderr:
            raise SystemExit("active-area-only display crop was not rejected")

        image.save(candidate)
        result = compare(reference, candidate, output,
                        ("--display-size", "256", "192"))
        if result.returncode == 0 or "must preserve the full 448 by 312 frame" not in result.stderr:
            raise SystemExit("reduced comparison frame was not rejected")

    print("PASS Fuse comparator accepts matching full frames and rejects mirrored or active-only captures")


if __name__ == "__main__":
    main()
