/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#include "app/wz_debugger_window.h"

#include <stdio.h>
#include <stdlib.h>

static unsigned checks;

#define CHECK(expression) do { \
    ++checks; \
    if (!(expression)) { \
        fprintf(stderr, "debugger-monitor-command-contract failed at %s:%d: %s\n", \
                __FILE__, __LINE__, #expression); \
        goto cleanup; \
    } \
} while (0)

int main(void)
{
    wz_machine_t* machine = (wz_machine_t*)calloc(1u, sizeof(*machine));
    wz_debugger_window_t window;
    int status = 1;

    if (machine == NULL ||
        wz_machine_init(machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        free(machine);
        return 1;
    }
    wz_debugger_window_init(&window);
    CHECK(wz_debugger_window_open(&window, machine, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window, "! 1 2", 5u) ==
          WZ_DEBUGGER_WINDOW_NOT_PAUSED);
    CHECK(machine->cpu.iff1 == 0u && machine->cpu.iff2 == 0u &&
          machine->cpu.interrupt_mode == WZ_Z80_INTERRUPT_MODE_0);
    CHECK(wz_debugger_window_set_paused(&window, true) ==
          WZ_DEBUGGER_WINDOW_OK);

    CHECK(wz_debugger_window_dispatch_monitor_command(&window, " ! 1 2 ", 7u) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(machine->cpu.iff1 == 1u && machine->cpu.iff2 == 1u &&
          machine->cpu.interrupt_mode == WZ_Z80_INTERRUPT_MODE_2);
    CHECK(wz_debugger_window_snapshot(&window)->cpu.iff1 == 1u &&
          wz_debugger_window_snapshot(&window)->cpu.iff2 == 1u &&
          wz_debugger_window_snapshot(&window)->cpu.interrupt_mode ==
              WZ_Z80_INTERRUPT_MODE_2);
    CHECK(wz_debugger_undo_registers(machine) == WZ_RESULT_OK);
    CHECK(wz_debugger_window_refresh(&window) == WZ_DEBUGGER_WINDOW_OK);
    CHECK(machine->cpu.iff1 == 0u && machine->cpu.iff2 == 0u &&
          machine->cpu.interrupt_mode == WZ_Z80_INTERRUPT_MODE_0);

    CHECK(wz_debugger_window_dispatch_monitor_command(&window, "! -1 1", 6u) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(machine->cpu.iff1 == 0u && machine->cpu.iff2 == 0u &&
          machine->cpu.interrupt_mode == WZ_Z80_INTERRUPT_MODE_1);
    CHECK(wz_debugger_undo_registers(machine) == WZ_RESULT_OK);
    CHECK(wz_debugger_window_refresh(&window) == WZ_DEBUGGER_WINDOW_OK);
    CHECK(machine->cpu.iff1 == 0u && machine->cpu.iff2 == 0u &&
          machine->cpu.interrupt_mode == WZ_Z80_INTERRUPT_MODE_0);

    CHECK(wz_debugger_window_dispatch_monitor_command(&window, "! 1", 3u) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(machine->cpu.iff1 == 1u && machine->cpu.iff2 == 1u &&
          machine->cpu.interrupt_mode == WZ_Z80_INTERRUPT_MODE_0);
    CHECK(wz_debugger_undo_registers(machine) == WZ_RESULT_OK);
    CHECK(wz_debugger_window_refresh(&window) == WZ_DEBUGGER_WINDOW_OK);

    CHECK(wz_debugger_window_dispatch_monitor_command(&window, "! -1", 4u) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(!wz_debugger_undo_available(machine));

    CHECK(wz_debugger_window_dispatch_monitor_command(&window, "! 2 1", 5u) ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window, "! 1 3", 5u) ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window, "! -2 1", 6u) ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window, "! A 1", 5u) ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window,
          "! 1 1 EXTRA", 11u) == WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(machine->cpu.iff1 == 0u && machine->cpu.iff2 == 0u &&
          machine->cpu.interrupt_mode == WZ_Z80_INTERRUPT_MODE_0);

    wz_debugger_window_close(&window);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window, "! 1 2", 5u) ==
          WZ_DEBUGGER_WINDOW_NOT_OPEN);
    status = 0;
cleanup:
    wz_debugger_window_close(&window);
    wz_machine_destroy(machine);
    free(machine);
    if (status == 0) {
        printf("debugger-monitor-command-contract assertions=%u status=pass\n",
               checks);
    }
    return status;
}
