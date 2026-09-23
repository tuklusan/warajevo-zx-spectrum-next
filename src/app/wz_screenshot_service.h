/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_SCREENSHOT_SERVICE_H
#define WZ_APP_WZ_SCREENSHOT_SERVICE_H

#include <stddef.h>

#include "core/wz_raster.h"

/* Encodes only the supplied Spectrum presentation raster; host chrome is absent. */
size_t wz_screenshot_png_required_size(const wz_raster_buffer_t* raster);
wz_result_t wz_screenshot_png_encode(const wz_raster_buffer_t* raster,
                                     wz_byte_t* output, size_t capacity,
                                     size_t* written);

#endif
