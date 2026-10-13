#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# Proprietary rights reserved except as expressly licensed herein.
#
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Commercial Use and use for AI/ML model training are
# prohibited unless separately authorized. Attribution is required. See
# LICENSE for full terms.
"""Compare normalized application and Fuse Spectrum-display PNG captures."""

import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageChops, ImageStat

FUSE_FRAME = (320, 240)
NORMALIZED_FRAME = (448, 312)
FUSE_X_BANDS = ((0, 32), (32, 288), (288, 320))
FUSE_Y_BANDS = ((0, 24), (24, 216), (216, 240))
TARGET_X_BANDS = ((0, 96), (96, 352), (352, 448))
TARGET_Y_BANDS = ((0, 60), (60, 252), (252, 312))


def load_rgb(path):
    with Image.open(path) as image:
        image.load()
        return image.convert("RGB")


def crop_region(image, region, label):
    if region is None:
        return image
    x, y, width, height = region
    if x < 0 or y < 0 or width <= 0 or height <= 0 or \
            x + width > image.width or y + height > image.height:
        raise SystemExit(f"{label} display crop is outside the captured image")
    return image.crop((x, y, x + width, y + height))


def validate_capture(image, label):
    variance = sum(ImageStat.Stat(image).var) / 3.0
    if variance < 25.0:
        raise SystemExit(f"{label} display capture is blank or uniform")


def normalize_fuse_frame(image):
    if image.size != FUSE_FRAME:
        raise SystemExit(
            f"pinned Fuse screen must be {FUSE_FRAME[0]} by {FUSE_FRAME[1]} pixels; "
            f"received {image.width} by {image.height}")
    normalized = Image.new("RGB", NORMALIZED_FRAME)
    for source_y, target_y in zip(FUSE_Y_BANDS, TARGET_Y_BANDS):
        for source_x, target_x in zip(FUSE_X_BANDS, TARGET_X_BANDS):
            tile = image.crop((source_x[0], source_y[0],
                               source_x[1], source_y[1]))
            size = (target_x[1] - target_x[0], target_y[1] - target_y[0])
            if tile.size != size:
                tile = tile.resize(size, Image.Resampling.NEAREST)
            normalized.paste(tile, (target_x[0], target_y[0]))
    return normalized


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--reference", type=Path, required=True)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--candidate-region", type=int, nargs=4,
                        metavar=("X", "Y", "WIDTH", "HEIGHT"))
    parser.add_argument("--display-size", type=int, nargs=2,
                        default=NORMALIZED_FRAME,
                        metavar=("WIDTH", "HEIGHT"))
    parser.add_argument("--channel-tolerance", type=int, default=16)
    parser.add_argument("--maximum-mismatch-percent", type=float, default=1.0)
    parser.add_argument("--maximum-mean-channel-error", type=float, default=3.0)
    args = parser.parse_args()
    if not 0 <= args.channel_tolerance <= 255:
        parser.error("channel tolerance must be between 0 and 255")
    if not 0 <= args.maximum_mismatch_percent <= 100:
        parser.error("maximum mismatch percentage must be between 0 and 100")
    if args.maximum_mean_channel_error < 0:
        parser.error("maximum mean channel error cannot be negative")

    display_width, display_height = args.display_size
    if display_width < NORMALIZED_FRAME[0] or display_height < NORMALIZED_FRAME[1]:
        parser.error("display size must preserve the full 448 by 312 frame including border")
    reference_input = load_rgb(args.reference)
    candidate = crop_region(load_rgb(args.candidate), args.candidate_region,
                            "application")
    validate_capture(reference_input, "Fuse")
    validate_capture(candidate, "application")
    reference = normalize_fuse_frame(reference_input)
    normalized_size = (display_width, display_height)
    if candidate.width < NORMALIZED_FRAME[0] or \
            candidate.height < NORMALIZED_FRAME[1]:
        raise SystemExit(
            "the application crop must be at least 448 by 312 pixels to include "
            "the full display; active-area-only captures are not accepted")
    reference = reference.resize(normalized_size, Image.Resampling.NEAREST)
    candidate = candidate.resize(normalized_size, Image.Resampling.NEAREST)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    reference_path = args.output.with_name(args.output.stem + "-fuse.png")
    candidate_path = args.output.with_name(args.output.stem + "-application.png")
    reference.save(reference_path)
    candidate.save(candidate_path)
    width, height = reference.size

    difference = ImageChops.difference(reference, candidate)
    stats = ImageStat.Stat(difference)
    mean_channel_error = sum(stats.mean) / 3.0
    pixels = difference.getdata()
    mismatches = sum(
        1 for red, green, blue in pixels
        if max(red, green, blue) > args.channel_tolerance)
    pixel_count = width * height
    mismatch_percent = mismatches * 100.0 / pixel_count
    output_image = args.output.with_name(args.output.stem + "-difference.png")
    difference.save(output_image)
    result = {
        "status": "pass" if (
            mismatch_percent <= args.maximum_mismatch_percent and
            mean_channel_error <= args.maximum_mean_channel_error) else "fail",
        "referenceSha256": hashlib.sha256(reference_path.read_bytes()).hexdigest(),
        "candidateSha256": hashlib.sha256(candidate_path.read_bytes()).hexdigest(),
        "referenceInputSha256": hashlib.sha256(args.reference.read_bytes()).hexdigest(),
        "candidateInputSha256": hashlib.sha256(args.candidate.read_bytes()).hexdigest(),
        "referenceInputName": args.reference.name,
        "candidateInputName": args.candidate.name,
        "width": width,
        "height": height,
        "mismatchedPixels": mismatches,
        "mismatchPercent": round(mismatch_percent, 6),
        "channelTolerance": args.channel_tolerance,
        "meanAbsoluteChannelError": round(mean_channel_error, 6),
        "maximumMismatchPercent": args.maximum_mismatch_percent,
        "maximumMeanChannelError": args.maximum_mean_channel_error,
        "normalization": "pinned Fuse border and active-area tiles mapped to the 448 by 312 raster; application crop scaled with nearest-neighbor",
        "fuseSourceSize": list(FUSE_FRAME),
        "applicationMinimumSourceSize": list(NORMALIZED_FRAME),
        "fuseCapture": reference_path.name,
        "applicationCapture": candidate_path.name,
        "differenceImage": output_image.name,
    }
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, sort_keys=True))
    if result["status"] != "pass":
        raise SystemExit("Fuse visual-equivalence gate failed")


if __name__ == "__main__":
    main()
