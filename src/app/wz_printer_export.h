/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_PRINTER_EXPORT_H
#define WZ_APP_WZ_PRINTER_EXPORT_H

#include <stddef.h>

#include "core/wz_printer.h"

typedef struct {
    size_t width;
    size_t height;
    size_t stride;
    size_t bytes;
} wz_printer_bitmap_t;

wz_result_t wz_printer_export_bitmap(const wz_printer_flush_event_t* event,
                                     wz_byte_t* output,
                                     size_t capacity,
                                     wz_printer_bitmap_t* bitmap);

#endif
