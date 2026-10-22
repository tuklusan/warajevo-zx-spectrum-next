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
        fprintf(stderr, "debugger-monitor-memory-write-contract failed at %s:%d: %s\n", \
                __FILE__, __LINE__, #expression); \
        goto cleanup; \
    } \
} while (0)

#define DISPATCH(window, text) \
    wz_debugger_window_dispatch_monitor_command(&(window), (text), \
                                                 sizeof(text) - 1u)

int main(void)
{
    wz_machine_t* machine = (wz_machine_t*)calloc(1u, sizeof(*machine));
    wz_machine_t* banked_machine = NULL;
    wz_debugger_window_t window;
    wz_debugger_window_t banked_window;
    bool window_initialized = false;
    bool banked_initialized = false;
    bool banked_window_initialized = false;
    wz_byte_t original_rom;
    int status = 1;

    if (machine == NULL ||
        wz_machine_init(machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        free(machine);
        return 1;
    }
    wz_debugger_window_init(&window);
    window_initialized = true;
    wz_machine_memory_write(machine, 0x8000u, 0x10u);
    wz_machine_memory_write(machine, 0x8001u, 0x20u);
    wz_machine_memory_write(machine, 0x8002u, 0x30u);
    wz_machine_memory_write(machine, 0x8003u, 0x40u);
    wz_machine_memory_write(machine, 0x9000u, 0x55u);
    original_rom = wz_machine_memory_read(machine, 0x3fffu);
    CHECK(wz_debugger_window_open(&window, machine, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(DISPATCH(window, "M 9000") == WZ_DEBUGGER_WINDOW_OK);
    CHECK(window.memory_address == 0x9000u &&
          wz_debugger_window_memory(&window)[0] == 0x55u);
    CHECK(DISPATCH(window, "M") == WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(DISPATCH(window, "M 8001 11 GG") ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(window.memory_address == 0x9000u &&
          wz_machine_memory_read(machine, 0x8001u) == 0x20u);
    CHECK(wz_debugger_window_set_paused(&window, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(DISPATCH(window, "M 8000 11 -1 33") == WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_machine_memory_read(machine, 0x8000u) == 0x11u &&
          wz_machine_memory_read(machine, 0x8001u) == 0x20u &&
          wz_machine_memory_read(machine, 0x8002u) == 0x33u);
    CHECK(window.memory_address == 0x8003u &&
          wz_debugger_window_memory(&window)[0] == 0x40u);
    CHECK(DISPATCH(window, "M FFFF 01 02") ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(window.memory_address == 0x8003u &&
          wz_machine_memory_read(machine, 0xffffu) == 0u);
    CHECK(DISPATCH(window, "M 3FFF AA BB") ==
          WZ_DEBUGGER_WINDOW_DEBUGGER_ERROR);
    CHECK(window.memory_address == 0x8003u &&
          wz_machine_memory_read(machine, 0x3fffu) == original_rom);

    banked_machine = (wz_machine_t*)calloc(1u, sizeof(*banked_machine));
    CHECK(banked_machine != NULL);
    CHECK(wz_machine_init(banked_machine, wz_machine_profile_128k_pal()) ==
          WZ_RESULT_OK);
    banked_initialized = true;
    wz_debugger_window_init(&banked_window);
    banked_window_initialized = true;
    CHECK(wz_debugger_window_open(&banked_window, banked_machine, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_debugger_set_page_config(banked_machine, 0x07u) ==
          WZ_RESULT_OK);
    wz_machine_memory_write(banked_machine, 0xc000u, 0x70u);
    CHECK(wz_debugger_set_page_config(banked_machine, 0x05u) ==
          WZ_RESULT_OK);
    wz_machine_memory_write(banked_machine, 0xc000u, 0x50u);
    CHECK(DISPATCH(banked_window, "M C000 A1 -1 A3") ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_machine_memory_read(banked_machine, 0xc000u) == 0xa1u &&
          wz_machine_memory_read(banked_machine, 0xc001u) == 0x50u &&
          wz_machine_memory_read(banked_machine, 0xc002u) == 0xa3u);
    CHECK(wz_debugger_set_page_config(banked_machine, 0x07u) ==
          WZ_RESULT_OK);
    CHECK(wz_machine_memory_read(banked_machine, 0xc000u) == 0x70u);

    wz_debugger_window_close(&banked_window);
    status = 0;
cleanup:
    if (banked_window_initialized) wz_debugger_window_close(&banked_window);
    if (banked_initialized) wz_machine_destroy(banked_machine);
    free(banked_machine);
    if (window_initialized) wz_debugger_window_close(&window);
    wz_machine_destroy(machine);
    free(machine);
    if (status == 0) {
        printf("debugger-monitor-memory-write-contract assertions=%u status=pass\n",
               checks);
    }
    return status;
}
