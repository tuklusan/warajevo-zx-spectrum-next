/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_DIAGNOSTICS_WINDOW_H
#define WZ_APP_WZ_DIAGNOSTICS_WINDOW_H

#include <stdbool.h>

#include "app/wz_command_registry.h"
#include "core/wz_machine.h"
#include "diagnostics/wz_diagnostics_router.h"
#include "diagnostics/wz_trace_file.h"

#define WZ_DIAGNOSTICS_WINDOW_COMMAND_ID "tools.diagnostics"

typedef enum {
    WZ_DIAGNOSTICS_WINDOW_OK = 0,
    WZ_DIAGNOSTICS_WINDOW_INVALID_ARGUMENT,
    WZ_DIAGNOSTICS_WINDOW_NOT_OPEN
} wz_diagnostics_window_result_t;

typedef struct {
    const wz_machine_t* machine;
    const wz_trace_file_t* trace;
    const wz_diagnostics_router_t* router;
    wz_qword_t first_sequence;
    wz_qword_t last_sequence;
    wz_qword_t generation;
    bool trace_available;
    bool trace_frozen;
    bool trace_failed;
    bool forwarding_enabled;
    bool diagnostic_block_available;
    bool open;
} wz_diagnostics_window_t;

void wz_diagnostics_window_init(wz_diagnostics_window_t* window);
wz_diagnostics_window_result_t wz_diagnostics_window_open(
    wz_diagnostics_window_t* window,
    const wz_machine_t* machine,
    const wz_trace_file_t* trace,
    const wz_diagnostics_router_t* router);
void wz_diagnostics_window_close(wz_diagnostics_window_t* window);
bool wz_diagnostics_window_is_open(const wz_diagnostics_window_t* window);
wz_diagnostics_window_result_t wz_diagnostics_window_refresh(
    wz_diagnostics_window_t* window);
const char* wz_diagnostics_window_trace_reason(
    const wz_diagnostics_window_t* window);
const char* wz_diagnostics_window_forwarding_reason(
    const wz_diagnostics_window_t* window);
wz_result_t wz_diagnostics_window_register_commands(
    wz_command_registry_t* registry,
    wz_command_availability_fn availability,
    wz_command_handler_fn handler,
    const void* context);

#endif
