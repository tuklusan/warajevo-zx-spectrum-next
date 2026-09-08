/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_DEBUGGER_WINDOW_H
#define WZ_APP_WZ_DEBUGGER_WINDOW_H

#include <stdbool.h>
#include <stddef.h>

#include "app/wz_command_registry.h"
#include "core/wz_debugger.h"

#define WZ_DEBUGGER_WINDOW_COMMAND_ID "tools.debugger"
#define WZ_DEBUGGER_WINDOW_STEP_COMMAND_ID "tools.debugger.step"
#define WZ_DEBUGGER_WINDOW_CONTINUE_COMMAND_ID "tools.debugger.continue"
#define WZ_DEBUGGER_WINDOW_MEMORY_COMMAND_ID "tools.debugger.memory"
#define WZ_DEBUGGER_WINDOW_MEMORY_BYTES 16u
#define WZ_DEBUGGER_WINDOW_DISASSEMBLY_CAPACITY 96u

typedef enum {
    WZ_DEBUGGER_WINDOW_OK = 0,
    WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT,
    WZ_DEBUGGER_WINDOW_NOT_OPEN,
    WZ_DEBUGGER_WINDOW_NOT_PAUSED,
    WZ_DEBUGGER_WINDOW_DEBUGGER_ERROR
} wz_debugger_window_result_t;

typedef struct {
    const wz_machine_t* machine;
    wz_debugger_snapshot_t snapshot;
    wz_debugger_page_info_t paging;
    wz_word_t memory_address;
    wz_byte_t memory[WZ_DEBUGGER_WINDOW_MEMORY_BYTES];
    size_t memory_length;
    char disassembly[WZ_DEBUGGER_WINDOW_DISASSEMBLY_CAPACITY];
    size_t disassembly_length;
    size_t executed;
    bool open;
    bool paused;
    bool trace_view_visible;
} wz_debugger_window_t;

void wz_debugger_window_init(wz_debugger_window_t* window);
wz_debugger_window_result_t wz_debugger_window_open(
    wz_debugger_window_t* window,
    wz_machine_t* machine,
    bool paused);
void wz_debugger_window_close(wz_debugger_window_t* window);
bool wz_debugger_window_is_open(const wz_debugger_window_t* window);
bool wz_debugger_window_is_paused(const wz_debugger_window_t* window);
wz_debugger_window_result_t wz_debugger_window_set_paused(
    wz_debugger_window_t* window,
    bool paused);
wz_debugger_window_result_t wz_debugger_window_refresh(
    wz_debugger_window_t* window);
wz_debugger_window_result_t wz_debugger_window_select_memory(
    wz_debugger_window_t* window,
    wz_word_t address);
wz_debugger_window_result_t wz_debugger_window_write_memory(
    wz_debugger_window_t* window,
    wz_word_t address,
    wz_byte_t value);
wz_debugger_window_result_t wz_debugger_window_step(
    wz_debugger_window_t* window);
wz_debugger_window_result_t wz_debugger_window_continue(
    wz_debugger_window_t* window,
    size_t max_instructions);
void wz_debugger_window_set_trace_view_visible(
    wz_debugger_window_t* window,
    bool visible);
const wz_debugger_snapshot_t* wz_debugger_window_snapshot(
    const wz_debugger_window_t* window);
const wz_debugger_page_info_t* wz_debugger_window_paging(
    const wz_debugger_window_t* window);
const wz_byte_t* wz_debugger_window_memory(
    const wz_debugger_window_t* window);
const char* wz_debugger_window_disassembly(
    const wz_debugger_window_t* window);
wz_result_t wz_debugger_window_register_commands(
    wz_command_registry_t* registry,
    wz_command_availability_fn availability,
    wz_command_handler_fn handler,
    const void* context);

#endif
