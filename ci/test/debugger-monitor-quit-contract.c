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
#include <string.h>

static unsigned checks;

#define CHECK(expression) do { \
    ++checks; \
    if (!(expression)) { \
        fprintf(stderr, "debugger-monitor-quit-contract failed at %s:%d: %s\n", \
                __FILE__, __LINE__, #expression); \
        goto cleanup; \
    } \
} while (0)

int main(void)
{
    wz_machine_t* machine = (wz_machine_t*)calloc(1u, sizeof(*machine));
    wz_debugger_window_t window;
    wz_debugger_snapshot_t before;
    wz_debugger_snapshot_t after;
    size_t executed = 99u;
    bool machine_initialized = false;
    bool window_initialized = false;
    int status = 1;

    wz_debugger_window_init(&window);
    window_initialized = true;
    if (machine == NULL ||
        wz_machine_init(machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        free(machine);
        return 1;
    }
    machine_initialized = true;
    wz_machine_memory_write(machine, 0x8000u, 0x00u);
    CHECK(wz_debugger_set_access_mode(machine,
        WZ_DEBUGGER_PAUSED_MUTATION) == WZ_RESULT_OK);
    CHECK(wz_debugger_set_register(machine,
        WZ_DEBUGGER_REGISTER_PC, 0x8000u) == WZ_RESULT_OK);
    CHECK(wz_debugger_set_breakpoint(machine, 0x8000u) == WZ_RESULT_OK);
    CHECK(wz_debugger_continue(machine, 8u, &executed) ==
          WZ_RESULT_BREAKPOINT_HIT);
    CHECK(executed == 0u && wz_debugger_breakpoint_hit(machine));
    CHECK(wz_debugger_window_open(&window, machine, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_debugger_snapshot(machine, &before) == WZ_RESULT_OK);

    CHECK(wz_debugger_window_dispatch_monitor_command(&window, "Q EXTRA", 7u) ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(wz_debugger_window_is_open(&window) && window.paused &&
          machine->debugger_access_mode == WZ_DEBUGGER_PAUSED_MUTATION);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window, " Q ", 3u) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(!wz_debugger_window_is_open(&window) && window.machine == NULL &&
          window.memory_length == 0u &&
          machine->debugger_access_mode == WZ_DEBUGGER_READ_ONLY);
    CHECK(wz_debugger_snapshot(machine, &after) == WZ_RESULT_OK &&
          memcmp(&before.cpu, &after.cpu, sizeof(before.cpu)) == 0 &&
          before.master_tick == after.master_tick &&
          before.border_color == after.border_color &&
          before.networking_mode == after.networking_mode);
    CHECK(wz_machine_memory_read(machine, 0x8000u) == 0x00u &&
          wz_debugger_breakpoint_active(machine) &&
          wz_debugger_breakpoint_hit(machine) &&
          machine->debugger_breakpoint_address == 0x8000u);
    CHECK(wz_debugger_window_open(&window, machine, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(!window.paused && window.snapshot.cpu.program_counter == 0x8000u &&
          wz_debugger_breakpoint_hit(machine));
    wz_debugger_window_close(&window);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window, "Q", 1u) ==
          WZ_DEBUGGER_WINDOW_NOT_OPEN);

    status = 0;
cleanup:
    if (window_initialized) wz_debugger_window_close(&window);
    if (machine_initialized) wz_machine_destroy(machine);
    free(machine);
    if (status == 0) {
        printf("debugger-monitor-quit-contract assertions=%u status=pass\n",
               checks);
    }
    return status;
}
