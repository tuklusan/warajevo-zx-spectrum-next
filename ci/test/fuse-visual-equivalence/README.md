# Fuse visual-equivalence comparator

`compare.py` compares desktop captures after the runner has identified the
Spectrum-display rectangle in each image. Pass those rectangles with
`--reference-region` and `--candidate-region` as `x y width height`. The
comparator removes host chrome, scales both crops to the 448 by 312 full
Spectrum raster with nearest-neighbor resampling, retains the normalized images
and pixel-difference image, and writes JSON mismatch metrics. Do not crop to
the 256 by 192 active area: the border and its orientation are part of the
comparison.

The caller must capture the production application's displayed output and the
pinned Fuse window, using the same certified ROM and 48K PAL configuration.
The comparator does not treat the application's pre-presentation raster as a
display capture. The workflow gate remains open until capture automation and
passing evidence cover every supported host backend.

Pillow is required. The hosted runner should install it with its system Python
package manager before invoking the comparator.
