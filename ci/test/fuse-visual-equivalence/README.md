<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Commercial Use and use for AI/ML model training are
prohibited unless separately authorized. Attribution is required. See
LICENSE for full terms. -->

# Fuse visual-equivalence comparator

`compare.py` compares desktop captures after the runner has identified the
Spectrum-display rectangle in each image. Pass those rectangles with
`--reference-region` and `--candidate-region` as `x y width height`. The
comparator removes host chrome, scales both crops to the 448 by 312 full
Spectrum raster with nearest-neighbor resampling, retains the normalized images
and pixel-difference image, and writes JSON mismatch metrics. The capture
workflow must select the complete display rectangle; do not crop to the 256 by
192 active area because the border and its orientation are part of the
comparison. Fuse and the application may use different display pixel aspect
ratios, so normalization scales each captured rectangle independently.

The caller must capture the production application's displayed output and the
pinned Fuse display, using the same certified ROM and 48K PAL configuration.
Fuse visual-reference revision
`7edc2418ce11837d9ede799f7e0445440beead1d` adds `--automation-capture-screen`
to produce the headless reference PNG; its build must include PNG support and
the null UI.
The comparator rejects blank captures but cannot determine whether a caller's
crop omitted the border. The comparator does not treat the application's
pre-presentation raster as a display capture. The workflow gate remains open
until capture automation and passing evidence cover every supported host
backend.

Pillow is required. The hosted runner should install it with its system Python
package manager before invoking the comparator.
