<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Commercial Use and use for AI/ML model training are
prohibited unless separately authorized. Attribution is required. See
LICENSE for full terms. -->

# Fuse visual-equivalence comparator

`compare.py` compares the pinned Fuse `screen.png` with a production-app
desktop capture. Pass the app display rectangle with `--candidate-region` as
`x y width height`; Fuse's direct PNG is the full reference frame. The
comparator maps the pinned Fuse PNG (320 by 240, including its border) to the
application's 448 by 312 raster. It aligns the 256 by 192 active area without
rescaling its pixels and independently scales the border tiles to their target
extents. The application crop must be at least 448 by 312 pixels, and the
comparison output cannot be reduced below that size, so active-area-only
captures cannot pass. The comparator retains both normalized images and the
pixel-difference image and writes JSON mismatch metrics. The capture workflow
must still select the complete application display rectangle; border and its
orientation are part of the comparison.

The caller must capture the production application's displayed output and the
pinned Fuse display, using the same certified ROM and 48K PAL configuration.
Fuse visual-reference revision
`7edc2418ce11837d9ede799f7e0445440beead1d` adds `--automation-capture-screen`
to produce the headless reference PNG; its build must include PNG support and
the null UI.
The comparator rejects blank captures and undersized active-area crops, but
cannot determine whether a sufficiently large caller-selected application crop
omitted some border. The comparator does not treat the application's
pre-presentation raster as a display capture. The hosted delivery gate passed
all four supported backends in run `38354012417`; the pinned metrics and
capture hashes are recorded in
[`test-results/fuse-visual-equivalence.json`](../../../test-results/fuse-visual-equivalence.json).

Pillow is required. The hosted runner should install it with its system Python
package manager before invoking the comparator.
