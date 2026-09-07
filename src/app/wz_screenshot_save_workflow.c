/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_screenshot_save_workflow.h"

#include <string.h>

static bool copy_destination(char* output, const char* destination)
{
    size_t length;

    if (output == 0 || destination == 0 || destination[0] == '\0') return false;
    length = strlen(destination);
    if (length >= WZ_SCREENSHOT_DESTINATION_CAPACITY) return false;
    (void)memcpy(output, destination, length + 1u);
    return true;
}

void wz_screenshot_save_workflow_init(wz_screenshot_save_workflow_t* workflow)
{
    if (workflow != 0) memset(workflow, 0, sizeof(*workflow));
}

static wz_screenshot_save_result_t encode(
    const wz_screenshot_save_workflow_t* workflow,
    const wz_raster_buffer_t* raster,
    wz_byte_t* output, size_t capacity, size_t* written)
{
    wz_result_t result;

    if (workflow == 0 || raster == 0 || written == 0) {
        return WZ_SCREENSHOT_SAVE_INVALID_ARGUMENT;
    }
    if (!workflow->has_current_destination) {
        *written = 0u;
        return WZ_SCREENSHOT_SAVE_NEEDS_DESTINATION;
    }
    result = wz_screenshot_png_encode(raster, output, capacity, written);
    if (result == WZ_RESULT_OK) return WZ_SCREENSHOT_SAVE_OK;
    if (result == WZ_RESULT_BUFFER_TOO_SMALL) {
        return WZ_SCREENSHOT_SAVE_BUFFER_TOO_SMALL;
    }
    return WZ_SCREENSHOT_SAVE_ENCODE_FAILED;
}

wz_screenshot_save_result_t wz_screenshot_save(
    const wz_screenshot_save_workflow_t* workflow,
    const wz_raster_buffer_t* raster,
    wz_byte_t* output, size_t capacity, size_t* written)
{
    return encode(workflow, raster, output, capacity, written);
}

wz_screenshot_save_result_t wz_screenshot_save_as(
    wz_screenshot_save_workflow_t* workflow,
    const char* destination,
    const wz_raster_buffer_t* raster,
    wz_byte_t* output, size_t capacity, size_t* written)
{
    wz_screenshot_save_workflow_t candidate;
    wz_screenshot_save_result_t result;

    if (workflow == 0 || raster == 0 || written == 0) {
        return WZ_SCREENSHOT_SAVE_INVALID_ARGUMENT;
    }
    if (destination == 0) {
        *written = 0u;
        return WZ_SCREENSHOT_SAVE_CANCELLED;
    }
    candidate = *workflow;
    if (!copy_destination(candidate.current_destination, destination)) {
        *written = 0u;
        return WZ_SCREENSHOT_SAVE_INVALID_DESTINATION;
    }
    candidate.has_current_destination = true;
    result = encode(&candidate, raster, output, capacity, written);
    if (result == WZ_SCREENSHOT_SAVE_OK) *workflow = candidate;
    return result;
}

const char* wz_screenshot_save_current_destination(
    const wz_screenshot_save_workflow_t* workflow)
{
    return workflow != 0 && workflow->has_current_destination ?
        workflow->current_destination : 0;
}
