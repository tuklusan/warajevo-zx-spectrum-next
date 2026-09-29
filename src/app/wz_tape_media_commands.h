/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#ifndef WZ_APP_WZ_TAPE_MEDIA_COMMANDS_H
#define WZ_APP_WZ_TAPE_MEDIA_COMMANDS_H

#include "app/wz_command_registry.h"
#include "core/wz_machine.h"

#define WZ_TAPE_INSERT_COMMAND_ID "media.tape.insert"
#define WZ_TAPE_EJECT_COMMAND_ID "media.tape.eject"

typedef bool (*wz_tape_media_load_fn)(const char* path, void* context);
typedef void (*wz_tape_media_release_fn)(void* context);

typedef struct {
    wz_machine_t* machine;
    wz_tape_media_load_fn load;
    wz_tape_media_release_fn release;
    void* context;
} wz_tape_media_command_context_t;

wz_result_t wz_tape_media_commands_register(
    wz_command_registry_t* registry,
    wz_tape_media_command_context_t* context);

#endif
