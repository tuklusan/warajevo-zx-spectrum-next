/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdio.h>

#include "core/wz_machine.h"
#include "core/wz_machine_profile.h"
#include "diagnostics/wz_trace_file.h"

typedef struct {
    wz_trace_file_t* file;
    wz_master_tick_t last_bus_tick;
    wz_master_tick_t last_ula_tick;
    wz_qword_t last_bus_sequence;
    bool have_bus;
    bool have_ula;
    bool ordering_failure;
} trace_check_t;

static void record_trace(const wz_trace_event_t* event, void* context)
{
    trace_check_t* check = (trace_check_t*)context;
    if (event->kind == WZ_TRACE_CPU_BUS) {
        if (check->have_ula && check->last_ula_tick == event->master_tick) {
            check->ordering_failure = true;
        }
        check->last_bus_tick = event->master_tick;
        check->last_bus_sequence = event->sequence;
        check->have_bus = true;
    } else if (event->kind == WZ_TRACE_ULA_FETCH) {
        if (check->have_bus && check->last_bus_tick == event->master_tick &&
            check->last_bus_sequence >= event->sequence) {
            check->ordering_failure = true;
        }
        check->last_ula_tick = event->master_tick;
        check->have_ula = true;
        wz_trace_file_emit(event, check->file);
    }
}

int main(int argc, char** argv)
{
    wz_machine_t machine = {0};
    wz_trace_file_t trace = {0};
    wz_trace_sink_t sink;
    trace_check_t check = {0};
    wz_master_tick_t frame_ticks;
    int result = 1;

    if (argc != 2) return 2;
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) !=
        WZ_RESULT_OK) goto cleanup;
    machine.cpu.program_counter = 0x8000u;
    frame_ticks = (wz_master_tick_t)machine.profile->tstates_per_frame *
        machine.profile->master_ticks_per_cpu_tstate;
    if (wz_trace_file_create(&trace, argv[1], 2u,
            (wz_dword_t)WZ_MACHINE_48K_PAL, 0u,
            UINT32_C(1) << WZ_TRACE_ULA_FETCH) != WZ_RESULT_OK) {
        goto cleanup;
    }
    check.file = &trace;
    wz_trace_sink_init(&sink, record_trace, &check);
    wz_machine_set_timing_trace(&machine, &sink);

    while (machine.master_tick < frame_ticks) {
        if (wz_z80_step(&machine) != WZ_RESULT_OK) goto cleanup;
    }
    if (trace.failed || trace.record_count != 12288u ||
        check.ordering_failure) goto cleanup;
    wz_machine_set_timing_trace(&machine, NULL);
    if (wz_trace_file_freeze(&trace) != WZ_RESULT_OK) goto cleanup;
    result = 0;

cleanup:
    wz_machine_set_timing_trace(&machine, NULL);
    if (trace.file != NULL) wz_trace_file_close(&trace);
    wz_machine_destroy(&machine);
    return result;
}
