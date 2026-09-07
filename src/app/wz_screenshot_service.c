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
#define WZ_PNG_MAX_IDAT_BLOCK 65535u
#define WZ_PNG_PALETTE_ENTRIES 25u

static bool add_size(size_t left, size_t right, size_t* result)
{
    if (right > (size_t)-1 - left) return false;
    *result = left + right;
    return true;
}

static bool mul_size(size_t left, size_t right, size_t* result)
{
    if (left != 0u && right > (size_t)-1 / left) return false;
    *result = left * right;
    return true;
}

static size_t deflate_block_count(size_t raw_size)
{
    return raw_size == 0u ? 1u :
        raw_size / WZ_PNG_MAX_IDAT_BLOCK +
        (raw_size % WZ_PNG_MAX_IDAT_BLOCK != 0u ? 1u : 0u);
}

static size_t png_required_for_raw(size_t raw_size)
{
    size_t blocks = deflate_block_count(raw_size);
    size_t deflate_size;
    size_t total;

    if (!mul_size(blocks, 5u, &deflate_size) ||
        !add_size(deflate_size, raw_size, &deflate_size) ||
        !add_size(deflate_size, 6u, &deflate_size) ||
        !add_size(8u + 25u + 87u + 12u + 12u, deflate_size, &total)) {
        return 0u;
    }
    return total;
}

size_t wz_screenshot_png_required_size(const wz_raster_buffer_t* raster)
{
    size_t row_size;
    size_t raw_size;

    if (raster == 0 || raster->samples == 0u || raster->width == 0u ||
        raster->height == 0u || !add_size(raster->width, 1u, &row_size) ||
        !mul_size(row_size, raster->height, &raw_size)) return 0u;
    return png_required_for_raw(raw_size);
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
            crc = (crc >> 1u) ^ (0xedb88320u & (wz_dword_t)-(wz_dword_t)(crc & 1u));
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
    output[(*offset)++] = (wz_byte_t)type[0];
    output[(*offset)++] = (wz_byte_t)type[1];
    output[(*offset)++] = (wz_byte_t)type[2];
    output[(*offset)++] = (wz_byte_t)type[3];
    for (size_t index = 0u; index < length; ++index) output[(*offset)++] = data[index];
    put_u32(output, offset, crc32(output + type_offset, 4u + length));
}

static void palette(wz_byte_t* output, size_t* offset)
{
    static const wz_byte_t colors[16u][3u] = {
        {0u, 0u, 0u}, {0u, 0u, 192u}, {192u, 0u, 0u}, {192u, 0u, 192u},
        {0u, 192u, 0u}, {0u, 192u, 192u}, {192u, 192u, 0u}, {192u, 192u, 192u},
        {0u, 0u, 0u}, {0u, 0u, 255u}, {255u, 0u, 0u}, {255u, 0u, 255u},
        {0u, 255u, 0u}, {0u, 255u, 255u}, {255u, 255u, 0u}, {255u, 255u, 255u}
    };
    wz_byte_t entries[WZ_PNG_PALETTE_ENTRIES * 3u];
    for (size_t index = 0u; index < 16u; ++index) {
        entries[index * 3u] = colors[index][0];
        entries[index * 3u + 1u] = colors[index][1];
        entries[index * 3u + 2u] = colors[index][2];
    }
    for (size_t index = 0u; index < 8u; ++index) {
        entries[(16u + index) * 3u] = colors[index][0];
        entries[(16u + index) * 3u + 1u] = colors[index][1];
        entries[(16u + index) * 3u + 2u] = colors[index][2];
    }
    entries[72u] = 0u;
    entries[73u] = 0u;
    entries[74u] = 0u;
    chunk(output, offset, "PLTE", entries, sizeof(entries));
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
    size_t required;
    size_t raw_size;
    size_t row_size;
    size_t offset = 0u;
    size_t raw_offset = 0u;
    size_t remaining;
    wz_dword_t adler_a = 1u;
    wz_dword_t adler_b = 0u;

    if (written == 0u) return WZ_RESULT_INVALID_ARGUMENT;
    *written = 0u;
    required = wz_screenshot_png_required_size(raster);
    if (required == 0u || output == 0u) return WZ_RESULT_INVALID_ARGUMENT;
    if (capacity < required) {
        *written = required;
        return WZ_RESULT_BUFFER_TOO_SMALL;
    }
    row_size = raster->width + 1u;
    if (!mul_size(row_size, raster->height, &raw_size)) return WZ_RESULT_INVALID_ARGUMENT;
    for (size_t index = 0u; index < raster->width * raster->height; ++index) {
        if (!wz_raster_sample_is_valid(raster->samples[index])) return WZ_RESULT_INVALID_ARGUMENT;
    }

    static const wz_byte_t signature[WZ_PNG_SIGNATURE_SIZE] =
        {0x89u, 0x50u, 0x4eu, 0x47u, 0x0du, 0x0au, 0x1au, 0x0au};
    for (size_t index = 0u; index < sizeof(signature); ++index) output[offset++] = signature[index];
    {
        wz_byte_t ihdr[13u] = {0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 8u, 3u, 0u, 0u, 0u};
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
    palette(output, &offset);
    put_u32(output, &offset, (wz_dword_t)(raw_size + 6u + deflate_block_count(raw_size) * 5u));
    {
        size_t type_offset = offset;
        output[offset++] = 'I'; output[offset++] = 'D'; output[offset++] = 'A'; output[offset++] = 'T';
        output[offset++] = 0x78u; output[offset++] = 0x01u;
        remaining = raw_size;
        while (remaining != 0u) {
            size_t block_size = remaining > WZ_PNG_MAX_IDAT_BLOCK ? WZ_PNG_MAX_IDAT_BLOCK : remaining;
            bool final = block_size == remaining;
            output[offset++] = final ? 1u : 0u;
            output[offset++] = (wz_byte_t)block_size;
            output[offset++] = (wz_byte_t)(block_size >> 8u);
            output[offset++] = (wz_byte_t)~(wz_byte_t)block_size;
            output[offset++] = (wz_byte_t)~(wz_byte_t)(block_size >> 8u);
            for (size_t index = 0u; index < block_size; ++index) {
                size_t source = raw_offset++;
                wz_byte_t value = source % row_size == 0u ? 0u :
                    raster->samples[(source / row_size) * raster->width + source % row_size - 1u];
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
