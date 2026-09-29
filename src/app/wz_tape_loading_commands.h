/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#ifndef WZ_APP_WZ_TAPE_LOADING_COMMANDS_H
#define WZ_APP_WZ_TAPE_LOADING_COMMANDS_H

#include "app/wz_command_registry.h"
#include "core/wz_machine.h"

#define WZ_TAPE_LOADING_MODE_COMMAND_ID "media.tape.loading_mode.set"
#define WZ_TAPE_LOADING_NORMAL_COMMAND_ID "media.tape.loading_mode.normal"
#define WZ_TAPE_LOADING_INSTANT_COMMAND_ID "media.tape.loading_mode.instant"

typedef struct wz_tape_loading_command_context
    wz_tape_loading_command_context_t;

typedef struct {
    wz_tape_loading_command_context_t* owner;
    wz_tape_loading_mode_t mode;
    bool fixed_mode;
} wz_tape_loading_handler_context_t;

struct wz_tape_loading_command_context {
    wz_machine_t* machine;
    wz_tape_loading_handler_context_t handlers[3];
};

wz_result_t wz_tape_loading_commands_register(
    wz_command_registry_t* registry,
    wz_tape_loading_command_context_t* context);

#endif
