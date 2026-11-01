/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#include "app/wz_debugger_window.h"
#include "app/wz_host_machine_frame.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned assertions;

#define CHECK(expression) do { \
    ++assertions; \
    if (!(expression)) { \
        fprintf(stderr, "debugger-monitor-exit-contract failed at %s:%d: %s\n", \
                __FILE__, __LINE__, #expression); \
        goto cleanup; \
    } \
} while (0)

int main(void)
{
    wz_machine_t* machine = (wz_machine_t*)calloc(1u, sizeof(*machine));
    wz_debugger_window_t window;
    wz_headless_runner_t runner;
    wz_z80_state_t before_resume;
    wz_master_tick_t before_tick;
    bool machine_initialized = false;
    int status = 1;

    wz_debugger_window_init(&window);
    if (machine == NULL ||
        wz_machine_init(machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        free(machine);
        return 1;
    }
    machine_initialized = true;
    machine->cpu.program_counter = 0x8000u;
    wz_machine_memory_write(machine, 0x8000u, 0x00u);
    wz_machine_memory_write(machine, 0x8001u, 0x76u);
    CHECK(wz_debugger_window_open(&window, machine, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window,
          "E EXTRA", 7u) == WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window,
          "R SETREG PC 8000", 16u) == WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window,
          "B 8001", 6u) == WZ_DEBUGGER_WINDOW_OK);
    CHECK(window.snapshot.cpu.program_counter == 0x8000u &&
          window.breakpoint_active && window.breakpoint_address == 0x8001u);
    before_resume = machine->cpu;
    before_tick = machine->master_tick;

    CHECK(wz_debugger_window_dispatch_monitor_command(&window,
          " e ", 3u) == WZ_DEBUGGER_WINDOW_RESUME_REQUESTED);
    CHECK(wz_debugger_window_is_open(&window) && window.paused &&
          machine->debugger_access_mode == WZ_DEBUGGER_PAUSED_MUTATION);
    CHECK(memcmp(&machine->cpu, &before_resume, sizeof(before_resume)) == 0 &&
          machine->master_tick == before_tick);
    CHECK(wz_headless_runner_init(&runner, machine, NULL) == WZ_RESULT_OK);

    CHECK(wz_debugger_window_set_paused(&window, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(!window.paused &&
          machine->debugger_access_mode == WZ_DEBUGGER_READ_ONLY);
    CHECK(wz_host_machine_frame_execute(&runner, 256u, false, NULL, NULL) ==
          WZ_RESULT_BREAKPOINT_HIT);
    CHECK(machine->cpu.program_counter == 0x8001u &&
          machine->master_tick > before_tick &&
          wz_debugger_breakpoint_active(machine) &&
          wz_debugger_breakpoint_hit(machine));
    CHECK(wz_machine_memory_read(machine, 0x8000u) == 0x00u &&
          wz_machine_memory_read(machine, 0x8001u) == 0x76u);
    CHECK(wz_debugger_window_is_open(&window) && !window.paused);

    before_tick = machine->master_tick;
    CHECK(wz_debugger_window_pause_at_breakpoint(&window) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(window.paused && window.snapshot.cpu.program_counter == 0x8001u &&
          window.breakpoint_hit && window.breakpoint_active);
    CHECK(machine->master_tick == before_tick);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window,
          "E EXTRA", 7u) == WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window,
          "E", 1u) == WZ_DEBUGGER_WINDOW_RESUME_REQUESTED);
    CHECK(machine->cpu.program_counter == 0x8001u &&
          machine->debugger_breakpoint_hit != 0u);

    CHECK(wz_debugger_window_set_paused(&window, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window,
          "E", 1u) == WZ_DEBUGGER_WINDOW_NOT_PAUSED);
    wz_debugger_window_close(&window);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window,
          "E", 1u) == WZ_DEBUGGER_WINDOW_NOT_OPEN);

    status = 0;
cleanup:
    wz_debugger_window_close(&window);
    if (machine_initialized) wz_machine_destroy(machine);
    free(machine);
    if (status == 0) {
        printf("debugger-monitor-exit-contract assertions=%u status=pass\n",
               assertions);
    }
    return status;
}
