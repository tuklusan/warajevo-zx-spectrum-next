/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_sokol_raster.h"

static bool wz_sokol_raster_source_valid(const wz_raster_buffer_t* source)
{
    return source != 0 && source->samples != 0 && source->width != 0u &&
        source->height != 0u && source->height <= SIZE_MAX / source->width;
}

wz_result_t wz_sokol_raster_init(wz_sokol_raster_t* target,
                                  const wz_raster_buffer_t* source)
{
    if (target == 0 || !wz_sokol_raster_source_valid(source)) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    target->image = sg_make_image(&(sg_image_desc){
        .width = (int)source->width,
        .height = (int)source->height,
        .pixel_format = SG_PIXELFORMAT_R8,
        .data.subimage[0][0] = {
            .ptr = source->samples,
            .size = source->width * source->height,
        },
    });
    if (!sg_image_exists(target->image)) {
        return WZ_RESULT_INVALID_STATE;
    }
    target->width = source->width;
    target->height = source->height;
    target->initialized = true;
    return WZ_RESULT_OK;
}

wz_result_t wz_sokol_raster_update(wz_sokol_raster_t* target,
                                    const wz_raster_buffer_t* source)
{
    if (target == 0 || !target->initialized ||
        !wz_sokol_raster_source_valid(source) || source->width != target->width ||
        source->height != target->height) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    sg_update_image(target->image, &(sg_image_data){
        .subimage[0][0] = {
            .ptr = source->samples,
            .size = source->width * source->height,
        },
    });
    return WZ_RESULT_OK;
}

void wz_sokol_raster_destroy(wz_sokol_raster_t* target)
{
    if (target == 0 || !target->initialized) {
        return;
    }
    sg_destroy_image(target->image);
    target->image.id = SG_INVALID_ID;
    target->width = 0u;
    target->height = 0u;
    target->initialized = false;
}
