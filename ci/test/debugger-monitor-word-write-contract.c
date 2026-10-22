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
        fprintf(stderr, "debugger-monitor-word-write-contract failed at %s:%d: %s\n", \
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
    wz_byte_t rom_before;
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
    rom_before = wz_machine_memory_read(machine, 0x3fffu);
    CHECK(wz_debugger_window_open(&window, machine, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    window.memory_address = 0x8000u;
    CHECK(DISPATCH(window, "W") == WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(DISPATCH(window, "W 1234") == WZ_DEBUGGER_WINDOW_NOT_PAUSED);
    CHECK(wz_machine_memory_read(machine, 0x8000u) == 0x10u);
    CHECK(wz_debugger_window_set_paused(&window, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(DISPATCH(window, "W 1234 ABCD") == WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_machine_memory_read(machine, 0x8000u) == 0x34u &&
          wz_machine_memory_read(machine, 0x8001u) == 0x12u &&
          wz_machine_memory_read(machine, 0x8002u) == 0xcdu &&
          wz_machine_memory_read(machine, 0x8003u) == 0xabu);
    CHECK(window.memory_address == 0x8004u);
    CHECK(DISPATCH(window, "W 1 GG") == WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(DISPATCH(window, "W 12345") == WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(window.memory_address == 0x8004u &&
          wz_machine_memory_read(machine, 0x8004u) == 0u);
    window.memory_address = 0x3ffeu;
    CHECK(DISPATCH(window, "W 1111 2222") ==
          WZ_DEBUGGER_WINDOW_DEBUGGER_ERROR);
    CHECK(window.memory_address == 0x3ffeu &&
          wz_machine_memory_read(machine, 0x3fffu) == rom_before);
    window.memory_address = 0xfffeu;
    CHECK(DISPATCH(window, "W BEEF") == WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_machine_memory_read(machine, 0xfffeu) == 0xefu &&
          wz_machine_memory_read(machine, 0xffffu) == 0xbeu &&
          window.memory_address == 0u);

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
    wz_machine_memory_write(banked_machine, 0xc001u, 0x71u);
    CHECK(wz_debugger_set_page_config(banked_machine, 0x05u) ==
          WZ_RESULT_OK);
    wz_machine_memory_write(banked_machine, 0xc000u, 0x50u);
    wz_machine_memory_write(banked_machine, 0xc001u, 0x51u);
    banked_window.memory_address = 0xc000u;
    CHECK(DISPATCH(banked_window, "W A1B2") == WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_machine_memory_read(banked_machine, 0xc000u) == 0xb2u &&
          wz_machine_memory_read(banked_machine, 0xc001u) == 0xa1u);
    CHECK(wz_debugger_set_page_config(banked_machine, 0x07u) ==
          WZ_RESULT_OK);
    CHECK(wz_machine_memory_read(banked_machine, 0xc000u) == 0x70u &&
          wz_machine_memory_read(banked_machine, 0xc001u) == 0x71u);
    CHECK(wz_debugger_set_page_config(banked_machine, 0x05u) ==
          WZ_RESULT_OK);
    CHECK(wz_machine_memory_read(banked_machine, 0xc000u) == 0xb2u &&
          wz_machine_memory_read(banked_machine, 0xc001u) == 0xa1u);

    status = 0;
cleanup:
    if (banked_window_initialized) wz_debugger_window_close(&banked_window);
    if (banked_initialized) wz_machine_destroy(banked_machine);
    free(banked_machine);
    if (window_initialized) wz_debugger_window_close(&window);
    wz_machine_destroy(machine);
    free(machine);
    if (status == 0) {
        printf("debugger-monitor-word-write-contract assertions=%u status=pass\n",
               checks);
    }
    return status;
}
