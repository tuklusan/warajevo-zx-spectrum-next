/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_printer_export.h"

#include <string.h>

wz_result_t wz_printer_export_bitmap(const wz_printer_flush_event_t* event,
                                     wz_byte_t* output,
                                     size_t capacity,
                                     wz_printer_bitmap_t* bitmap)
{
    size_t width;
    size_t height;
    size_t x;
    size_t y;
    if (event == NULL || output == NULL || bitmap == NULL || event->length == 0u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (event->mode == WZ_PRINTER_MODE_EPSON ||
        event->mode == WZ_PRINTER_MODE_EPSON_ENLARGED) {
        width = event->length;
        height = 8u;
        if (width > SIZE_MAX / height || width * height > capacity) {
            return WZ_RESULT_BUFFER_TOO_SMALL;
        }
        for (x = 0u; x < width; ++x) {
            for (y = 0u; y < height; ++y) {
                output[y * width + x] =
                    (event->data[x] & (wz_byte_t)(1u << y)) != 0u ? 0xffu : 0u;
            }
        }
    } else if (event->mode == WZ_PRINTER_MODE_HP ||
               event->mode == WZ_PRINTER_MODE_HP_ENLARGED) {
        width = 8u;
        height = 1u;
        if (capacity < width) return WZ_RESULT_BUFFER_TOO_SMALL;
        for (x = 0u; x < width; ++x) {
            output[x] = (event->data[0] & (wz_byte_t)(0x80u >> x)) != 0u ?
                0xffu : 0u;
        }
    } else {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    bitmap->width = width;
    bitmap->height = height;
    bitmap->stride = width;
    bitmap->bytes = width * height;
    return WZ_RESULT_OK;
}
