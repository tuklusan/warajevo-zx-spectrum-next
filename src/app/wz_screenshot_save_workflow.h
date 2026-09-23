/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_SCREENSHOT_SAVE_WORKFLOW_H
#define WZ_APP_WZ_SCREENSHOT_SAVE_WORKFLOW_H

#include <stdbool.h>
#include <stddef.h>

#include "app/wz_screenshot_service.h"

#define WZ_SCREENSHOT_DESTINATION_CAPACITY 1024u

typedef enum {
    WZ_SCREENSHOT_SAVE_OK = 0,
    WZ_SCREENSHOT_SAVE_INVALID_ARGUMENT,
    WZ_SCREENSHOT_SAVE_NEEDS_DESTINATION,
    WZ_SCREENSHOT_SAVE_CANCELLED,
    WZ_SCREENSHOT_SAVE_INVALID_DESTINATION,
    WZ_SCREENSHOT_SAVE_BUFFER_TOO_SMALL,
    WZ_SCREENSHOT_SAVE_ENCODE_FAILED
} wz_screenshot_save_result_t;

typedef struct {
    char current_destination[WZ_SCREENSHOT_DESTINATION_CAPACITY];
    bool has_current_destination;
} wz_screenshot_save_workflow_t;

void wz_screenshot_save_workflow_init(wz_screenshot_save_workflow_t* workflow);
wz_screenshot_save_result_t wz_screenshot_save(
    const wz_screenshot_save_workflow_t* workflow,
    const wz_raster_buffer_t* raster,
    wz_byte_t* output, size_t capacity, size_t* written);
wz_screenshot_save_result_t wz_screenshot_save_as(
    wz_screenshot_save_workflow_t* workflow,
    const char* destination,
    const wz_raster_buffer_t* raster,
    wz_byte_t* output, size_t capacity, size_t* written);
const char* wz_screenshot_save_current_destination(
    const wz_screenshot_save_workflow_t* workflow);

#endif
