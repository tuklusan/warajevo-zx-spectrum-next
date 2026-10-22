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
        fprintf(stderr, "debugger-monitor-poke-contract failed at %s:%d: %s\n", \
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
    wz_byte_t original_boundary;
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
    wz_machine_memory_write(machine, 0x8004u, 0x55u);
    original_rom = wz_machine_memory_read(machine, 0x3fffu);
    original_boundary = wz_machine_memory_read(machine, 0x4000u);
    CHECK(wz_debugger_window_open(&window, machine, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_debugger_window_select_memory(&window, 0x8000u) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(DISPATCH(window, "P 11 22 -1 44") ==
          WZ_DEBUGGER_WINDOW_NOT_PAUSED);
    CHECK(wz_machine_memory_read(machine, 0x8000u) == 0x10u &&
          wz_machine_memory_read(machine, 0x8001u) == 0x20u &&
          wz_machine_memory_read(machine, 0x8002u) == 0x30u &&
          wz_machine_memory_read(machine, 0x8003u) == 0x40u);
    CHECK(wz_debugger_window_set_paused(&window, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(DISPATCH(window, "P 11 22 -1 44") == WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_machine_memory_read(machine, 0x8000u) == 0x11u &&
          wz_machine_memory_read(machine, 0x8001u) == 0x22u &&
          wz_machine_memory_read(machine, 0x8002u) == 0x30u &&
          wz_machine_memory_read(machine, 0x8003u) == 0x44u &&
          wz_machine_memory_read(machine, 0x8004u) == 0x55u);
    CHECK(window.memory_address == 0x8004u &&
          wz_debugger_window_memory(&window)[0] == 0x55u);

    CHECK(DISPATCH(window, "P") == WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(DISPATCH(window, "P 100") == WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(DISPATCH(window, "P GG") == WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(window.memory_address == 0x8004u &&
          wz_machine_memory_read(machine, 0x8004u) == 0x55u);
    window.memory_address = 0xfffcu;
    CHECK(DISPATCH(window, "P AA BB CC DD EE") ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(window.memory_address == 0xfffcu &&
          wz_machine_memory_read(machine, 0xfffcu) == 0u &&
          wz_machine_memory_read(machine, 0xfffdu) == 0u &&
          wz_machine_memory_read(machine, 0xfffeu) == 0u &&
          wz_machine_memory_read(machine, 0xffffu) == 0u);
    CHECK(wz_debugger_window_select_memory(&window, 0x3fffu) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(DISPATCH(window, "P AA BB") ==
          WZ_DEBUGGER_WINDOW_DEBUGGER_ERROR);
    CHECK(wz_machine_memory_read(machine, 0x3fffu) == original_rom &&
          wz_machine_memory_read(machine, 0x4000u) == original_boundary);
    CHECK(DISPATCH(window, "P 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10 11") ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    wz_debugger_window_close(&window);
    CHECK(DISPATCH(window, "P 01") == WZ_DEBUGGER_WINDOW_NOT_OPEN);

    banked_machine = (wz_machine_t*)calloc(1u, sizeof(*banked_machine));
    CHECK(banked_machine != NULL);
    CHECK(wz_machine_init(banked_machine, wz_machine_profile_128k_pal()) ==
          WZ_RESULT_OK);
    banked_initialized = true;
    wz_debugger_window_init(&banked_window);
    banked_window_initialized = true;
    CHECK(wz_debugger_window_open(&banked_window, banked_machine, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_debugger_window_set_paused(&banked_window, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_debugger_set_page_config(banked_machine, 0x07u) == WZ_RESULT_OK);
    wz_machine_memory_write(banked_machine, 0xc000u, 0x80u);
    wz_machine_memory_write(banked_machine, 0xc001u, 0x81u);
    wz_machine_memory_write(banked_machine, 0xc002u, 0x82u);
    wz_machine_memory_write(banked_machine, 0xc003u, 0x83u);
    CHECK(wz_debugger_set_page_config(banked_machine, 0x05u) == WZ_RESULT_OK);
    wz_machine_memory_write(banked_machine, 0xc000u, 0x10u);
    wz_machine_memory_write(banked_machine, 0xc001u, 0x20u);
    wz_machine_memory_write(banked_machine, 0xc002u, 0x30u);
    wz_machine_memory_write(banked_machine, 0xc003u, 0x40u);
    CHECK(wz_debugger_window_select_memory(&banked_window, 0xc000u) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(DISPATCH(banked_window, "P D1 D2 -1 D4") ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(banked_window.memory_address == 0xc004u &&
          wz_machine_memory_read(banked_machine, 0xc000u) == 0xd1u &&
          wz_machine_memory_read(banked_machine, 0xc001u) == 0xd2u &&
          wz_machine_memory_read(banked_machine, 0xc002u) == 0x30u &&
          wz_machine_memory_read(banked_machine, 0xc003u) == 0xd4u);
    CHECK(wz_debugger_set_page_config(banked_machine, 0x07u) == WZ_RESULT_OK);
    CHECK(wz_machine_memory_read(banked_machine, 0xc000u) == 0x80u &&
          wz_machine_memory_read(banked_machine, 0xc001u) == 0x81u &&
          wz_machine_memory_read(banked_machine, 0xc002u) == 0x82u &&
          wz_machine_memory_read(banked_machine, 0xc003u) == 0x83u);
    CHECK(wz_debugger_set_page_config(banked_machine, 0x05u) == WZ_RESULT_OK);
    CHECK(wz_machine_memory_read(banked_machine, 0xc000u) == 0xd1u &&
          wz_machine_memory_read(banked_machine, 0xc001u) == 0xd2u &&
          wz_machine_memory_read(banked_machine, 0xc002u) == 0x30u &&
          wz_machine_memory_read(banked_machine, 0xc003u) == 0xd4u);
    status = 0;

cleanup:
    if (banked_window_initialized) wz_debugger_window_close(&banked_window);
    if (banked_initialized) wz_machine_destroy(banked_machine);
    free(banked_machine);
    if (window_initialized) wz_debugger_window_close(&window);
    wz_machine_destroy(machine);
    free(machine);
    if (status == 0) {
        printf("debugger-monitor-poke-contract assertions=%u status=pass\n",
               checks);
    }
    return status;
}
