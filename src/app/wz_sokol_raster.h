/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_SOKOL_RASTER_H
#define WZ_APP_WZ_SOKOL_RASTER_H

#include <stdbool.h>

#include "core/wz_raster.h"
#include "sokol_gfx.h"

typedef struct {
    sg_image image;
    size_t width;
    size_t height;
    bool initialized;
} wz_sokol_raster_t;

wz_result_t wz_sokol_raster_init(wz_sokol_raster_t* target,
                                  const wz_raster_buffer_t* source);
wz_result_t wz_sokol_raster_update(wz_sokol_raster_t* target,
                                    const wz_raster_buffer_t* source);
void wz_sokol_raster_destroy(wz_sokol_raster_t* target);

#endif
