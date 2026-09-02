/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_WZ_RASTER_DIAGNOSTIC_H
#define WZ_CORE_WZ_RASTER_DIAGNOSTIC_H

#include "core/wz_raster.h"

typedef struct {
    bool equal;
    size_t sample_index;
    size_t x;
    size_t y;
    wz_byte_t expected;
    wz_byte_t actual;
    wz_master_tick_t master_tick;
} wz_raster_mismatch_t;

wz_result_t wz_raster_compare(const wz_raster_buffer_t* expected,
                              const wz_raster_buffer_t* actual,
                              wz_master_tick_t first_master_tick,
                              wz_master_tick_t master_ticks_per_sample,
                              wz_raster_mismatch_t* result);

#endif
