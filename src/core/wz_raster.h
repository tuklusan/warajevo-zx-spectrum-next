Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.

#ifndef WZ_CORE_WZ_RASTER_H
#define WZ_CORE_WZ_RASTER_H

#include "core/wz_types.h"

#define WZ_RASTER_ACTIVE_MIN 0x00u
#define WZ_RASTER_ACTIVE_MAX 0x0fu
#define WZ_RASTER_BORDER_MIN 0x10u
#define WZ_RASTER_BORDER_MAX 0x17u
#define WZ_RASTER_BLANKING 0x18u

static inline bool wz_raster_sample_is_valid(wz_byte_t sample)
{
    return sample <= WZ_RASTER_BLANKING;
}

static inline bool wz_raster_sample_is_active(wz_byte_t sample)
{
    return sample >= WZ_RASTER_ACTIVE_MIN && sample <= WZ_RASTER_ACTIVE_MAX;
}

static inline bool wz_raster_sample_is_border(wz_byte_t sample)
{
    return sample >= WZ_RASTER_BORDER_MIN && sample <= WZ_RASTER_BORDER_MAX;
}

#endif
