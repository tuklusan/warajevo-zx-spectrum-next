/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_debugger_window.h"

#include <ctype.h>
#include <string.h>

typedef struct {
    const char* name;
    wz_debugger_register_t reg;
} wz_debugger_monitor_register_t;

static const wz_debugger_monitor_register_t monitor_registers[] = {
    {"A", WZ_DEBUGGER_REGISTER_MAIN_A},
    {"F", WZ_DEBUGGER_REGISTER_MAIN_F},
    {"B", WZ_DEBUGGER_REGISTER_MAIN_B},
    {"C", WZ_DEBUGGER_REGISTER_MAIN_C},
    {"D", WZ_DEBUGGER_REGISTER_MAIN_D},
    {"E", WZ_DEBUGGER_REGISTER_MAIN_E},
    {"H", WZ_DEBUGGER_REGISTER_MAIN_H},
    {"L", WZ_DEBUGGER_REGISTER_MAIN_L},
    {"A'", WZ_DEBUGGER_REGISTER_ALT_A},
    {"F'", WZ_DEBUGGER_REGISTER_ALT_F},
    {"B'", WZ_DEBUGGER_REGISTER_ALT_B},
    {"C'", WZ_DEBUGGER_REGISTER_ALT_C},
    {"D'", WZ_DEBUGGER_REGISTER_ALT_D},
    {"E'", WZ_DEBUGGER_REGISTER_ALT_E},
    {"H'", WZ_DEBUGGER_REGISTER_ALT_H},
    {"L'", WZ_DEBUGGER_REGISTER_ALT_L},
    {"IX", WZ_DEBUGGER_REGISTER_IX},
    {"IY", WZ_DEBUGGER_REGISTER_IY},
    {"SP", WZ_DEBUGGER_REGISTER_SP},
    {"PC", WZ_DEBUGGER_REGISTER_PC},
    {"I", WZ_DEBUGGER_REGISTER_I},
    {"R", WZ_DEBUGGER_REGISTER_R},
    {"IFF1", WZ_DEBUGGER_REGISTER_IFF1},
    {"IFF2", WZ_DEBUGGER_REGISTER_IFF2},
    {"EIDELAY", WZ_DEBUGGER_REGISTER_INTERRUPT_ENABLE_DELAY},
    {"IM", WZ_DEBUGGER_REGISTER_INTERRUPT_MODE}
};

static bool monitor_token_equal(const char* token, size_t length,
                                const char* expected)
{
    size_t index;
    if (token == NULL || expected == NULL || strlen(expected) != length) {
        return false;
    }
    for (index = 0u; index < length; ++index) {
        if (toupper((unsigned char)token[index]) !=
            toupper((unsigned char)expected[index])) return false;
    }
    return true;
}

static bool monitor_next_token(const char* command, size_t command_length,
                               size_t* cursor, const char** token,
                               size_t* token_length)
{
    size_t start;
    if (command == NULL || cursor == NULL || token == NULL ||
        token_length == NULL) return false;
    while (*cursor < command_length &&
           (command[*cursor] == ' ' || command[*cursor] == '\t' ||
            command[*cursor] == '\r' || command[*cursor] == '\n')) {
        ++*cursor;
    }
    if (*cursor >= command_length) return false;
    start = *cursor;
    while (*cursor < command_length &&
           command[*cursor] != ' ' && command[*cursor] != '\t' &&
           command[*cursor] != '\r' && command[*cursor] != '\n') {
        ++*cursor;
    }
    *token = command + start;
    *token_length = *cursor - start;
    return true;
}

static bool monitor_parse_hex(const char* token, size_t length,
                              wz_word_t* value)
{
    size_t index;
    wz_word_t parsed = 0u;
    if (token == NULL || value == NULL || length == 0u || length > 4u) {
        return false;
    }
    for (index = 0u; index < length; ++index) {
        const unsigned char character = (unsigned char)token[index];
        const unsigned char upper = (unsigned char)toupper(character);
        unsigned digit;
        if (character >= '0' && character <= '9') {
            digit = (unsigned)(character - '0');
        } else if (upper >= 'A' && upper <= 'F') {
            digit = (unsigned)(upper - 'A') + 10u;
        } else {
            return false;
        }
        parsed = (wz_word_t)((parsed << 4u) | (wz_word_t)digit);
    }
    *value = parsed;
    return true;
}

static bool monitor_parse_decimal(const char* token, size_t length,
                                  unsigned* value)
{
    size_t index;
    unsigned parsed = 0u;
    if (token == NULL || value == NULL || length == 0u || length > 1u) {
        return false;
    }
    for (index = 0u; index < length; ++index) {
        const unsigned char character = (unsigned char)token[index];
        if (character < '0' || character > '9') return false;
        parsed = parsed * 10u + (unsigned)(character - '0');
    }
    *value = parsed;
    return true;
}

static bool monitor_parse_register(const char* token, size_t length,
                                   wz_debugger_register_t* reg)
{
    size_t index;
    if (reg == NULL) return false;
    for (index = 0u; index < sizeof(monitor_registers) /
            sizeof(monitor_registers[0]); ++index) {
        if (monitor_token_equal(token, length, monitor_registers[index].name)) {
            *reg = monitor_registers[index].reg;
            return true;
        }
    }
    return false;
}

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

wz_debugger_window_result_t wz_debugger_window_set_register(
    wz_debugger_window_t* window,
    wz_debugger_register_t reg,
    wz_word_t value)
{
    if (!wz_debugger_window_is_open(window)) {
        return WZ_DEBUGGER_WINDOW_NOT_OPEN;
    }
    if (!window->paused) {
        return WZ_DEBUGGER_WINDOW_NOT_PAUSED;
    }
    if (wz_debugger_set_register(window->machine, reg, value) != WZ_RESULT_OK) {
        return WZ_DEBUGGER_WINDOW_DEBUGGER_ERROR;
    }
    return refresh_view(window);
}

wz_debugger_window_result_t wz_debugger_window_dispatch_monitor_command(
    wz_debugger_window_t* window,
    const char* command,
    size_t command_length)
{
    const char* tokens[18];
    size_t lengths[18];
    size_t cursor = 0u;
    size_t token_count = 0u;
    wz_debugger_register_t reg;
    wz_word_t value;

    if (command == NULL || command_length == 0u) {
        return WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT;
    }
    while (token_count < sizeof(tokens) / sizeof(tokens[0]) &&
           monitor_next_token(command, command_length, &cursor,
                              &tokens[token_count], &lengths[token_count])) {
        ++token_count;
    }
    if (monitor_next_token(command, command_length, &cursor,
                           &tokens[0], &lengths[0])) {
        return WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT;
    }
    if (token_count == 4u &&
        monitor_token_equal(tokens[0], lengths[0], "R") &&
        monitor_token_equal(tokens[1], lengths[1], "SETREG") &&
        monitor_parse_register(tokens[2], lengths[2], &reg) &&
        monitor_parse_hex(tokens[3], lengths[3], &value)) {
        return wz_debugger_window_set_register(window, reg, value);
    }
    if (token_count == 4u &&
        monitor_token_equal(tokens[0], lengths[0], "K")) {
        wz_word_t first;
        wz_word_t last;
        wz_word_t byte_value;
        if (!monitor_parse_hex(tokens[1], lengths[1], &first) ||
            !monitor_parse_hex(tokens[2], lengths[2], &last) ||
            !monitor_parse_hex(tokens[3], lengths[3], &byte_value) ||
            byte_value > UINT8_MAX || last < first) {
            return WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT;
        }
        if (!wz_debugger_window_is_open(window)) {
            return WZ_DEBUGGER_WINDOW_NOT_OPEN;
        }
        if (!window->paused) return WZ_DEBUGGER_WINDOW_NOT_PAUSED;
        if (wz_debugger_fill_memory(window->machine, first,
                (size_t)last - (size_t)first + 1u,
                (wz_byte_t)byte_value) != WZ_RESULT_OK) {
            return WZ_DEBUGGER_WINDOW_DEBUGGER_ERROR;
        }
        return refresh_view(window);
    }
    if (token_count >= 2u && token_count <= 17u &&
        monitor_token_equal(tokens[0], lengths[0], "P")) {
        wz_byte_t bytes[16];
        bool write_mask[16];
        size_t byte_count = token_count - 1u;
        for (size_t index = 0u; index < byte_count; ++index) {
            if (lengths[index + 1u] == 2u &&
                tokens[index + 1u][0] == '-' &&
                tokens[index + 1u][1] == '1') {
                bytes[index] = 0u;
                write_mask[index] = false;
            } else {
                wz_word_t parsed;
                if (!monitor_parse_hex(tokens[index + 1u],
                                       lengths[index + 1u], &parsed) ||
                    parsed > UINT8_MAX) {
                    return WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT;
                }
                bytes[index] = (wz_byte_t)parsed;
                write_mask[index] = true;
            }
        }
        if (!wz_debugger_window_is_open(window)) {
            return WZ_DEBUGGER_WINDOW_NOT_OPEN;
        }
        if (!window->paused) return WZ_DEBUGGER_WINDOW_NOT_PAUSED;
        if ((size_t)window->memory_address + byte_count > 0x10000u) {
            return WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT;
        }
        if (wz_debugger_write_memory_bytes(window->machine,
                window->memory_address, bytes, write_mask, byte_count) !=
            WZ_RESULT_OK) {
            return WZ_DEBUGGER_WINDOW_DEBUGGER_ERROR;
        }
        window->memory_address =
            (wz_word_t)(window->memory_address + byte_count);
        return refresh_view(window);
    }
    if (token_count >= 2u && token_count <= 3u &&
        monitor_token_equal(tokens[0], lengths[0], "!")) {
        wz_debugger_snapshot_t snapshot;
        unsigned iff_value = 0u;
        unsigned mode_value = 0u;
        bool change_iff = true;
        bool change_mode = token_count == 3u;
        if (lengths[1] == 2u && tokens[1][0] == '-' && tokens[1][1] == '1') {
            change_iff = false;
        } else if (!monitor_parse_decimal(tokens[1], lengths[1], &iff_value) ||
                   iff_value > 1u) {
            return WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT;
        }
        if (change_mode &&
            (!monitor_parse_decimal(tokens[2], lengths[2], &mode_value) ||
             mode_value > WZ_Z80_INTERRUPT_MODE_2)) {
            return WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT;
        }
        if (!wz_debugger_window_is_open(window)) {
            return WZ_DEBUGGER_WINDOW_NOT_OPEN;
        }
        if (!window->paused) return WZ_DEBUGGER_WINDOW_NOT_PAUSED;
        if (!change_iff && !change_mode) return refresh_view(window);
        if (wz_debugger_snapshot(window->machine, &snapshot) != WZ_RESULT_OK) {
            return WZ_DEBUGGER_WINDOW_DEBUGGER_ERROR;
        }
        if (change_iff) {
            snapshot.cpu.iff1 = (wz_byte_t)iff_value;
            snapshot.cpu.iff2 = (wz_byte_t)iff_value;
        }
        if (change_mode) snapshot.cpu.interrupt_mode = (wz_byte_t)mode_value;
        if (wz_debugger_set_cpu_state(window->machine, &snapshot.cpu) !=
            WZ_RESULT_OK) {
            return WZ_DEBUGGER_WINDOW_DEBUGGER_ERROR;
        }
        return refresh_view(window);
    }
    return WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT;
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
