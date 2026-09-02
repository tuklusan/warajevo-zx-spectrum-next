/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_HOST_SCALING_H
#define WZ_APP_WZ_HOST_SCALING_H

#include <stddef.h>

#include "app/wz_host_presentation.h"
#include "core/wz_types.h"

typedef struct {
    size_t width;
    size_t height;
    size_t factor;
} wz_host_scale_t;

wz_result_t wz_host_scale_integer(wz_host_scale_t* scale,
                                  const wz_host_presentation_rect_t* source,
                                  size_t factor);
wz_result_t wz_host_scale_source_pixel(const wz_host_scale_t* scale,
                                       size_t output_x, size_t output_y,
                                       size_t* source_x, size_t* source_y);

#endif
