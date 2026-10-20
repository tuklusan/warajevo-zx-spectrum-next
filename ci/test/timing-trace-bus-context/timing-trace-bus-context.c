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
#include "core/wz_runner.h"
#include "diagnostics/wz_trace_file.h"

typedef struct {
    wz_trace_cpu_state_sync_t cpu;
    wz_trace_event_t first_bus_event;
    wz_qword_t line_assert_sequence;
    wz_qword_t interrupt_sample_sequence;
    wz_qword_t interrupt_accept_sequence;
    wz_master_tick_t last_tick;
    wz_qword_t last_sequence;
    size_t ula_fetch_count;
    size_t cpu_instruction_count;
    bool have_bus_event;
    bool have_previous_event;
    bool timeline_ordered;
    bool bus_context_valid;
    bool interrupt_ordered;
} recovery_t;

static bool recover_cross_domain(const wz_trace_event_t* event, void* opaque)
{
    recovery_t* recovery = (recovery_t*)opaque;

    if (recovery->have_previous_event &&
        (event->sequence <= recovery->last_sequence ||
         event->master_tick < recovery->last_tick)) {
        recovery->timeline_ordered = false;
    }
    recovery->last_sequence = event->sequence;
    recovery->last_tick = event->master_tick;
    recovery->have_previous_event = true;

    if (event->kind == WZ_TRACE_CPU_STATE_SYNC ||
        event->kind == WZ_TRACE_CPU_STATE_DELTA) {
        bool applied = wz_trace_cpu_state_sync_apply(&recovery->cpu, event);
        if ((event->kind == WZ_TRACE_CPU_STATE_DELTA || event->cycle == 4u) &&
            !applied) {
            recovery->timeline_ordered = false;
        }
    } else if (event->kind == WZ_TRACE_CPU_BUS &&
               !recovery->have_bus_event) {
        recovery->first_bus_event = *event;
        recovery->have_bus_event = true;
        recovery->bus_context_valid = recovery->cpu.has_absolute_state &&
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
    } else if (event->kind == WZ_TRACE_CPU_INSTRUCTION) {
        ++recovery->cpu_instruction_count;
    } else if (event->kind == WZ_TRACE_ULA_FETCH) {
        ++recovery->ula_fetch_count;
    } else if (event->kind == WZ_TRACE_INTERRUPT) {
        if (event->value == WZ_TRACE_INTERRUPT_LINE_ASSERT &&
            recovery->line_assert_sequence == 0u) {
            recovery->line_assert_sequence = event->sequence;
        } else if (event->value == WZ_TRACE_INTERRUPT_MASKABLE_SAMPLE &&
                   recovery->interrupt_sample_sequence == 0u) {
            recovery->interrupt_sample_sequence = event->sequence;
        } else if (event->value == WZ_TRACE_INTERRUPT_MASKABLE_ACCEPT &&
                   recovery->interrupt_accept_sequence == 0u) {
            recovery->interrupt_accept_sequence = event->sequence;
        }
    }

    recovery->interrupt_ordered = recovery->line_assert_sequence != 0u &&
        recovery->interrupt_sample_sequence > recovery->line_assert_sequence &&
        recovery->interrupt_accept_sequence >
            recovery->interrupt_sample_sequence;
    return true;
}

int main(int argc, char** argv)
{
    wz_machine_t machine = {0};
    wz_headless_runner_t runner = {0};
    wz_trace_sink_t trace_sink = {0};
    wz_trace_file_t trace_file = {0};
    recovery_t recovery = {0};
    wz_master_tick_t frame_ticks;
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
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.interrupt_mode = (wz_byte_t)WZ_Z80_INTERRUPT_MODE_1;
    frame_ticks = (wz_master_tick_t)machine.profile->tstates_per_frame *
        machine.profile->master_ticks_per_cpu_tstate;

    if (wz_trace_file_create(&trace_file, argv[1], 1u,
            (wz_dword_t)WZ_MACHINE_48K_PAL, 0u, UINT32_MAX) != WZ_RESULT_OK) {
        goto cleanup;
    }
    trace_created = true;
    wz_trace_sink_init(&trace_sink, wz_trace_file_emit, &trace_file);
    wz_machine_set_timing_trace(&machine, &trace_sink);
    if (wz_headless_runner_init(&runner, &machine, NULL) != WZ_RESULT_OK ||
        wz_headless_runner_execute(&runner, frame_ticks) != WZ_RESULT_OK ||
        trace_file.failed ||
        wz_trace_file_freeze(&trace_file) != WZ_RESULT_OK) {
        goto cleanup;
    }
    wz_machine_set_timing_trace(&machine, NULL);
    wz_trace_file_close(&trace_file);

    wz_trace_cpu_state_sync_init(&recovery.cpu);
    recovery.timeline_ordered = true;
    if (wz_trace_file_recover(argv[1], recover_cross_domain, &recovery,
                              &recovered_count) != WZ_RESULT_OK ||
        recovered_count < WZ_ULA_CAPTURE_CELL_COUNT * 2u ||
        recovery.ula_fetch_count != WZ_ULA_CAPTURE_CELL_COUNT * 2u ||
        recovery.cpu_instruction_count == 0u ||
        !recovery.have_bus_event ||
        recovery.first_bus_event.cycle != WZ_BUS_INTERRUPT_ACKNOWLEDGE ||
        recovery.first_bus_event.program_counter != 0x2345u ||
        !recovery.bus_context_valid || !recovery.timeline_ordered ||
        !recovery.interrupt_ordered) {
        (void)fprintf(stderr, "trace failed: events=%zu ula=%zu cpu=%zu bus=%d "
            "cycle=%u pc=%04x context=%d order=%d interrupt=%d\n",
            recovered_count, recovery.ula_fetch_count,
            recovery.cpu_instruction_count, recovery.have_bus_event ? 1 : 0,
            recovery.first_bus_event.cycle,
            (unsigned)recovery.first_bus_event.program_counter,
            recovery.bus_context_valid ? 1 : 0,
            recovery.timeline_ordered ? 1 : 0,
            recovery.interrupt_ordered ? 1 : 0);
        goto cleanup;
    }

    (void)puts("PASS cross-domain trace reconstructs interrupt, CPU, bus, and ULA timing");
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
