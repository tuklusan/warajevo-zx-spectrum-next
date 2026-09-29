/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_tape_media_commands.h"

#include <stdio.h>
#include <string.h>

static bool tape_insert_available(const void* opaque, const char** reason)
{
    const wz_tape_media_command_context_t* context =
        (const wz_tape_media_command_context_t*)opaque;
    if (reason != NULL) *reason = NULL;
    if (context == NULL || context->machine == NULL || context->load == NULL) {
        if (reason != NULL) *reason = "machine-unavailable";
        return false;
    }
    return true;
}

static bool tape_eject_available(const void* opaque, const char** reason)
{
    const wz_tape_media_command_context_t* context =
        (const wz_tape_media_command_context_t*)opaque;
    if (reason != NULL) *reason = NULL;
    if (context == NULL || context->machine == NULL) {
        if (reason != NULL) *reason = "machine-unavailable";
        return false;
    }
    if (context->machine->tape_mounted == 0u) {
        if (reason != NULL) *reason = "no-tape-mounted";
        return false;
    }
    return true;
}

static wz_result_t tape_insert_handler(
    const void* opaque,
    const wz_command_arguments_t arguments,
    wz_command_result_t* result)
{
    const wz_tape_media_command_context_t* context =
        (const wz_tape_media_command_context_t*)opaque;
    char path[4096];
    if (context == NULL || context->machine == NULL || context->load == NULL ||
        arguments.data == NULL || arguments.size == 0u ||
        arguments.size >= sizeof(path) || result == NULL) {
        if (result != NULL) {
            result->status = WZ_COMMAND_RESULT_REJECTED;
            result->reason = "tape-path-required";
        }
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    for (size_t index = 0u; index < arguments.size; ++index) {
        if (((const char*)arguments.data)[index] == '\0') {
            result->status = WZ_COMMAND_RESULT_REJECTED;
            result->reason = "invalid-tape-path";
            return WZ_RESULT_INVALID_ARGUMENT;
        }
    }
    memcpy(path, arguments.data, arguments.size);
    path[arguments.size] = '\0';
    if (context->load(path, context->context) &&
        context->machine->tape_mounted != 0u) {
        (void)snprintf(result->message, sizeof(result->message), "tape inserted");
        return WZ_RESULT_OK;
    }
    result->status = WZ_COMMAND_RESULT_FAILED;
    result->reason = "tape-load-failed";
    return WZ_RESULT_PARSE_ERROR;
}

static wz_result_t tape_eject_handler(
    const void* opaque,
    const wz_command_arguments_t arguments,
    wz_command_result_t* result)
{
    const wz_tape_media_command_context_t* context =
        (const wz_tape_media_command_context_t*)opaque;
    (void)arguments;
    if (context == NULL || context->machine == NULL || result == NULL) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (wz_machine_unmount_tape(context->machine) != WZ_RESULT_OK) {
        result->status = WZ_COMMAND_RESULT_FAILED;
        result->reason = "tape-eject-failed";
        return WZ_RESULT_INVALID_STATE;
    }
    if (context->release != NULL) context->release(context->context);
    (void)snprintf(result->message, sizeof(result->message), "tape ejected");
    return WZ_RESULT_OK;
}

wz_result_t wz_tape_media_commands_register(
    wz_command_registry_t* registry,
    wz_tape_media_command_context_t* context)
{
    wz_command_metadata_t insert_command = {
        WZ_TAPE_INSERT_COMMAND_ID, "Insert Tape...",
        "Load a tape image from a local path", "media", "PATH",
        "wz-command-result", "wz_tape_media_commands", "file-dialog-path",
        NULL, WZ_COMMAND_LOCAL_ONLY, tape_insert_available,
        tape_insert_handler, context, true, false, NULL
    };
    wz_command_metadata_t eject_command = {
        WZ_TAPE_EJECT_COMMAND_ID, "Eject Tape",
        "Eject the mounted tape", "media", "NONE", "wz-command-result",
        "wz_tape_media_commands", "local", NULL, WZ_COMMAND_MEDIA_DESTRUCTIVE,
        tape_eject_available, tape_eject_handler, context, true, true, NULL
    };
    if (registry == NULL || context == NULL || context->machine == NULL) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (wz_command_registry_register(registry, insert_command) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    return wz_command_registry_register(registry, eject_command);
}
