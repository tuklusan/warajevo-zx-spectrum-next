/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_debugger_window.h"

#include <string.h>

static wz_debugger_window_result_t refresh_view(wz_debugger_window_t* window)
{
    wz_result_t result;
    size_t consumed = 0u;

    if (window == 0 || !window->open || window->machine == 0) {
        return WZ_DEBUGGER_WINDOW_NOT_OPEN;
    }
    result = wz_debugger_snapshot(window->machine, &window->snapshot);
    if (result != WZ_RESULT_OK ||
        wz_debugger_read_page_info(window->machine, &window->paging) != WZ_RESULT_OK ||
        wz_debugger_read_memory_block(window->machine, window->memory_address,
                                       window->memory, WZ_DEBUGGER_WINDOW_MEMORY_BYTES) !=
            WZ_RESULT_OK ||
        wz_debugger_disassemble(window->machine, window->memory_address,
                                window->disassembly,
                                sizeof(window->disassembly), &consumed) != WZ_RESULT_OK) {
        return WZ_DEBUGGER_WINDOW_DEBUGGER_ERROR;
    }
    window->memory_length = WZ_DEBUGGER_WINDOW_MEMORY_BYTES;
    window->disassembly_length = consumed;
    return WZ_DEBUGGER_WINDOW_OK;
}

void wz_debugger_window_init(wz_debugger_window_t* window)
{
    if (window != 0) {
        memset(window, 0, sizeof(*window));
    }
}

wz_debugger_window_result_t wz_debugger_window_open(
    wz_debugger_window_t* window,
    wz_machine_t* machine,
    bool paused)
{
    if (window == 0 || machine == 0) {
        return WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT;
    }
    wz_debugger_window_init(window);
    window->machine = machine;
    window->memory_address = machine->cpu.program_counter;
    window->open = true;
    window->paused = paused;
    if (wz_debugger_set_access_mode(machine,
                                    paused ? WZ_DEBUGGER_PAUSED_MUTATION
                                           : WZ_DEBUGGER_READ_ONLY) != WZ_RESULT_OK) {
        wz_debugger_window_close(window);
        return WZ_DEBUGGER_WINDOW_DEBUGGER_ERROR;
    }
    return refresh_view(window);
}

void wz_debugger_window_close(wz_debugger_window_t* window)
{
    if (window == 0) {
        return;
    }
    window->open = false;
    window->paused = false;
    window->machine = 0;
    window->memory_length = 0u;
    window->disassembly_length = 0u;
}

bool wz_debugger_window_is_open(const wz_debugger_window_t* window)
{
    return window != 0 && window->open;
}

bool wz_debugger_window_is_paused(const wz_debugger_window_t* window)
{
    return wz_debugger_window_is_open(window) && window->paused;
}

wz_debugger_window_result_t wz_debugger_window_set_paused(
    wz_debugger_window_t* window,
    bool paused)
{
    if (!wz_debugger_window_is_open(window)) {
        return WZ_DEBUGGER_WINDOW_NOT_OPEN;
    }
    if (wz_debugger_set_access_mode(window->machine,
                                    paused ? WZ_DEBUGGER_PAUSED_MUTATION
                                           : WZ_DEBUGGER_READ_ONLY) != WZ_RESULT_OK) {
        return WZ_DEBUGGER_WINDOW_DEBUGGER_ERROR;
    }
    window->paused = paused;
    return WZ_DEBUGGER_WINDOW_OK;
}

wz_debugger_window_result_t wz_debugger_window_refresh(
    wz_debugger_window_t* window)
{
    return refresh_view(window);
}

wz_debugger_window_result_t wz_debugger_window_select_memory(
    wz_debugger_window_t* window,
    wz_word_t address)
{
    if (!wz_debugger_window_is_open(window)) {
        return WZ_DEBUGGER_WINDOW_NOT_OPEN;
    }
    window->memory_address = address;
    return refresh_view(window);
}

wz_debugger_window_result_t wz_debugger_window_write_memory(
    wz_debugger_window_t* window,
    wz_word_t address,
    wz_byte_t value)
{
    if (!wz_debugger_window_is_open(window)) {
        return WZ_DEBUGGER_WINDOW_NOT_OPEN;
    }
    if (!window->paused) {
        return WZ_DEBUGGER_WINDOW_NOT_PAUSED;
    }
    if (wz_debugger_write_memory(window->machine, address, value) != WZ_RESULT_OK) {
        return WZ_DEBUGGER_WINDOW_DEBUGGER_ERROR;
    }
    return refresh_view(window);
}

wz_debugger_window_result_t wz_debugger_window_step(
    wz_debugger_window_t* window)
{
    if (!wz_debugger_window_is_open(window)) {
        return WZ_DEBUGGER_WINDOW_NOT_OPEN;
    }
    if (!window->paused) {
        return WZ_DEBUGGER_WINDOW_NOT_PAUSED;
    }
    if (wz_debugger_step(window->machine, &window->executed) != WZ_RESULT_OK) {
        return WZ_DEBUGGER_WINDOW_DEBUGGER_ERROR;
    }
    return refresh_view(window);
}

wz_debugger_window_result_t wz_debugger_window_continue(
    wz_debugger_window_t* window,
    size_t max_instructions)
{
    if (!wz_debugger_window_is_open(window)) {
        return WZ_DEBUGGER_WINDOW_NOT_OPEN;
    }
    if (!window->paused || max_instructions == 0u) {
        return WZ_DEBUGGER_WINDOW_NOT_PAUSED;
    }
    if (wz_debugger_continue(window->machine, max_instructions,
                             &window->executed) != WZ_RESULT_OK) {
        return WZ_DEBUGGER_WINDOW_DEBUGGER_ERROR;
    }
    return refresh_view(window);
}

void wz_debugger_window_set_trace_view_visible(
    wz_debugger_window_t* window,
    bool visible)
{
    if (window != 0) {
        window->trace_view_visible = visible;
    }
}

const wz_debugger_snapshot_t* wz_debugger_window_snapshot(
    const wz_debugger_window_t* window)
{
    return wz_debugger_window_is_open(window) ? &window->snapshot : 0;
}

const wz_debugger_page_info_t* wz_debugger_window_paging(
    const wz_debugger_window_t* window)
{
    return wz_debugger_window_is_open(window) ? &window->paging : 0;
}

const wz_byte_t* wz_debugger_window_memory(
    const wz_debugger_window_t* window)
{
    return wz_debugger_window_is_open(window) ? window->memory : 0;
}

const char* wz_debugger_window_disassembly(
    const wz_debugger_window_t* window)
{
    return wz_debugger_window_is_open(window) ? window->disassembly : 0;
}

wz_result_t wz_debugger_window_register_commands(
    wz_command_registry_t* registry,
    wz_command_availability_fn availability,
    wz_command_handler_fn handler,
    const void* context)
{
    static const wz_command_metadata_t commands[] = {
        {WZ_DEBUGGER_WINDOW_COMMAND_ID, "Debugger", "Open the Debugger / Monitor",
         "Tools", "none", "window", "ui.debugger.window", "none", "F12",
         WZ_COMMAND_LOCAL_ONLY, 0, 0, 0, false, false},
        {WZ_DEBUGGER_WINDOW_STEP_COMMAND_ID, "Step", "Execute one instruction",
         "Tools.Debugger", "none", "result", "ui.debugger.step", "none", "F10",
         WZ_COMMAND_LOCAL_ONLY, 0, 0, 0, true, true},
        {WZ_DEBUGGER_WINDOW_CONTINUE_COMMAND_ID, "Continue", "Continue execution",
         "Tools.Debugger", "max-instructions", "result", "ui.debugger.continue", "none", "F5",
         WZ_COMMAND_LOCAL_ONLY, 0, 0, 0, true, true},
        {WZ_DEBUGGER_WINDOW_MEMORY_COMMAND_ID, "Memory", "Inspect or edit memory",
         "Tools.Debugger", "address,value", "result", "ui.debugger.memory", "dialog", "",
         WZ_COMMAND_LOCAL_ONLY, 0, 0, 0, true, true}
    };
    size_t index;

    if (registry == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    for (index = 0u; index < sizeof(commands) / sizeof(commands[0]); ++index) {
        wz_command_metadata_t command = commands[index];
        command.availability = availability;
        command.handler = handler;
        command.handler_context = context;
        if (wz_command_registry_register(registry, command) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
    }
    return WZ_RESULT_OK;
}
