/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>

#include "core/wz_presentation_snapshot.h"

int main(void)
{
    wz_byte_t published_storage[4u];
    wz_byte_t source_storage[4u] = {1u, 2u, 3u, 4u};
    wz_byte_t invalid_storage[4u] = {1u, 2u, 3u, 0x19u};
    wz_raster_buffer_t source = {source_storage, 2u, 2u};
    wz_raster_buffer_t invalid = {invalid_storage, 2u, 2u};
    wz_presentation_snapshot_t snapshot;
    const wz_raster_buffer_t* view;

    if (wz_presentation_snapshot_init(&snapshot, 2u, 2u,
                                      published_storage, sizeof(published_storage)) !=
            WZ_RESULT_OK || wz_presentation_snapshot_sequence(&snapshot) != 0u ||
        wz_presentation_snapshot_publish(&snapshot, &source) != WZ_RESULT_OK ||
        wz_presentation_snapshot_sequence(&snapshot) != 1u ||
        wz_presentation_snapshot_publish(&snapshot, &invalid) !=
            WZ_RESULT_INVALID_ARGUMENT ||
        wz_presentation_snapshot_sequence(&snapshot) != 1u) {
        fputs("presentation snapshot publication contract failed\n", stderr);
        return 1;
    }
    view = wz_presentation_snapshot_raster(&snapshot);
    if (view == 0 || view->samples[0] != 1u || view->samples[3] != 4u) {
        fputs("presentation snapshot read contract failed\n", stderr);
        return 1;
    }
    puts("wz_presentation_snapshot contract passed");
    return 0;
}
