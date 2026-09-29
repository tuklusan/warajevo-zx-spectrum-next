/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. */

#include "app/wz_tape_manager.h"

#include <stdio.h>
#include <string.h>

static int fail(const char* message)
{
    (void)fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static void initialize(wz_tap_block_t* blocks, wz_byte_t data[5])
{
    for (size_t index = 0u; index < 5u; ++index) {
        data[index] = (wz_byte_t)(10u + index);
        blocks[index].data = &data[index];
        blocks[index].length = 1u;
    }
}

int main(void)
{
    wz_tap_block_t blocks[5];
    wz_tap_block_t copied[5];
    wz_byte_t data[5];
    wz_tape_manager_edit_t edit;
    bool selected[5] = {false, true, true, false, false};
    size_t copied_count = 0u;
    initialize(blocks, data);
    if (wz_tape_manager_edit_init(&edit, blocks, 5u, 5u) != WZ_RESULT_OK)
        return fail("edit initialization");
    if (wz_tape_manager_reorder_selected(&edit, selected, 5u, -1) !=
        WZ_RESULT_OK || data[0] != 10u ||
        edit.blocks[0].data[0] != 11u || edit.blocks[1].data[0] != 12u ||
        edit.blocks[2].data[0] != 10u || !selected[0] || !selected[1] ||
        selected[2]) return fail("stable group move up");
    if (wz_tape_manager_reorder_selected(&edit, selected, 5u, 1) !=
        WZ_RESULT_OK || edit.blocks[0].data[0] != 10u ||
        edit.blocks[1].data[0] != 11u || edit.blocks[2].data[0] != 12u ||
        selected[0] || !selected[1] || !selected[2]) {
        return fail("stable group move down");
    }
    selected[0] = true;
    selected[1] = false;
    selected[2] = true;
    if (wz_tape_manager_copy_selected_to_new(&edit, selected, 5u, copied,
            2u, &copied_count) != WZ_RESULT_OK || copied_count != 2u ||
        copied[0].data[0] != 10u || copied[1].data[0] != 12u) {
        return fail("copy selected blocks in tape order");
    }
    if (wz_tape_manager_copy_selected_to_new(&edit, selected, 5u, copied,
            1u, &copied_count) != WZ_RESULT_INVALID_ARGUMENT ||
        copied_count != 0u) return fail("reject undersized extraction");
    selected[0] = false;
    selected[1] = true;
    selected[2] = false;
    selected[3] = true;
    selected[4] = false;
    if (wz_tape_manager_delete_selected(&edit, selected, 5u) != WZ_RESULT_OK ||
        edit.count != 3u || edit.blocks[0].data[0] != 10u ||
        edit.blocks[1].data[0] != 12u || edit.blocks[2].data[0] != 14u ||
        selected[0] || selected[1] || selected[2]) {
        return fail("delete selected blocks stably");
    }
    selected[0] = true;
    selected[1] = true;
    selected[2] = true;
    if (wz_tape_manager_delete_selected(&edit, selected, 3u) !=
            WZ_RESULT_INVALID_ARGUMENT || edit.count != 3u ||
        edit.blocks[0].data[0] != 10u || edit.blocks[1].data[0] != 12u ||
        edit.blocks[2].data[0] != 14u) {
        return fail("retain at least one block");
    }
    if (wz_tape_manager_reorder_selected(&edit, selected, 2u, -1) !=
        WZ_RESULT_INVALID_ARGUMENT ||
        wz_tape_manager_reorder_selected(&edit, selected, 3u, 0) !=
        WZ_RESULT_INVALID_ARGUMENT) return fail("reject invalid selections");
    (void)puts("PASS: selection, reorder, extract, and delete contracts");
    return 0;
}
