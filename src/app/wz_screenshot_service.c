/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_screenshot_service.h"

#include <stdbool.h>

#define WZ_PNG_SIGNATURE_SIZE 8u
#define WZ_PNG_MAX_DEFLATE_BLOCK 65535u

static bool add_size(size_t left, size_t right, size_t* result)
{
    if (right > (size_t)-1 - left) return false;
    *result = left + right;
    return true;
}

static bool multiply_size(size_t left, size_t right, size_t* result)
{
    if (left != 0u && right > (size_t)-1 / left) return false;
    *result = left * right;
    return true;
}

static size_t deflate_block_count(size_t raw_bytes)
{
    return raw_bytes == 0u ? 1u :
        raw_bytes / WZ_PNG_MAX_DEFLATE_BLOCK +
        (raw_bytes % WZ_PNG_MAX_DEFLATE_BLOCK != 0u ? 1u : 0u);
}

static bool raster_dimensions(const wz_raster_buffer_t* raster,
                              size_t* scanline_bytes, size_t* raw_bytes)
{
    size_t pixels_per_row;
    if (raster == 0 || raster->samples == 0u || raster->width == 0u ||
        raster->height == 0u || raster->width > 0xffffffffu ||
        raster->height > 0xffffffffu ||
        !multiply_size(raster->width, 3u, &pixels_per_row) ||
        !add_size(pixels_per_row, 1u, scanline_bytes) ||
        !multiply_size(*scanline_bytes, raster->height, raw_bytes) ||
        *raw_bytes > 0xffffffffu) return false;
    return true;
}

static bool sample_rgb(wz_byte_t sample, wz_byte_t* red,
                       wz_byte_t* green, wz_byte_t* blue)
{
    static const wz_byte_t colors[16u][3u] = {
        {0u, 0u, 0u}, {0u, 0u, 192u}, {192u, 0u, 0u}, {192u, 0u, 192u},
        {0u, 192u, 0u}, {0u, 192u, 192u}, {192u, 192u, 0u}, {192u, 192u, 192u},
        {0u, 0u, 0u}, {0u, 0u, 255u}, {255u, 0u, 0u}, {255u, 0u, 255u},
        {0u, 255u, 0u}, {0u, 255u, 255u}, {255u, 255u, 0u}, {255u, 255u, 255u}
    };
    size_t index;
    if (!wz_raster_sample_is_valid(sample) || sample == WZ_RASTER_BLANKING) {
        index = 0u;
    } else if (wz_raster_sample_is_border(sample)) {
        index = (size_t)(sample - WZ_RASTER_BORDER_MIN);
    } else {
        index = sample;
    }
    if (red == 0u || green == 0u || blue == 0u) return false;
    *red = colors[index][0];
    *green = colors[index][1];
    *blue = colors[index][2];
    return true;
}

static bool raster_samples_valid(const wz_raster_buffer_t* raster)
{
    size_t pixel_count;
    if (!multiply_size(raster->width, raster->height, &pixel_count)) return false;
    for (size_t index = 0u; index < pixel_count; ++index) {
        if (!wz_raster_sample_is_valid(raster->samples[index])) return false;
    }
    return true;
}

static size_t required_for_raw(size_t raw_bytes)
{
    size_t deflate_bytes;
    size_t total;
    if (!multiply_size(deflate_block_count(raw_bytes), 5u, &deflate_bytes) ||
        !add_size(deflate_bytes, raw_bytes, &deflate_bytes) ||
        !add_size(deflate_bytes, 6u, &deflate_bytes) ||
        !add_size(8u + 25u + 12u + 12u, deflate_bytes, &total)) return 0u;
    return total;
}

size_t wz_screenshot_png_required_size(const wz_raster_buffer_t* raster)
{
    size_t scanline_bytes;
    size_t raw_bytes;
    if (!raster_dimensions(raster, &scanline_bytes, &raw_bytes)) return 0u;
    (void)scanline_bytes;
    return required_for_raw(raw_bytes);
}

static void put_u32(wz_byte_t* output, size_t* offset, wz_dword_t value)
{
    output[(*offset)++] = (wz_byte_t)(value >> 24u);
    output[(*offset)++] = (wz_byte_t)(value >> 16u);
    output[(*offset)++] = (wz_byte_t)(value >> 8u);
    output[(*offset)++] = (wz_byte_t)value;
}

static wz_dword_t crc32(const wz_byte_t* data, size_t length)
{
    wz_dword_t crc = 0xffffffffu;
    for (size_t index = 0u; index < length; ++index) {
        crc ^= data[index];
        for (unsigned bit = 0u; bit < 8u; ++bit) {
            crc = (crc >> 1u) ^
                (0xedb88320u & (wz_dword_t)-(wz_dword_t)(crc & 1u));
        }
    }
    return ~crc;
}

static void chunk(wz_byte_t* output, size_t* offset, const char type[4],
                  const wz_byte_t* data, size_t length)
{
    size_t type_offset;
    put_u32(output, offset, (wz_dword_t)length);
    type_offset = *offset;
    for (size_t index = 0u; index < 4u; ++index) output[(*offset)++] = (wz_byte_t)type[index];
    for (size_t index = 0u; index < length; ++index) output[(*offset)++] = data[index];
    put_u32(output, offset, crc32(output + type_offset, 4u + length));
}

static void adler_update(wz_dword_t* a, wz_dword_t* b, wz_byte_t value)
{
    *a = (*a + value) % 65521u;
    *b = (*b + *a) % 65521u;
}

wz_result_t wz_screenshot_png_encode(const wz_raster_buffer_t* raster,
                                     wz_byte_t* output, size_t capacity,
                                     size_t* written)
{
    size_t scanline_bytes;
    size_t raw_bytes;
    size_t required;
    size_t offset = 0u;
    size_t raw_offset = 0u;
    size_t remaining;
    wz_dword_t adler_a = 1u;
    wz_dword_t adler_b = 0u;
    static const wz_byte_t signature[WZ_PNG_SIGNATURE_SIZE] =
        {0x89u, 0x50u, 0x4eu, 0x47u, 0x0du, 0x0au, 0x1au, 0x0au};

    if (written == 0u) return WZ_RESULT_INVALID_ARGUMENT;
    *written = 0u;
    if (!raster_dimensions(raster, &scanline_bytes, &raw_bytes) ||
        !raster_samples_valid(raster)) return WZ_RESULT_INVALID_ARGUMENT;
    required = required_for_raw(raw_bytes);
    if (required == 0u || output == 0u) return WZ_RESULT_INVALID_ARGUMENT;
    if (capacity < required) {
        *written = required;
        return WZ_RESULT_BUFFER_TOO_SMALL;
    }
    for (size_t index = 0u; index < sizeof(signature); ++index) output[offset++] = signature[index];
    {
        wz_byte_t ihdr[13u] = {0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 8u, 2u, 0u, 0u, 0u};
        ihdr[0] = (wz_byte_t)(raster->width >> 24u);
        ihdr[1] = (wz_byte_t)(raster->width >> 16u);
        ihdr[2] = (wz_byte_t)(raster->width >> 8u);
        ihdr[3] = (wz_byte_t)raster->width;
        ihdr[4] = (wz_byte_t)(raster->height >> 24u);
        ihdr[5] = (wz_byte_t)(raster->height >> 16u);
        ihdr[6] = (wz_byte_t)(raster->height >> 8u);
        ihdr[7] = (wz_byte_t)raster->height;
        chunk(output, &offset, "IHDR", ihdr, sizeof(ihdr));
    }
    put_u32(output, &offset, (wz_dword_t)(raw_bytes + 6u + deflate_block_count(raw_bytes) * 5u));
    {
        size_t type_offset = offset;
        output[offset++] = 'I'; output[offset++] = 'D'; output[offset++] = 'A'; output[offset++] = 'T';
        output[offset++] = 0x78u; output[offset++] = 0x01u;
        remaining = raw_bytes;
        while (remaining != 0u) {
            size_t block_size = remaining > WZ_PNG_MAX_DEFLATE_BLOCK ?
                WZ_PNG_MAX_DEFLATE_BLOCK : remaining;
            bool final = block_size == remaining;
            output[offset++] = final ? 1u : 0u;
            output[offset++] = (wz_byte_t)block_size;
            output[offset++] = (wz_byte_t)(block_size >> 8u);
            output[offset++] = (wz_byte_t)~(wz_byte_t)block_size;
            output[offset++] = (wz_byte_t)~(wz_byte_t)(block_size >> 8u);
            for (size_t index = 0u; index < block_size; ++index) {
                size_t row_offset = raw_offset++;
                wz_byte_t value;
                if (row_offset % scanline_bytes == 0u) {
                    value = 0u;
                } else {
                    size_t pixel_offset = row_offset % scanline_bytes - 1u;
                    size_t pixel_index = (row_offset / scanline_bytes) * raster->width +
                        pixel_offset / 3u;
                    wz_byte_t red;
                    wz_byte_t green;
                    wz_byte_t blue;
                    (void)sample_rgb(raster->samples[pixel_index], &red, &green, &blue);
                    value = pixel_offset % 3u == 0u ? red :
                        (pixel_offset % 3u == 1u ? green : blue);
                }
                output[offset++] = value;
                adler_update(&adler_a, &adler_b, value);
            }
            remaining -= block_size;
        }
        output[offset++] = (wz_byte_t)(adler_b >> 8u);
        output[offset++] = (wz_byte_t)adler_b;
        output[offset++] = (wz_byte_t)(adler_a >> 8u);
        output[offset++] = (wz_byte_t)adler_a;
        put_u32(output, &offset, crc32(output + type_offset, offset - type_offset));
    }
    chunk(output, &offset, "IEND", 0, 0u);
    *written = offset;
    return WZ_RESULT_OK;
}
