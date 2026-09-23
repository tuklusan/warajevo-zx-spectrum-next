/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_host_scaling.h"

static bool wz_host_rect_valid(const wz_host_presentation_rect_t* source)
{
    return source != 0 && source->width != 0u && source->height != 0u;
}

void wz_host_display_settings_init(wz_host_display_settings_t* settings)
{
    if (settings == 0) {
        return;
    }
    settings->integer_scale = 1u;
    settings->crop_to_display = false;
}

wz_result_t wz_host_display_settings_set_scale(
    wz_host_display_settings_t* settings, size_t integer_scale)
{
    if (settings == 0 || integer_scale == 0u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    settings->integer_scale = integer_scale;
    return WZ_RESULT_OK;
}

wz_result_t wz_host_display_settings_set_crop(
    wz_host_display_settings_t* settings, bool crop_to_display)
{
    if (settings == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    settings->crop_to_display = crop_to_display;
    return WZ_RESULT_OK;
}

size_t wz_host_display_settings_scale(
    const wz_host_display_settings_t* settings)
{
    return settings == 0 ? 0u : settings->integer_scale;
}

bool wz_host_display_settings_crop(
    const wz_host_display_settings_t* settings)
{
    return settings != 0 && settings->crop_to_display;
}

wz_result_t wz_host_scale_integer(wz_host_scale_t* scale,
                                  const wz_host_presentation_rect_t* source,
                                  size_t factor)
{
    if (scale == 0 || !wz_host_rect_valid(source) || factor == 0u ||
        source->width > SIZE_MAX / factor || source->height > SIZE_MAX / factor) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    scale->width = source->width * factor;
    scale->height = source->height * factor;
    scale->factor = factor;
    return WZ_RESULT_OK;
}

wz_result_t wz_host_scale_source_pixel(const wz_host_scale_t* scale,
                                       size_t output_x, size_t output_y,
                                       size_t* source_x, size_t* source_y)
{
    if (scale == 0 || scale->factor == 0u || source_x == 0 || source_y == 0 ||
        output_x >= scale->width || output_y >= scale->height) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    *source_x = output_x / scale->factor;
    *source_y = output_y / scale->factor;
    return WZ_RESULT_OK;
}
