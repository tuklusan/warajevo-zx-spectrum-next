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
        fprintf(stderr, "debugger-monitor-copy-contract failed at %s:%d: %s\n", \
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
    wz_machine_memory_write(machine, 0x8020u, 1u);
    wz_machine_memory_write(machine, 0x8021u, 2u);
    wz_machine_memory_write(machine, 0x8022u, 3u);
    wz_machine_memory_write(machine, 0x8023u, 4u);
    wz_machine_memory_write(machine, 0xfffeu, 0xbeu);
    wz_machine_memory_write(machine, 0xffffu, 0xefu);
    wz_machine_memory_write(machine, 0x8000u, 0x5au);
    rom_before = wz_machine_memory_read(machine, 0x3fffu);
    CHECK(wz_debugger_window_open(&window, machine, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(DISPATCH(window, "C 8020 8010 3") ==
          WZ_DEBUGGER_WINDOW_NOT_PAUSED);
    CHECK(wz_machine_memory_read(machine, 0x8010u) == 0u);
    CHECK(wz_debugger_window_set_paused(&window, true) ==
          WZ_DEBUGGER_WINDOW_OK);

    CHECK(DISPATCH(window, "C 8020 8021 3") == WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_machine_memory_read(machine, 0x8020u) == 1u &&
          wz_machine_memory_read(machine, 0x8021u) == 1u &&
          wz_machine_memory_read(machine, 0x8022u) == 2u &&
          wz_machine_memory_read(machine, 0x8023u) == 3u);
    CHECK(window.memory_address == 0x8021u);

    CHECK(DISPATCH(window, "C 8020 8010 0") == WZ_DEBUGGER_WINDOW_OK);
    CHECK(window.memory_address == 0x8010u &&
          wz_machine_memory_read(machine, 0x8010u) == 0u);
    CHECK(DISPATCH(window, "C 8020 8010 -1") ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(DISPATCH(window, "C 8020 8010 65537") ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(DISPATCH(window, "C 8020 8010 1x") ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(DISPATCH(window, "C 10000 8010 1") ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(DISPATCH(window, "C FFFE FFFF 2") ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(DISPATCH(window, "C FFFF FFFC 2") ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(wz_machine_memory_read(machine, 0xffffu) == 0xefu &&
          wz_machine_memory_read(machine, 0xfffcu) == 0u &&
          window.memory_address == 0x8010u);

    CHECK(DISPATCH(window, "C FFFE FFFC 2") == WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_machine_memory_read(machine, 0xfffcu) == 0xbeu &&
          wz_machine_memory_read(machine, 0xfffdu) == 0xefu &&
          window.memory_address == 0xfffcu);
    CHECK(DISPATCH(window, "C 0000 8100 1") == WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_machine_memory_read(machine, 0x8100u) ==
          wz_machine_memory_read(machine, 0x0000u));
    CHECK(DISPATCH(window, "C 8000 3FFF 1") ==
          WZ_DEBUGGER_WINDOW_DEBUGGER_ERROR);
    CHECK(wz_machine_memory_read(machine, 0x3fffu) == rom_before &&
          wz_machine_memory_read(machine, 0x8000u) == 0x5au);
    CHECK(DISPATCH(window, "C 8000 8010") ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    wz_debugger_window_close(&window);
    CHECK(DISPATCH(window, "C 8000 8010 1") ==
          WZ_DEBUGGER_WINDOW_NOT_OPEN);

    banked_machine = (wz_machine_t*)calloc(1u, sizeof(*banked_machine));
    CHECK(banked_machine != NULL);
    CHECK(wz_machine_init(banked_machine, wz_machine_profile_128k_pal()) ==
          WZ_RESULT_OK);
    banked_initialized = true;
    wz_debugger_window_init(&banked_window);
    banked_window_initialized = true;
    CHECK(wz_debugger_window_open(&banked_window, banked_machine, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_debugger_set_page_config(banked_machine, 0x05u) == WZ_RESULT_OK);
    wz_machine_memory_write(banked_machine, 0xc000u, 0x51u);
    wz_machine_memory_write(banked_machine, 0xc001u, 0x52u);
    wz_machine_memory_write(banked_machine, 0xc010u, 0x5fu);
    CHECK(wz_debugger_set_page_config(banked_machine, 0x07u) == WZ_RESULT_OK);
    wz_machine_memory_write(banked_machine, 0xc000u, 0x71u);
    wz_machine_memory_write(banked_machine, 0xc001u, 0x72u);
    wz_machine_memory_write(banked_machine, 0xc010u, 0x7fu);
    CHECK(wz_debugger_set_page_config(banked_machine, 0x05u) == WZ_RESULT_OK);
    CHECK(DISPATCH(banked_window, "C C000 C010 2") ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_machine_memory_read(banked_machine, 0xc010u) == 0x51u &&
          wz_machine_memory_read(banked_machine, 0xc011u) == 0x52u);
    CHECK(wz_debugger_set_page_config(banked_machine, 0x07u) == WZ_RESULT_OK);
    CHECK(wz_machine_memory_read(banked_machine, 0xc010u) == 0x7fu);

    status = 0;
cleanup:
    if (banked_window_initialized) wz_debugger_window_close(&banked_window);
    if (banked_initialized) wz_machine_destroy(banked_machine);
    free(banked_machine);
    if (window_initialized) wz_debugger_window_close(&window);
    wz_machine_destroy(machine);
    free(machine);
    if (status == 0) {
        printf("debugger-monitor-copy-contract assertions=%u status=pass\n",
               checks);
    }
    return status;
}
