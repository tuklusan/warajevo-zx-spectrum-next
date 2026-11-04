/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_debugger_window.h"

#include <stdio.h>
#include <string.h>

static unsigned checks;

typedef struct {
    unsigned sample;
    unsigned accept;
} trace_capture_t;

static void record_trace(const wz_trace_event_t* event, void* context)
{
    trace_capture_t* capture = (trace_capture_t*)context;
    if (event->kind != WZ_TRACE_INTERRUPT) return;
    if (event->value == WZ_TRACE_INTERRUPT_MASKABLE_SAMPLE) ++capture->sample;
    if (event->value == WZ_TRACE_INTERRUPT_MASKABLE_ACCEPT) ++capture->accept;
}

#define CHECK(expression) do { \
    ++checks; \
    if (!(expression)) { \
        fprintf(stderr, "debugger-monitor-interrupt-contract failed at %s:%d: %s\n", \
                __FILE__, __LINE__, #expression); \
        goto cleanup; \
    } \
} while (0)

static int run_mode(wz_z80_interrupt_mode_t mode, wz_word_t expected_pc,
                    wz_master_tick_t expected_ticks)
{
    wz_machine_t machine;
    wz_debugger_window_t window;
    bool initialized = false;
    int status = 1;
    char command[16];
    trace_capture_t trace_capture = {0};
    wz_trace_sink_t trace_sink;

    wz_debugger_window_init(&window);
    CHECK(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
          WZ_RESULT_OK);
    initialized = true;
    wz_trace_sink_init(&trace_sink, record_trace, &trace_capture);
    wz_machine_set_timing_trace(&machine, &trace_sink);
    CHECK(wz_debugger_window_open(&window, &machine, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    machine.cpu.program_counter = 0x8000u;
    machine.cpu.stack_pointer = 0xffffu;
    machine.cpu.i = 0x90u;
    if (mode == WZ_Z80_INTERRUPT_MODE_2) {
        wz_machine_memory_write(&machine, 0x90ffu, 0x34u);
        wz_machine_memory_write(&machine, 0x9100u, 0x12u);
    }

    (void)snprintf(command, sizeof(command), "! 1 %u", (unsigned)mode);
    CHECK(wz_debugger_window_dispatch_monitor_command(
              &window, command, strlen(command)) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(machine.cpu.iff1 == 1u && machine.cpu.iff2 == 1u &&
          machine.cpu.interrupt_mode == (wz_byte_t)mode);
    CHECK(wz_debugger_window_snapshot(&window)->cpu.interrupt_mode ==
          (wz_byte_t)mode);
    CHECK(wz_debugger_window_set_paused(&window, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(!wz_debugger_window_is_paused(&window));
    CHECK(wz_z80_sample_maskable_interrupt(&machine) == WZ_RESULT_OK);
    CHECK(machine.cpu.program_counter == expected_pc);
    CHECK(machine.cpu.stack_pointer == 0xfffdu);
    CHECK(machine.cpu.iff1 == 0u && machine.cpu.iff2 == 0u);
    CHECK(machine.master_tick == expected_ticks);
    CHECK(trace_capture.sample == 1u && trace_capture.accept == 1u);

    wz_debugger_window_close(&window);
    wz_machine_destroy(&machine);
    return 0;

cleanup:
    wz_debugger_window_close(&window);
    if (initialized) wz_machine_destroy(&machine);
    return status;
}

int main(void)
{
    if (run_mode(WZ_Z80_INTERRUPT_MODE_0, 0x0038u, 28u) != 0 ||
        run_mode(WZ_Z80_INTERRUPT_MODE_1, 0x0038u, 26u) != 0 ||
        run_mode(WZ_Z80_INTERRUPT_MODE_2, 0x1234u, 38u) != 0) {
        return 1;
    }
    printf("debugger-monitor-interrupt-contract assertions=%u status=pass\n",
           checks);
    return 0;
}
