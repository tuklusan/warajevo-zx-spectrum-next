/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <assert.h>
#include <string.h>

#include "app/wz_screenshot_service.h"

int main(void)
{
    wz_byte_t samples[4u] = {WZ_PALETTE_BLACK, WZ_PALETTE_WHITE, 0x10u, WZ_RASTER_BLANKING};
    wz_byte_t original[4u];
    wz_raster_buffer_t raster;
    wz_byte_t png[512u];
    size_t required;
    size_t written = 0u;

    memcpy(original, samples, sizeof(samples));
    assert(wz_raster_buffer_init(&raster, 2u, 2u, samples, sizeof(samples)) == WZ_RESULT_OK);
    required = wz_screenshot_png_required_size(&raster);
    assert(required > 0u && required < sizeof(png));
    assert(wz_screenshot_png_encode(&raster, png, required - 1u, &written) ==
           WZ_RESULT_BUFFER_TOO_SMALL);
    assert(written == required);
    assert(wz_screenshot_png_encode(&raster, png, sizeof(png), &written) == WZ_RESULT_OK);
    assert(written == required);
    assert(memcmp(png, "\x89PNG\r\n\x1a\n", 8u) == 0);
    assert(memcmp(samples, original, sizeof(samples)) == 0);

    samples[0] = (wz_byte_t)(WZ_RASTER_BLANKING + 1u);
    memset(png, 0xa5, sizeof(png));
    assert(wz_screenshot_png_encode(&raster, png, sizeof(png), &written) ==
           WZ_RESULT_INVALID_ARGUMENT);
    assert(written == 0u);
    for (size_t index = 0u; index < sizeof(png); ++index) assert(png[index] == 0xa5u);
    return 0;
}
