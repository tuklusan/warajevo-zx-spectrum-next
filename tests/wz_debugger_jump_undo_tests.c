/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>
#include "core/wz_debugger.h"

typedef struct {
    unsigned count;
    wz_trace_event_t events[16u];
} trace_log_t;

static void record_trace(const wz_trace_event_t* event, void* context)
{
    trace_log_t* log = (trace_log_t*)context;
    if (log->count < sizeof(log->events) / sizeof(log->events[0])) {
        log->events[log->count] = *event;
    }
    log->count += 1u;
}

int main(void)
{
    wz_machine_t machine;
    wz_trace_sink_t trace;
    trace_log_t trace_log = {0};
    wz_z80_state_t original;
    wz_z80_state_t changed;
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) return 1;
    wz_trace_sink_init(&trace, record_trace, &trace_log);
    wz_machine_set_timing_trace(&machine, &trace);
    original = machine.cpu;
    changed = original;
    changed.program_counter = 0x2345u;
    changed.main.a = 0x12u;
    changed.main.f = 0x00u;
    if (wz_debugger_set_access_mode(&machine, WZ_DEBUGGER_PAUSED_MUTATION) != WZ_RESULT_OK ||
        wz_debugger_undo_registers(&machine) != WZ_RESULT_NOT_FOUND ||
        wz_debugger_jump(&machine, 0x4321u) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x4321u ||
        wz_debugger_set_cpu_state(&machine, &changed) != WZ_RESULT_OK ||
        !wz_debugger_undo_available(&machine) ||
        wz_debugger_undo_registers(&machine) != WZ_RESULT_OK ||
        wz_debugger_undo_available(&machine) || machine.cpu.program_counter != 0x4321u ||
        machine.cpu.main.a != original.main.a ||
        machine.cpu.main.f != original.main.f ||
        wz_debugger_undo_registers(&machine) != WZ_RESULT_NOT_FOUND) {
        fputs("debugger jump/undo contract failed\n", stderr);
        wz_machine_destroy(&machine);
        return 1;
    }
    if (wz_debugger_set_access_mode(&machine, WZ_DEBUGGER_READ_ONLY) != WZ_RESULT_OK ||
        wz_debugger_undo_registers(&machine) != WZ_RESULT_INVALID_STATE ||
        wz_debugger_jump(&machine, 0u) != WZ_RESULT_INVALID_STATE ||
        trace_log.count != 9u ||
        trace_log.events[1].kind != WZ_TRACE_DEVELOPER_MARKER ||
        trace_log.events[1].auxiliary != WZ_DEBUGGER_TRACE_UNDO ||
        trace_log.events[1].value != WZ_RESULT_NOT_FOUND ||
        trace_log.events[2].auxiliary != WZ_DEBUGGER_TRACE_JUMP ||
        trace_log.events[2].value != WZ_RESULT_OK ||
        trace_log.events[4].auxiliary != WZ_DEBUGGER_TRACE_UNDO ||
        trace_log.events[4].value != WZ_RESULT_OK ||
        trace_log.events[5].auxiliary != WZ_DEBUGGER_TRACE_UNDO ||
        trace_log.events[5].value != WZ_RESULT_NOT_FOUND ||
        trace_log.events[7].auxiliary != WZ_DEBUGGER_TRACE_UNDO ||
        trace_log.events[7].value != WZ_RESULT_INVALID_STATE ||
        trace_log.events[8].auxiliary != WZ_DEBUGGER_TRACE_JUMP ||
        trace_log.events[8].value != WZ_RESULT_INVALID_STATE) {
        fputs("debugger jump access policy failed\n", stderr);
        wz_machine_destroy(&machine);
        return 1;
    }
    wz_machine_destroy(&machine);
    puts("wz_debugger_jump_undo contract passed");
    return 0;
}
