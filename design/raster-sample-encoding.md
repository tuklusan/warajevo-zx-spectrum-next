Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.

# Canonical Raster Sample Encoding

The deterministic core exposes one logical byte per native raster-clock sample.
The encoding is independent of host RGB formats, GPU textures, scaling,
shaders, window composition, and screenshot containers.

| Value | Meaning |
| --- | --- |
| `0x00`-`0x0F` | Spectrum logical palette index; `0x00`-`0x07` normal and `0x08`-`0x0F` bright active-pixel colors |
| `0x10`-`0x17` | Border color index, using only the 3-bit border value |
| `0x18` | Blanking/synchronization sample |
| `0x19`-`0xFF` | Reserved; producers must not emit these values in the initial contract |

The initial native raster dimensions come from the machine profile. For 48K
PAL they are 448 raster clocks by 312 lines. A sample has no implicit host
endianness or RGB interpretation. Palette conversion belongs after this
encoding and cannot alter machine state or canonical hashes.

Consumers must treat reserved values as invalid input and fail in a controlled
way. This contract is frozen before the raster buffer implementation; later
tickets may add a versioned encoding only through an explicit architecture
change.
