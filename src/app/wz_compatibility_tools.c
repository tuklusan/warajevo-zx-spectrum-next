/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_compatibility_tools.h"

#include <ctype.h>
#include <string.h>

static const wz_compatibility_tool_t tools[WZ_COMPATIBILITY_TOOL_COUNT] = {
    {"tools.compatibility", "Compatibility Tools", "", "", "", 0,
     WZ_COMPATIBILITY_AVAILABLE, false},
    {"tools.compatibility.tape", "Tape Converter", "tape", "tape",
     "Conversion may discard or normalize source-format information.",
     "conversion-not-yet-implemented", WZ_COMPATIBILITY_LATER, true},
    {"tools.compatibility.snapshot", "Snapshot Converter", "snapshot", "snapshot",
     "Conversion may discard machine-state information that the destination cannot represent.",
     "conversion-not-yet-implemented", WZ_COMPATIBILITY_LATER, true},
    {"tools.compatibility.spectrum_data", "Spectrum Data Converter", "spectrum-data", "text-or-spectrum-data",
     "Conversion may discard source-format metadata or machine-visible information.",
     "conversion-not-yet-implemented", WZ_COMPATIBILITY_LATER, true},
    {"tools.compatibility.microdrive", "Microdrive Tools", "MDR", "MDR",
     "Conversion may discard or normalize cartridge data and metadata.",
     "conversion-not-yet-implemented", WZ_COMPATIBILITY_LATER, true},
    {"tools.compatibility.database", "Legacy Database Converter", "legacy-database", "portable-data",
     "Conversion may discard legacy database fields that the destination cannot represent.",
     "conversion-not-yet-implemented", WZ_COMPATIBILITY_LATER, true}
};

size_t wz_compatibility_tools_count(void)
{
    return WZ_COMPATIBILITY_TOOL_COUNT;
}

const wz_compatibility_tool_t* wz_compatibility_tools_at(size_t index)
{
    return index < WZ_COMPATIBILITY_TOOL_COUNT ? &tools[index] : 0;
}

bool wz_compatibility_tools_is_available(size_t index, const char** reason)
{
    const wz_compatibility_tool_t* tool = wz_compatibility_tools_at(index);

    if (reason != 0) {
        *reason = tool == 0 ? "unknown-compatibility-tool" : tool->reason;
    }
    return tool != 0 && tool->availability == WZ_COMPATIBILITY_AVAILABLE;
}

bool wz_compatibility_tools_loss_disclosure(size_t index, const char** warning)
{
    const wz_compatibility_tool_t* tool = wz_compatibility_tools_at(index);

    if (warning != 0) {
        *warning = tool == 0 ? "unknown-compatibility-tool" : tool->warning;
    }
    return tool != 0 && tool->requires_loss_disclosure;
}

const char* wz_compatibility_tools_command_id(void)
{
    return WZ_COMPATIBILITY_TOOLS_COMMAND_ID;
}

void wz_compatibility_tools_window_init(wz_compatibility_tools_window_t* window)
{
    if (window != 0) memset(window, 0, sizeof(*window));
}

void wz_compatibility_tools_window_open(wz_compatibility_tools_window_t* window)
{
    if (window != 0) window->open = true;
}

void wz_compatibility_tools_window_close(wz_compatibility_tools_window_t* window)
{
    if (window != 0) window->open = false;
}

bool wz_compatibility_tools_window_is_open(
    const wz_compatibility_tools_window_t* window)
{
    return window != 0 && window->open;
}

wz_result_t wz_compatibility_tools_register_commands(
    wz_command_registry_t* registry,
    wz_command_availability_fn availability,
    wz_command_handler_fn handler,
    const void* context)
{
    wz_command_metadata_t command;
    if (registry == 0 || handler == 0) return WZ_RESULT_INVALID_ARGUMENT;
    memset(&command, 0, sizeof(command));
    command.id = WZ_COMPATIBILITY_TOOLS_COMMAND_ID;
    command.label = "Compatibility Tools";
    command.description = "Show legacy conversion tool availability";
    command.menu_group = "tools";
    command.parameter_schema = "NONE";
    command.result_schema = "window";
    command.handler_identity = "ui.compatibility-tools.window";
    command.parameter_acquisition = "none";
    command.permission = WZ_COMMAND_LOCAL_ONLY;
    command.availability = availability;
    command.handler = handler;
    command.handler_context = context;
    command.affects_machine_state = false;
    command.recordable = false;
    return wz_command_registry_register(registry, command);
}

static bool matches(const char* format, const char* value)
{
    if (format == 0 || value == 0) return false;
    if (*format == '.') ++format;
    while (*format != '\0' && *value != '\0') {
        if (tolower((unsigned char)*format) !=
            tolower((unsigned char)*value)) return false;
        ++format;
        ++value;
    }
    return *format == '\0' && *value == '\0';
}

wz_file_route_t wz_compatibility_tools_route_for_format(
    const char* format,
    const char** reason)
{
    static const char* native_formats[] = {
        "tap", "tzx", "wav", "sna", "z80", "mdr"
    };
    static const char* conversion_formats[] = {
        "voc", "blk", "spc", "ltp", "zxs", "zxt", "slt", "sem",
        "sit", "snp", "scr", "dck", "trd"
    };
    size_t index;

    if (reason != 0) {
        *reason = "unknown-format";
    }
    if (format == 0 || *format == '\0') {
        return WZ_FILE_ROUTE_UNKNOWN;
    }
    for (index = 0u; index < sizeof(native_formats) / sizeof(native_formats[0]); ++index) {
        if (matches(format, native_formats[index])) {
            if (reason != 0) {
                *reason = "native-load-run";
            }
            return WZ_FILE_ROUTE_NATIVE_LOAD;
        }
    }
    for (index = 0u; index < sizeof(conversion_formats) / sizeof(conversion_formats[0]); ++index) {
        if (matches(format, conversion_formats[index])) {
            if (reason != 0) {
                *reason = "explicit-conversion-required";
            }
            return WZ_FILE_ROUTE_EXPLICIT_CONVERSION;
        }
    }
    if (reason != 0) {
        *reason = "unsupported-format";
    }
    return WZ_FILE_ROUTE_UNSUPPORTED;
}

const char* wz_compatibility_tools_route_name(wz_file_route_t route)
{
    switch (route) {
    case WZ_FILE_ROUTE_NATIVE_LOAD:
        return "native-load-run";
    case WZ_FILE_ROUTE_EXPLICIT_CONVERSION:
        return "explicit-conversion";
    case WZ_FILE_ROUTE_UNSUPPORTED:
        return "unsupported";
    case WZ_FILE_ROUTE_UNKNOWN:
    default:
        return "unknown";
    }
}
