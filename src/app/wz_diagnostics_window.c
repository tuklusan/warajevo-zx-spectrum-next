/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_diagnostics_window.h"

#include <string.h>

static wz_diagnostics_window_result_t refresh_view(
    wz_diagnostics_window_t* window)
{
    if (window == 0 || !window->open) return WZ_DIAGNOSTICS_WINDOW_NOT_OPEN;
    window->trace_available = window->trace != 0 && window->trace->file != 0;
    window->trace_frozen = window->trace_available && window->trace->frozen;
    window->trace_failed = window->trace_available && window->trace->failed;
    window->forwarding_enabled = window->router != 0 &&
        window->router->forwarding_enabled;
    window->diagnostic_block_available = window->machine != 0;
    if (window->trace_available) {
        window->first_sequence = window->trace->first_sequence;
        window->last_sequence = window->trace->last_sequence;
        window->generation = window->trace->generation;
    } else {
        window->first_sequence = 0u;
        window->last_sequence = 0u;
        window->generation = 0u;
    }
    return WZ_DIAGNOSTICS_WINDOW_OK;
}

void wz_diagnostics_window_init(wz_diagnostics_window_t* window)
{
    if (window != 0) memset(window, 0, sizeof(*window));
}

wz_diagnostics_window_result_t wz_diagnostics_window_open(
    wz_diagnostics_window_t* window,
    const wz_machine_t* machine,
    const wz_trace_file_t* trace,
    const wz_diagnostics_router_t* router)
{
    if (window == 0) return WZ_DIAGNOSTICS_WINDOW_INVALID_ARGUMENT;
    wz_diagnostics_window_init(window);
    window->machine = machine;
    window->trace = trace;
    window->router = router;
    window->open = true;
    return refresh_view(window);
}

void wz_diagnostics_window_close(wz_diagnostics_window_t* window)
{
    if (window != 0) {
        window->open = false;
        window->machine = 0;
        window->trace = 0;
        window->router = 0;
    }
}

bool wz_diagnostics_window_is_open(const wz_diagnostics_window_t* window)
{
    return window != 0 && window->open;
}

wz_diagnostics_window_result_t wz_diagnostics_window_refresh(
    wz_diagnostics_window_t* window)
{
    return refresh_view(window);
}

const char* wz_diagnostics_window_trace_reason(
    const wz_diagnostics_window_t* window)
{
    if (!wz_diagnostics_window_is_open(window)) return "window-unavailable";
    if (!window->trace_available) return "trace-file-unavailable";
    if (window->trace_failed) return "trace-file-failed";
    if (window->trace_frozen) return "trace-file-frozen";
    return "trace-file-active";
}

const char* wz_diagnostics_window_forwarding_reason(
    const wz_diagnostics_window_t* window)
{
    if (!wz_diagnostics_window_is_open(window)) return "window-unavailable";
    if (window->router == 0) return "diagnostics-router-unavailable";
    if (!window->forwarding_enabled) return "trace-forwarding-disabled";
    if (window->router->syslog == 0) return "syslog-sink-unavailable";
    return "trace-forwarding-enabled";
}

wz_result_t wz_diagnostics_window_register_commands(
    wz_command_registry_t* registry,
    wz_command_availability_fn availability,
    wz_command_handler_fn handler,
    const void* context)
{
    wz_command_metadata_t command;
    if (registry == 0 || handler == 0) return WZ_RESULT_INVALID_ARGUMENT;
    command.id = WZ_DIAGNOSTICS_WINDOW_COMMAND_ID;
    command.label = "Diagnostics";
    command.description = "Open project diagnostics";
    command.menu_group = "Tools";
    command.parameter_schema = "none";
    command.result_schema = "window";
    command.handler_identity = "ui.diagnostics.window";
    command.parameter_acquisition = "none";
    command.keyboard_shortcut = "";
    command.permission = WZ_COMMAND_LOCAL_ONLY;
    command.availability = availability;
    command.handler = handler;
    command.handler_context = context;
    command.affects_machine_state = false;
    command.recordable = false;
    return wz_command_registry_register(registry, command);
}
