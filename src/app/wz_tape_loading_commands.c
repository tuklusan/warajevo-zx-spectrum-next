/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#include "app/wz_tape_loading_commands.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static bool tape_loading_available(const void* opaque, const char** reason)
{
    const wz_tape_loading_handler_context_t* handler =
        (const wz_tape_loading_handler_context_t*)opaque;
    if (reason != NULL) *reason = NULL;
    if (handler == NULL || handler->owner == NULL ||
        handler->owner->machine == NULL) {
        if (reason != NULL) *reason = "machine-unavailable";
        return false;
    }
    return true;
}

static bool tape_loading_parse_mode(const wz_command_arguments_t arguments,
                                    wz_tape_loading_mode_t* mode)
{
    char value[16];
    size_t index;

    if (arguments.data == NULL || mode == NULL || arguments.size == 0u ||
        arguments.size >= sizeof(value)) {
        return false;
    }
    for (index = 0u; index < arguments.size; ++index) {
        unsigned char character = (unsigned char)
            ((const char*)arguments.data)[index];
        if (character == '\0') return false;
        value[index] = (char)tolower(character);
    }
    value[arguments.size] = '\0';
    if (strcmp(value, "normal") == 0) {
        *mode = WZ_TAPE_LOADING_NORMAL;
        return true;
    }
    if (strcmp(value, "instant") == 0) {
        *mode = WZ_TAPE_LOADING_INSTANT_TRAP;
        return true;
    }
    return false;
}

static wz_result_t tape_loading_handler(
    const void* opaque,
    const wz_command_arguments_t arguments,
    wz_command_result_t* result)
{
    const wz_tape_loading_handler_context_t* handler =
        (const wz_tape_loading_handler_context_t*)opaque;
    wz_tape_loading_mode_t mode;

    if (handler == NULL || handler->owner == NULL ||
        handler->owner->machine == NULL || result == NULL) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (handler->fixed_mode) {
        mode = handler->mode;
    } else if (!tape_loading_parse_mode(arguments, &mode)) {
        result->status = WZ_COMMAND_RESULT_REJECTED;
        result->reason = "unknown-tape-loading-mode";
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (wz_machine_set_tape_loading_mode(handler->owner->machine, mode) !=
        WZ_RESULT_OK) {
        result->status = WZ_COMMAND_RESULT_FAILED;
        result->reason = "tape-loading-mode-change-failed";
        return WZ_RESULT_INVALID_STATE;
    }
    (void)snprintf(result->message, sizeof(result->message), "%s",
        mode == WZ_TAPE_LOADING_NORMAL ? "Normal" : "Instant / Trap");
    return WZ_RESULT_OK;
}

wz_result_t wz_tape_loading_commands_register(
    wz_command_registry_t* registry,
    wz_tape_loading_command_context_t* context)
{
    static const char* const ids[3] = {
        WZ_TAPE_LOADING_MODE_COMMAND_ID,
        WZ_TAPE_LOADING_NORMAL_COMMAND_ID,
        WZ_TAPE_LOADING_INSTANT_COMMAND_ID
    };
    static const char* const labels[3] = {
        "Set Tape Loading Mode", "Normal", "Instant / Trap"
    };
    static const char* const schemas[3] = {"normal|instant", "NONE", "NONE"};
    wz_command_metadata_t commands[3];

    if (registry == NULL || context == NULL || context->machine == NULL) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    context->handlers[0].owner = context;
    context->handlers[0].mode = WZ_TAPE_LOADING_NORMAL;
    context->handlers[0].fixed_mode = false;
    context->handlers[1].owner = context;
    context->handlers[1].mode = WZ_TAPE_LOADING_NORMAL;
    context->handlers[1].fixed_mode = true;
    context->handlers[2].owner = context;
    context->handlers[2].mode = WZ_TAPE_LOADING_INSTANT_TRAP;
    context->handlers[2].fixed_mode = true;

    for (size_t index = 0u; index < 3u; ++index) {
        commands[index].id = ids[index];
        commands[index].label = labels[index];
        commands[index].description = "Select pulse-accurate or optional instant tape loading";
        commands[index].menu_group = index == 0u ? NULL : "media";
        commands[index].parameter_schema = schemas[index];
        commands[index].result_schema = "wz-command-result";
        commands[index].handler_identity = "wz_tape_loading_commands";
        commands[index].parameter_acquisition =
            index == 0u ? "mode-selector" : "fixed-mode-choice";
        commands[index].keyboard_shortcut = NULL;
        commands[index].permission = WZ_COMMAND_REMOTE_SAFE;
        commands[index].availability = tape_loading_available;
        commands[index].handler = tape_loading_handler;
        commands[index].handler_context = &context->handlers[index];
        commands[index].affects_machine_state = true;
        commands[index].recordable = true;
        commands[index].remote_permission = NULL;
    }
    for (size_t index = 0u; index < 3u; ++index) {
        wz_result_t result = wz_command_registry_register(registry,
                                                           commands[index]);
        if (result != WZ_RESULT_OK) return result;
    }
    return WZ_RESULT_OK;
}
