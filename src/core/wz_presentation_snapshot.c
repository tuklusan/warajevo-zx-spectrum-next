/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_presentation_snapshot.h"

#include <string.h>

static bool raster_shape_valid(const wz_raster_buffer_t* raster)
{
    return raster != 0 && raster->samples != 0 && raster->width != 0u &&
        raster->height != 0u && raster->height <= SIZE_MAX / raster->width;
}

wz_result_t wz_presentation_snapshot_init(
    wz_presentation_snapshot_t* snapshot, size_t width, size_t height,
    wz_byte_t* storage, size_t capacity)
{
    if (snapshot == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    snapshot->raster.samples = 0;
    snapshot->raster.width = 0u;
    snapshot->raster.height = 0u;
    snapshot->sequence = 0u;
    return wz_raster_buffer_init(&snapshot->raster, width, height, storage,
                                 capacity);
}

wz_result_t wz_presentation_snapshot_publish(
    wz_presentation_snapshot_t* snapshot,
    const wz_raster_buffer_t* source)
{
    size_t length;

    if (snapshot == 0 || !raster_shape_valid(&snapshot->raster) ||
        !raster_shape_valid(source) ||
        source->width != snapshot->raster.width ||
        source->height != snapshot->raster.height) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    length = source->width * source->height;
    for (size_t index = 0u; index < length; ++index) {
        if (!wz_raster_sample_is_valid(source->samples[index])) {
            return WZ_RESULT_INVALID_ARGUMENT;
        }
    }
    memcpy(snapshot->raster.samples, source->samples, length);
    snapshot->sequence += 1u;
    return WZ_RESULT_OK;
}

const wz_raster_buffer_t* wz_presentation_snapshot_raster(
    const wz_presentation_snapshot_t* snapshot)
{
    return snapshot == 0 || !raster_shape_valid(&snapshot->raster) ? 0 :
        &snapshot->raster;
}

wz_qword_t wz_presentation_snapshot_sequence(
    const wz_presentation_snapshot_t* snapshot)
{
    return snapshot == 0 ? 0u : snapshot->sequence;
}
