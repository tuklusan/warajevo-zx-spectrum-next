/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and
 * use for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "core/wz_machine.h"
#include "core/wz_machine_profile.h"
#include "diagnostics/wz_trace_file.h"

typedef struct {
    wz_trace_cpu_state_sync_t cpu;
    wz_trace_event_t bus_event;
    bool have_bus_event;
    bool context_valid;
} recovery_t;

static bool recover_context(const wz_trace_event_t* event, void* opaque)
{
    recovery_t* recovery = (recovery_t*)opaque;

    if (event->kind == WZ_TRACE_CPU_STATE_SYNC ||
        event->kind == WZ_TRACE_CPU_STATE_DELTA) {
        if (event->kind == WZ_TRACE_CPU_STATE_DELTA &&
            !wz_trace_cpu_state_sync_apply(&recovery->cpu, event)) {
            recovery->context_valid = false;
        } else {
            (void)wz_trace_cpu_state_sync_apply(&recovery->cpu, event);
        }
    } else if (event->kind == WZ_TRACE_CPU_BUS) {
        recovery->have_bus_event = true;
        recovery->bus_event = *event;
        recovery->context_valid = recovery->cpu.has_absolute_state &&
            recovery->cpu.last_sequence < event->sequence &&
            recovery->cpu.master_tick <= event->master_tick &&
            recovery->cpu.state.program_counter == 0x2345u &&
            recovery->cpu.state.stack_pointer == 0xcafeu &&
            recovery->cpu.state.main.a == 0x11u &&
            recovery->cpu.state.main.f == 0x22u &&
            recovery->cpu.state.main.b == 0x33u &&
            recovery->cpu.state.main.c == 0x44u &&
            recovery->cpu.state.main.d == 0x55u &&
            recovery->cpu.state.main.e == 0x66u &&
            recovery->cpu.state.main.h == 0x77u &&
            recovery->cpu.state.main.l == 0x88u;
    }
    return true;
}

int main(int argc, char** argv)
{
    wz_machine_t machine = {0};
    wz_trace_sink_t trace_sink = {0};
    wz_trace_file_t trace_file = {0};
    wz_bus_request_t request = {0};
    recovery_t recovery = {0};
    size_t recovered_count = 0u;
    bool machine_initialized = false;
    bool trace_created = false;
    int result = 1;

    if (argc != 2 || argv[1][0] == '\0') return 2;
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) !=
        WZ_RESULT_OK) {
        goto cleanup;
    }
    machine_initialized = true;
    machine.cpu.program_counter = 0x2345u;
    machine.cpu.stack_pointer = 0xcafeu;
    machine.cpu.main.a = 0x11u;
    machine.cpu.main.f = 0x22u;
    machine.cpu.main.b = 0x33u;
    machine.cpu.main.c = 0x44u;
    machine.cpu.main.d = 0x55u;
    machine.cpu.main.e = 0x66u;
    machine.cpu.main.h = 0x77u;
    machine.cpu.main.l = 0x88u;
    machine.memory[0x4000u] = 0xa5u;

    if (wz_trace_file_create(&trace_file, argv[1], 1u,
            (wz_dword_t)WZ_MACHINE_48K_PAL, 0u, UINT32_MAX) != WZ_RESULT_OK) {
        goto cleanup;
    }
    trace_created = true;
    wz_trace_sink_init(&trace_sink, wz_trace_file_emit, &trace_file);
    wz_machine_set_timing_trace(&machine, &trace_sink);
    wz_bus_request_init(&request, WZ_BUS_MEMORY_READ, 0u, 0x4000u, 0u, 3u);
    if (wz_machine_bus_request(&machine, &request) != WZ_RESULT_OK ||
        request.value != 0xa5u || trace_file.failed ||
        wz_trace_file_freeze(&trace_file) != WZ_RESULT_OK) {
        goto cleanup;
    }
    wz_machine_set_timing_trace(&machine, NULL);
    wz_trace_file_close(&trace_file);

    wz_trace_cpu_state_sync_init(&recovery.cpu);
    if (wz_trace_file_recover(argv[1], recover_context, &recovery,
                              &recovered_count) != WZ_RESULT_OK ||
        recovered_count < 6u || !recovery.have_bus_event ||
        !recovery.context_valid ||
        recovery.bus_event.address != 0x4000u ||
        recovery.bus_event.value != 0xa5u ||
        recovery.bus_event.program_counter != 0x2345u) {
        goto cleanup;
    }

    (void)puts("PASS trace file reconstructs CPU context at a bus event");
    result = 0;

cleanup:
    if (machine_initialized) {
        wz_machine_set_timing_trace(&machine, NULL);
        wz_machine_destroy(&machine);
    }
    if (trace_file.file != NULL) wz_trace_file_close(&trace_file);
    if (trace_created && remove(argv[1]) != 0) return 1;
    return result;
}
