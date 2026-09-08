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
    wz_tap_block_t editable[4];
    wz_tape_manager_edit_t edit;
    wz_tap_block_t extracted;
    const wz_tape_manager_maintenance_operation_t* operation;
    wz_tape_manager_view_t view;
    char report[1024];
    size_t report_length = 0u;
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
        count != 2u ||
        wz_tape_manager_edit_init(&edit, editable, 0u, 4u) !=
            WZ_RESULT_INVALID_ARGUMENT) {
        return 1;
    }
    editable[0] = tap_blocks[0];
    editable[1] = tap_blocks[1];
    if (wz_tape_manager_edit_init(&edit, editable, 2u, 4u) != WZ_RESULT_OK ||
        wz_tape_manager_add_block(&edit, tap_blocks[0], 1u) != WZ_RESULT_OK ||
        edit.count != 3u || wz_tape_manager_reorder(&edit, 2u, 0u) != WZ_RESULT_OK ||
        wz_tape_manager_change_position(&edit, 0u, 2u) != WZ_RESULT_OK ||
        wz_tape_manager_extract_block(&edit, 1u, &extracted) != WZ_RESULT_OK ||
        extracted.length != sizeof(data_a)) {
        return 1;
    }
    if (wz_tape_manager_copy_block_to_new(&edit, 1u, &extracted) != WZ_RESULT_OK ||
        wz_tape_manager_edit_block(&edit, 1u, tap_blocks[1]) != WZ_RESULT_OK ||
        wz_tape_manager_delete_block(&edit, 1u) != WZ_RESULT_OK ||
        edit.count != 2u || wz_tape_manager_delete_block(&edit, 9u) !=
            WZ_RESULT_INVALID_ARGUMENT) {
        return 1;
    }
    if (wz_tape_manager_maintenance_count() != 5u ||
        (operation = wz_tape_manager_maintenance_at(
             WZ_TAPE_MANAGER_FORMAT_STANDARD_TAP, 0u)) == 0 ||
        operation->available || strcmp(operation->unavailable_reason,
                                       "requires-native-tape") != 0 ||
        (operation = wz_tape_manager_maintenance_at(
             WZ_TAPE_MANAGER_FORMAT_NATIVE_TAP, 4u)) == 0 ||
        !operation->available || operation->unavailable_reason != 0 ||
        strcmp(operation->command_id, "media.tape.native.efficiency") != 0 ||
        wz_tape_manager_maintenance_at(WZ_TAPE_MANAGER_FORMAT_NATIVE_TAP, 5u) != 0) {
        return 1;
    }
    if (wz_tape_manager_export_report(&view, rows, 2u, report,
            sizeof(report), &report_length) != WZ_RESULT_OK ||
        report_length == 0u || strstr(report, "Tape Report") == 0 ||
        strstr(report, "Source: demo.tap") == 0 ||
        strstr(report, "Block 1: type=TAP data block") == 0 ||
        wz_tape_manager_export_report(&view, rows, 2u, report, 8u,
                                      &report_length) != WZ_RESULT_BUFFER_TOO_SMALL ||
        report_length != 0u) {
        return 1;
    }
    puts("wz_tape_manager presentation contract passed");
    return 0;
}
