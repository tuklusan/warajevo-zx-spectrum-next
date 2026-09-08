/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>
#include <string.h>

#include "app/wz_tape_manager.h"

int main(void)
{
    static const wz_byte_t data_a[] = {0x00u, 0x01u};
    static const wz_byte_t data_b[] = {0xffu, 0x02u, 0x03u};
    const wz_tap_block_t tap_blocks[] = {
        {data_a, sizeof(data_a)}, {data_b, sizeof(data_b)}
    };
    wz_tape_manager_block_t rows[2];
    wz_tape_manager_view_t view;
    size_t count = 0u;

    if (wz_tape_manager_view_init(&view, "demo.tap",
            WZ_TAPE_MANAGER_FORMAT_STANDARD_TAP, "normal", 1u, 7u,
            WZ_TAPE_MANAGER_STATE_LOADING, 1u) != WZ_RESULT_OK ||
        strcmp(view.source_identity, "demo.tap") != 0 ||
        view.current_block != 1u || view.current_position != 7u ||
        wz_tape_manager_blocks_from_tap(tap_blocks, 2u, 1u, rows, 2u,
                                         &count) != WZ_RESULT_OK ||
        count != 2u || rows[0].logical_length != 2u ||
        rows[0].stored_length != 4u || rows[0].selected ||
        !rows[1].selected || strcmp(rows[0].type, "TAP data block") != 0 ||
        wz_tape_manager_blocks_from_tap(tap_blocks, 2u, 0u, rows, 1u,
                                         &count) != WZ_RESULT_BUFFER_TOO_SMALL ||
        count != 2u) {
        return 1;
    }
    puts("wz_tape_manager presentation contract passed");
    return 0;
}
