/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_networking_commands.h"

#include <string.h>
#include <stdint.h>

#define WZ_NETWORKING_DIRTY_MEDIA_REASON \
    "dirty-microdrive-requires-local-resolution"

static bool networking_available(const void* opaque, const char** reason)
{
    const wz_networking_command_context_t* context =
        (const wz_networking_command_context_t*)opaque;
    if (reason != 0) *reason = 0;
    if (context == 0 || context->machine == 0) {
        if (reason != 0) *reason = "machine-unavailable";
        return false;
    }
    if (context->machine->networking_mode == WZ_NETWORKING_INTERFACE1 &&
        wz_mdr_transport_is_dirty(&context->machine->microdrive) != 0u &&
        context->flush_callback == 0 && !context->discard_dirty_media) {
        if (reason != 0) *reason = WZ_NETWORKING_DIRTY_MEDIA_REASON;
        return false;
    }
    return true;
}

static bool networking_mode_from_arguments(
    wz_command_arguments_t arguments,
    wz_networking_mode_t* mode)
{
    if (mode == 0 || arguments.data == 0) return false;
    if (arguments.size == sizeof(uint8_t)) {
        *mode = (wz_networking_mode_t)*(const uint8_t*)arguments.data;
        return *mode <= WZ_NETWORKING_EAR_MIC;
    }
    if (arguments.size == 4u && memcmp(arguments.data, "none", 4u) == 0) {
        *mode = WZ_NETWORKING_NONE;
    } else if (arguments.size == 9u &&
               memcmp(arguments.data, "interface1", 9u) == 0) {
        *mode = WZ_NETWORKING_INTERFACE1;
    } else if (arguments.size == 7u &&
               memcmp(arguments.data, "ear_mic", 7u) == 0) {
        *mode = WZ_NETWORKING_EAR_MIC;
    } else {
        return false;
    }
    return true;
}

static wz_command_permission_t networking_remote_permission(
    const void* opaque,
    wz_command_arguments_t arguments)
{
    const wz_networking_command_context_t* context =
        (const wz_networking_command_context_t*)opaque;
    wz_networking_mode_t mode;
    if (!networking_mode_from_arguments(arguments, &mode) ||
        context == 0 || context->machine == 0) {
        return WZ_COMMAND_REMOTE_SAFE;
    }
    if (context->machine->networking_mode == WZ_NETWORKING_INTERFACE1 &&
        wz_mdr_transport_is_dirty(&context->machine->microdrive) != 0u) {
        return WZ_COMMAND_HOST_WRITE;
    }
    return WZ_COMMAND_REMOTE_SAFE;
}

static wz_result_t networking_handler(
    const void* opaque, wz_command_arguments_t arguments,
    wz_command_result_t* result)
{
    const wz_networking_command_context_t* context =
        (const wz_networking_command_context_t*)opaque;
    wz_networking_mode_t mode;

    if (context == 0 || context->machine == 0 || result == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (arguments.data == 0) {
        result->status = WZ_COMMAND_RESULT_REJECTED;
        result->reason = "networking-mode-argument-required";
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (!networking_mode_from_arguments(arguments, &mode)) {
        result->status = WZ_COMMAND_RESULT_REJECTED;
        result->reason = "unknown-networking-mode";
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    return wz_machine_reconfigure_networking_mode_with_mdr_resolution(
        context->machine, mode, context->flush_callback, context->flush_context,
        context->discard_dirty_media);
}

wz_result_t wz_networking_commands_register(
    wz_command_registry_t* registry,
    wz_networking_command_context_t* context)
{
    wz_command_metadata_t command;
    if (registry == 0 || context == 0) return WZ_RESULT_INVALID_ARGUMENT;
    command.id = WZ_NETWORKING_COMMAND_ID;
    command.label = "Set Networking Mode";
    command.description = "Cold-reconfigure the machine networking mode";
    command.menu_group = "settings.peripherals.networking";
    command.parameter_schema = "networking-mode-u8";
    command.result_schema = "wz-command-result";
    command.handler_identity = "wz_networking_commands";
    command.parameter_acquisition = "networking-radio-group";
    command.keyboard_shortcut = 0;
    command.permission = WZ_COMMAND_REMOTE_SAFE;
    command.availability = networking_available;
    command.handler = networking_handler;
    command.handler_context = context;
    command.affects_machine_state = true;
    command.recordable = true;
    command.remote_permission = networking_remote_permission;
    return wz_command_registry_register(registry, command);
}
