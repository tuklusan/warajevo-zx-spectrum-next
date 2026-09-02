/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_raster.h"

static bool wz_raster_buffer_valid(const wz_raster_buffer_t* buffer)
{
    return buffer != 0 && buffer->samples != 0 &&
        buffer->width != 0u && buffer->height != 0u;
}

wz_result_t wz_raster_buffer_init(wz_raster_buffer_t* buffer,
                                   size_t width, size_t height,
                                   wz_byte_t* storage, size_t capacity)
{
    if (buffer == 0 || storage == 0 || width == 0u || height == 0u ||
        height > SIZE_MAX / width || capacity < width * height) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    buffer->samples = storage;
    buffer->width = width;
    buffer->height = height;
    return wz_raster_buffer_clear(buffer, WZ_RASTER_BLANKING);
}

wz_result_t wz_raster_buffer_clear(wz_raster_buffer_t* buffer,
                                   wz_byte_t sample)
{
    size_t length;

    if (!wz_raster_buffer_valid(buffer) || !wz_raster_sample_is_valid(sample)) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    length = buffer->width * buffer->height;
    for (size_t index = 0u; index < length; ++index) {
        buffer->samples[index] = sample;
    }
    return WZ_RESULT_OK;
}

wz_result_t wz_raster_buffer_write(wz_raster_buffer_t* buffer,
                                   size_t x, size_t y, wz_byte_t sample)
{
    if (!wz_raster_buffer_valid(buffer) || x >= buffer->width ||
        y >= buffer->height || !wz_raster_sample_is_valid(sample)) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    buffer->samples[y * buffer->width + x] = sample;
    return WZ_RESULT_OK;
}

wz_result_t wz_raster_buffer_read(const wz_raster_buffer_t* buffer,
                                  size_t x, size_t y, wz_byte_t* sample)
{
    if (!wz_raster_buffer_valid(buffer) || sample == 0 || x >= buffer->width ||
        y >= buffer->height) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    *sample = buffer->samples[y * buffer->width + x];
    return WZ_RESULT_OK;
}

wz_result_t wz_raster_decode_active_pixel(wz_byte_t bitmap,
                                          wz_byte_t attribute,
                                          wz_byte_t bit_position,
                                          wz_byte_t* sample)
{
    if (sample == 0 || bit_position >= 8u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    return wz_raster_decode_attribute(
        attribute,
        (bitmap & (wz_byte_t)(0x80u >> bit_position)) != 0u,
        sample);
}

wz_result_t wz_raster_decode_attribute(wz_byte_t attribute, bool ink_selected,
                                       wz_byte_t* sample)
{
    wz_byte_t color = ink_selected ? attribute & 0x07u :
        (attribute >> 3u) & 0x07u;

    if (sample == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if ((attribute & 0x40u) != 0u) {
        color = (wz_byte_t)(color + 8u);
    }
    *sample = color;
    return WZ_RESULT_OK;
}
