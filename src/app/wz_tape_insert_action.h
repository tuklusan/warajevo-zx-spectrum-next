/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is licensed under the GNU General Public License, version 2 or later.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#ifndef WZ_APP_WZ_TAPE_INSERT_ACTION_H
#define WZ_APP_WZ_TAPE_INSERT_ACTION_H

#include "app/wz_file_dialog.h"
#include "app/wz_tape_media_commands.h"

typedef enum {
    WZ_TAPE_INSERT_ACTION_CANCELLED = 0,
    WZ_TAPE_INSERT_ACTION_INSERTED,
    WZ_TAPE_INSERT_ACTION_FAILED
} wz_tape_insert_action_result_t;

/* A cancelled chooser is a no-op; a selected path uses the shared command. */
wz_tape_insert_action_result_t wz_tape_insert_action_dispatch(
    wz_file_dialog_result_t dialog_result,
    const char* path,
    wz_command_registry_t* registry,
    wz_command_result_t* command_result);

#endif
