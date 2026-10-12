/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_debugger_window.h"

#include <stdio.h>
#include <stdlib.h>

#define REQUIRE(expression) do { \
    if (!(expression)) { \
        fprintf(stderr, "failed: %s\n", #expression); \
        return false; \
    } \
} while (0)

static bool verify_live_shared_machine_and_pause_gate(void)
{
    wz_machine_t* machine = (wz_machine_t*)calloc(1u, sizeof(*machine));
    wz_debugger_window_t window;
    wz_byte_t value = 0u;
    bool ok = false;

    if (machine == NULL) return false;
    if (wz_machine_init(machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        free(machine);
        return false;
    }
    machine->cpu.program_counter = 0x4000u;
    wz_debugger_window_init(&window);
    if (wz_debugger_window_open(&window, machine, false) != WZ_DEBUGGER_WINDOW_OK ||
        window.machine != machine ||
        !wz_debugger_window_is_open(&window) ||
        wz_debugger_window_is_paused(&window) ||
        wz_debugger_window_snapshot(&window) == NULL ||
        wz_debugger_window_snapshot(&window)->cpu.program_counter != 0x4000u ||
        wz_debugger_window_write_memory(&window, 0x8000u, 0x5au) !=
            WZ_DEBUGGER_WINDOW_NOT_PAUSED ||
        wz_debugger_window_step(&window) != WZ_DEBUGGER_WINDOW_NOT_PAUSED ||
        wz_debugger_read_memory(machine, 0x8000u, &value) != WZ_RESULT_OK ||
        value != 0u) goto cleanup;

    if (wz_debugger_window_set_paused(&window, true) != WZ_DEBUGGER_WINDOW_OK ||
        !wz_debugger_window_is_paused(&window) ||
        wz_debugger_window_write_memory(&window, 0x8000u, 0x5au) !=
            WZ_DEBUGGER_WINDOW_OK ||
        wz_debugger_read_memory(machine, 0x8000u, &value) != WZ_RESULT_OK ||
        value != 0x5au) goto cleanup;

    wz_debugger_window_close(&window);
    if (wz_debugger_window_is_open(&window) || window.machine != NULL ||
        wz_debugger_window_snapshot(&window) != NULL) goto cleanup;
    ok = true;

cleanup:
    wz_debugger_window_close(&window);
    wz_machine_destroy(machine);
    free(machine);
    REQUIRE(ok);
    return true;
}

int main(void)
{
    if (!verify_live_shared_machine_and_pause_gate()) return 1;
    puts("debugger-window-contract cases=1 status=pass");
    return 0;
}
