/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_WZ_PRESENTATION_SNAPSHOT_H
#define WZ_CORE_WZ_PRESENTATION_SNAPSHOT_H

#include "core/wz_raster.h"

typedef struct {
    wz_raster_buffer_t raster;
    wz_qword_t sequence;
} wz_presentation_snapshot_t;

wz_result_t wz_presentation_snapshot_init(
    wz_presentation_snapshot_t* snapshot, size_t width, size_t height,
    wz_byte_t* storage, size_t capacity);
wz_result_t wz_presentation_snapshot_publish(
    wz_presentation_snapshot_t* snapshot,
    const wz_raster_buffer_t* source);
const wz_raster_buffer_t* wz_presentation_snapshot_raster(
    const wz_presentation_snapshot_t* snapshot);
wz_qword_t wz_presentation_snapshot_sequence(
    const wz_presentation_snapshot_t* snapshot);

#endif
