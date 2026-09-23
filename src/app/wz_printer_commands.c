/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_printer_commands.h"

#include <stdint.h>

static bool printer_available(const void* opaque, const char** reason)
{
    const wz_printer_command_context_t* context = opaque;
    if (reason != 0) *reason = 0;
    if (context == 0 || context->machine == 0) {
        if (reason != 0) *reason = "machine-unavailable";
        return false;
    }
    return true;
}

static wz_result_t printer_mode_handler(
    const void* opaque, wz_command_arguments_t arguments,
    wz_command_result_t* result)
{
    const wz_printer_command_context_t* context = opaque;
    wz_printer_mode_t mode;
    if (context == 0 || context->machine == 0 || result == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (arguments.data == 0 || arguments.size != sizeof(uint8_t)) {
        result->status = WZ_COMMAND_RESULT_REJECTED;
        result->reason = "printer-mode-argument-required";
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    mode = (wz_printer_mode_t)*(const uint8_t*)arguments.data;
    if (mode > WZ_PRINTER_MODE_HP_ENLARGED) {
        result->status = WZ_COMMAND_RESULT_REJECTED;
        result->reason = "unknown-printer-mode";
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    return wz_machine_set_printer_mode(context->machine, mode);
}

wz_result_t wz_printer_commands_register(
    wz_command_registry_t* registry,
    wz_printer_command_context_t* context)
{
    wz_command_metadata_t command;
    if (registry == 0 || context == 0) return WZ_RESULT_INVALID_ARGUMENT;
    command.id = WZ_PRINTER_MODE_COMMAND_ID;
    command.label = "Set ZX Printer Mode";
    command.description = "Configure the deterministic virtual ZX Printer mode";
    command.menu_group = "settings.peripherals.zx_printer";
    command.parameter_schema = "printer-mode-u8";
    command.result_schema = "wz-command-result";
    command.handler_identity = "wz_printer_commands";
    command.parameter_acquisition = "printer-mode-radio-group";
    command.keyboard_shortcut = 0;
    command.permission = WZ_COMMAND_HOST_WRITE;
    command.availability = printer_available;
    command.handler = printer_mode_handler;
    command.handler_context = context;
    command.affects_machine_state = true;
    command.recordable = true;
    return wz_command_registry_register(registry, command);
}
