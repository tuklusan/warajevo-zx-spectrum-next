/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_tape_insert_action.h"

#include <string.h>

wz_tape_insert_action_result_t wz_tape_insert_action_dispatch(
    wz_file_dialog_result_t dialog_result,
    const char* path,
    wz_command_registry_t* registry,
    wz_command_result_t* command_result)
{
    if (dialog_result == WZ_FILE_DIALOG_CANCELLED) {
        return WZ_TAPE_INSERT_ACTION_CANCELLED;
    }
    if (dialog_result != WZ_FILE_DIALOG_SELECTED || path == NULL ||
        path[0] == '\0' || registry == NULL || command_result == NULL) {
        return WZ_TAPE_INSERT_ACTION_FAILED;
    }
    if (wz_command_registry_dispatch(registry, WZ_TAPE_INSERT_COMMAND_ID,
            (wz_command_arguments_t){path, strlen(path)}, command_result) !=
        WZ_RESULT_OK) {
        return WZ_TAPE_INSERT_ACTION_FAILED;
    }
    return WZ_TAPE_INSERT_ACTION_INSERTED;
}
