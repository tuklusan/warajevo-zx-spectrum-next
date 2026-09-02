/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_raster_diagnostic.h"

wz_result_t wz_raster_compare(const wz_raster_buffer_t* expected,
                              const wz_raster_buffer_t* actual,
                              wz_master_tick_t first_master_tick,
                              wz_master_tick_t master_ticks_per_sample,
                              wz_raster_mismatch_t* result)
{
    size_t length;
    size_t index;

    if (expected == NULL || actual == NULL || result == NULL ||
        expected->width != actual->width || expected->height != actual->height ||
        (expected->samples == NULL && expected->width * expected->height != 0u) ||
        (actual->samples == NULL && actual->width * actual->height != 0u)) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    length = expected->width * expected->height;
    result->equal = true;
    result->sample_index = length;
    result->x = expected->width;
    result->y = expected->height;
    result->expected = 0u;
    result->actual = 0u;
    result->master_tick = first_master_tick;
    for (index = 0u; index < length; ++index) {
        if (expected->samples[index] != actual->samples[index]) {
            result->equal = false;
            result->sample_index = index;
            result->x = index % expected->width;
            result->y = index / expected->width;
            result->expected = expected->samples[index];
            result->actual = actual->samples[index];
            result->master_tick = first_master_tick +
                                   ((wz_master_tick_t)index * master_ticks_per_sample);
            break;
        }
    }
    return WZ_RESULT_OK;
}
