/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_WZ_RASTER_H
#define WZ_CORE_WZ_RASTER_H

#include <stddef.h>

#include "core/wz_types.h"

#define WZ_RASTER_ACTIVE_MIN 0x00u
#define WZ_RASTER_ACTIVE_MAX 0x0fu
#define WZ_RASTER_BORDER_MIN 0x10u
#define WZ_RASTER_BORDER_MAX 0x17u
#define WZ_RASTER_BLANKING 0x18u
#define WZ_RASTER_CANONICAL_WIDTH 448u
#define WZ_RASTER_CANONICAL_HEIGHT 312u

enum {
    WZ_PALETTE_BLACK = 0u,
    WZ_PALETTE_BLUE = 1u,
    WZ_PALETTE_RED = 2u,
    WZ_PALETTE_MAGENTA = 3u,
    WZ_PALETTE_GREEN = 4u,
    WZ_PALETTE_CYAN = 5u,
    WZ_PALETTE_YELLOW = 6u,
    WZ_PALETTE_WHITE = 7u,
    WZ_PALETTE_BRIGHT_BLACK = 8u,
    WZ_PALETTE_BRIGHT_BLUE = 9u,
    WZ_PALETTE_BRIGHT_RED = 10u,
    WZ_PALETTE_BRIGHT_MAGENTA = 11u,
    WZ_PALETTE_BRIGHT_GREEN = 12u,
    WZ_PALETTE_BRIGHT_CYAN = 13u,
    WZ_PALETTE_BRIGHT_YELLOW = 14u,
    WZ_PALETTE_BRIGHT_WHITE = 15u
};

typedef struct {
    wz_byte_t* samples;
    size_t width;
    size_t height;
} wz_raster_buffer_t;

wz_result_t wz_raster_buffer_init(wz_raster_buffer_t* buffer,
                                   size_t width, size_t height,
                                   wz_byte_t* storage, size_t capacity);
wz_result_t wz_raster_buffer_clear(wz_raster_buffer_t* buffer,
                                   wz_byte_t sample);
wz_result_t wz_raster_buffer_write(wz_raster_buffer_t* buffer,
                                   size_t x, size_t y, wz_byte_t sample);
wz_result_t wz_raster_buffer_read(const wz_raster_buffer_t* buffer,
                                  size_t x, size_t y, wz_byte_t* sample);
wz_result_t wz_raster_decode_active_pixel(wz_byte_t bitmap,
                                          wz_byte_t attribute,
                                          wz_byte_t bit_position,
                                          wz_byte_t* sample);
wz_result_t wz_raster_decode_attribute(wz_byte_t attribute, bool ink_selected,
                                       wz_byte_t* sample);
wz_result_t wz_raster_decode_attribute_phase(wz_byte_t attribute,
                                             bool ink_selected,
                                             bool flash_phase,
                                             wz_byte_t* sample);
wz_result_t wz_raster_palette_index(wz_byte_t base_color, bool bright,
                                    wz_byte_t* sample);

static inline bool wz_raster_sample_is_valid(wz_byte_t sample)
{
    return sample <= WZ_RASTER_BLANKING;
}

static inline bool wz_raster_sample_is_active(wz_byte_t sample)
{
    return sample <= WZ_RASTER_ACTIVE_MAX;
}

static inline bool wz_raster_sample_is_border(wz_byte_t sample)
{
    return sample >= WZ_RASTER_BORDER_MIN && sample <= WZ_RASTER_BORDER_MAX;
}

#endif
