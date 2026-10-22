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
        fprintf(stderr, "debugger-monitor-fill-contract failed at %s:%d: %s\n", \
                __FILE__, __LINE__, #expression); \
        goto cleanup; \
    } \
} while (0)

int main(void)
{
    wz_machine_t* machine = (wz_machine_t*)calloc(1u, sizeof(*machine));
    wz_debugger_window_t window;
    wz_byte_t original_rom_byte;
    wz_byte_t original_boundary_byte;
    int status = 1;

    if (machine == NULL ||
        wz_machine_init(machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        free(machine);
        return 1;
    }
    wz_debugger_window_init(&window);
    wz_machine_memory_write(machine, 0x7fffu, 0x11u);
    wz_machine_memory_write(machine, 0x8000u, 0x22u);
    wz_machine_memory_write(machine, 0x8001u, 0x33u);
    wz_machine_memory_write(machine, 0x8002u, 0x44u);
    wz_machine_memory_write(machine, 0x8003u, 0x55u);
    CHECK(wz_machine_memory_read(machine, 0x7fffu) == 0x11u &&
          wz_machine_memory_read(machine, 0x8000u) == 0x22u &&
          wz_machine_memory_read(machine, 0x8001u) == 0x33u &&
          wz_machine_memory_read(machine, 0x8002u) == 0x44u &&
          wz_machine_memory_read(machine, 0x8003u) == 0x55u);
    original_rom_byte = wz_machine_memory_read(machine, 0x3fffu);
    original_boundary_byte = wz_machine_memory_read(machine, 0x4000u);
    CHECK(wz_debugger_window_open(&window, machine, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window,
          "K 8000 8002 5A", 14u) == WZ_DEBUGGER_WINDOW_NOT_PAUSED);
    CHECK(wz_machine_memory_read(machine, 0x8000u) == 0x22u &&
          wz_machine_memory_read(machine, 0x8001u) == 0x33u &&
          wz_machine_memory_read(machine, 0x8002u) == 0x44u);
    CHECK(wz_debugger_window_set_paused(&window, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_debugger_window_select_memory(&window, 0x8000u) ==
          WZ_DEBUGGER_WINDOW_OK);

    CHECK(wz_debugger_window_dispatch_monitor_command(&window,
          " k 8000 8002 5a ", 16u) == WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_machine_memory_read(machine, 0x7fffu) == 0x11u &&
          wz_machine_memory_read(machine, 0x8000u) == 0x5au &&
          wz_machine_memory_read(machine, 0x8001u) == 0x5au &&
          wz_machine_memory_read(machine, 0x8002u) == 0x5au &&
          wz_machine_memory_read(machine, 0x8003u) == 0x55u);
    CHECK(wz_debugger_window_memory(&window)[0] == 0x5au &&
          wz_debugger_window_memory(&window)[1] == 0x5au &&
          wz_debugger_window_memory(&window)[2] == 0x5au);

    CHECK(wz_debugger_window_dispatch_monitor_command(&window,
          "K FFFF FFFF A5", 14u) == WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_machine_memory_read(machine, 0xffffu) == 0xa5u);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window,
          "K 0000 0001 A5", 14u) == WZ_DEBUGGER_WINDOW_DEBUGGER_ERROR);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window,
          "K 3FFF 4000 A5", 14u) == WZ_DEBUGGER_WINDOW_DEBUGGER_ERROR);
    CHECK(wz_machine_memory_read(machine, 0x3fffu) == original_rom_byte &&
          wz_machine_memory_read(machine, 0x4000u) == original_boundary_byte);

    CHECK(wz_debugger_window_dispatch_monitor_command(&window,
          "K 8002 8001 A5", 14u) == WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window,
          "K 8000 8001 100", 15u) == WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window,
          "K 8000 8001 GG", 14u) == WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window,
          "K 8000 8001", 11u) == WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window,
          "K 8000 8001 A5 EXTRA", 20u) == WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(wz_machine_memory_read(machine, 0x8000u) == 0x5au &&
          wz_machine_memory_read(machine, 0x8001u) == 0x5au);

    wz_debugger_window_close(&window);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window,
          "K 8000 8000 00", 14u) == WZ_DEBUGGER_WINDOW_NOT_OPEN);
    status = 0;
cleanup:
    wz_debugger_window_close(&window);
    wz_machine_destroy(machine);
    free(machine);
    if (status == 0) {
        printf("debugger-monitor-fill-contract assertions=%u status=pass\n",
               checks);
    }
    return status;
}
