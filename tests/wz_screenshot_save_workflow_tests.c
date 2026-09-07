/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <assert.h>
#include <string.h>

#include "app/wz_screenshot_save_workflow.h"

int main(void)
{
    wz_byte_t samples[4u];
    wz_byte_t original[4u];
    wz_byte_t png[512u];
    wz_raster_buffer_t raster;
    wz_screenshot_save_workflow_t workflow;
    size_t required;
    size_t written = 0u;

    wz_screenshot_save_workflow_init(&workflow);
    assert(wz_raster_buffer_init(&raster, 2u, 2u, samples, sizeof(samples)) == WZ_RESULT_OK);
    samples[0] = WZ_PALETTE_BLACK;
    samples[1] = WZ_PALETTE_WHITE;
    samples[2] = WZ_RASTER_BORDER_MIN;
    samples[3] = WZ_RASTER_BLANKING;
    memcpy(original, samples, sizeof(samples));
    required = wz_screenshot_png_required_size(&raster);

    assert(wz_screenshot_save(&workflow, &raster, png, sizeof(png), &written) ==
           WZ_SCREENSHOT_SAVE_NEEDS_DESTINATION);
    assert(written == 0u && wz_screenshot_save_current_destination(&workflow) == 0);
    assert(wz_screenshot_save_as(&workflow, 0, &raster, png, sizeof(png), &written) ==
           WZ_SCREENSHOT_SAVE_CANCELLED);
    assert(wz_screenshot_save_current_destination(&workflow) == 0 &&
           memcmp(samples, original, sizeof(samples)) == 0);
    assert(wz_screenshot_save_as(&workflow, "", &raster, png, sizeof(png), &written) ==
           WZ_SCREENSHOT_SAVE_INVALID_DESTINATION);
    assert(wz_screenshot_save_current_destination(&workflow) == 0 &&
           memcmp(samples, original, sizeof(samples)) == 0);
    assert(wz_screenshot_save_as(&workflow, "screen.png", &raster, png, required - 1u,
                                 &written) == WZ_SCREENSHOT_SAVE_BUFFER_TOO_SMALL);
    assert(written == required &&
           wz_screenshot_save_current_destination(&workflow) == 0 &&
           memcmp(samples, original, sizeof(samples)) == 0);
    assert(wz_screenshot_save_as(&workflow, "screen.png", &raster, png, sizeof(png),
                                 &written) == WZ_SCREENSHOT_SAVE_OK);
    assert(written == required);
    assert(strcmp(wz_screenshot_save_current_destination(&workflow), "screen.png") == 0);
    assert(memcmp(png, "\x89PNG\r\n\x1a\n", 8u) == 0);
    assert(memcmp(samples, original, sizeof(samples)) == 0);
    return 0;
}
