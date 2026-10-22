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
        fprintf(stderr, "debugger-monitor-undo-contract failed at %s:%d: %s\n", \
                __FILE__, __LINE__, #expression); \
        goto cleanup; \
    } \
} while (0)

#define DISPATCH(text) \
    wz_debugger_window_dispatch_monitor_command(&window, (text), \
                                                 sizeof(text) - 1u)

int main(void)
{
    wz_machine_t* machine = (wz_machine_t*)calloc(1u, sizeof(*machine));
    wz_debugger_window_t window;
    wz_master_tick_t original_tick;
    bool window_needs_close = true;
    int status = 1;

    if (machine == NULL ||
        wz_machine_init(machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        free(machine);
        return 1;
    }
    wz_debugger_window_init(&window);
    wz_machine_memory_write(machine, 0x8000u, 0x5au);
    CHECK(wz_debugger_window_open(&window, machine, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    original_tick = machine->master_tick;
    CHECK(DISPATCH("U") == WZ_DEBUGGER_WINDOW_NOT_PAUSED);
    CHECK(wz_debugger_window_set_paused(&window, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_debugger_window_set_register(&window,
        WZ_DEBUGGER_REGISTER_MAIN_A, 0x42u) == WZ_DEBUGGER_WINDOW_OK);
    CHECK(machine->cpu.main.a == 0x42u && wz_debugger_undo_available(machine));
    CHECK(DISPATCH("U EXTRA") == WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(machine->cpu.main.a == 0x42u && wz_machine_memory_read(machine, 0x8000u) ==
          0x5au && machine->master_tick == original_tick);
    CHECK(wz_debugger_window_set_paused(&window, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(DISPATCH("U") == WZ_DEBUGGER_WINDOW_NOT_PAUSED &&
          machine->cpu.main.a == 0x42u);
    CHECK(wz_debugger_window_set_paused(&window, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(DISPATCH(" U ") == WZ_DEBUGGER_WINDOW_OK);
    CHECK(machine->cpu.main.a == 0u && !wz_debugger_undo_available(machine) &&
          wz_machine_memory_read(machine, 0x8000u) == 0x5au &&
          machine->master_tick == original_tick);
    CHECK(DISPATCH("U") == WZ_DEBUGGER_WINDOW_OK && machine->cpu.main.a == 0u);

    wz_debugger_window_close(&window);
    window_needs_close = false;
    CHECK(DISPATCH("U") == WZ_DEBUGGER_WINDOW_NOT_OPEN);
    status = 0;
cleanup:
    if (window_needs_close) wz_debugger_window_close(&window);
    wz_machine_destroy(machine);
    free(machine);
    if (status == 0) {
        printf("debugger-monitor-undo-contract assertions=%u status=pass\n",
               checks);
    }
    return status;
}
