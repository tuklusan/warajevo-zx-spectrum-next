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
    machine->cpu.program_counter = 0x8123u;
    wz_debugger_window_init(&window);
    if (wz_debugger_window_open(&window, machine, false) !=
            WZ_DEBUGGER_WINDOW_OK ||
        wz_debugger_window_set_register(&window, WZ_DEBUGGER_REGISTER_MAIN_A,
                                        0x42u) !=
            WZ_DEBUGGER_WINDOW_NOT_PAUSED ||
        machine->cpu.main.a != 0u) {
        goto cleanup;
    }
    if (wz_debugger_window_set_paused(&window, true) !=
            WZ_DEBUGGER_WINDOW_OK ||
        wz_debugger_window_set_register(&window, WZ_DEBUGGER_REGISTER_MAIN_A,
                                        0x42u) != WZ_DEBUGGER_WINDOW_OK ||
        machine->cpu.main.a != 0x42u ||
        wz_debugger_window_snapshot(&window)->cpu.main.a != 0x42u ||
        !wz_debugger_undo_available(machine)) {
        goto cleanup;
    }
    if (wz_debugger_window_set_register(&window, WZ_DEBUGGER_REGISTER_IFF1,
                                        2u) !=
            WZ_DEBUGGER_WINDOW_DEBUGGER_ERROR ||
        machine->cpu.main.a != 0x42u ||
        wz_debugger_undo_registers(machine) != WZ_RESULT_OK ||
        wz_debugger_window_refresh(&window) != WZ_DEBUGGER_WINDOW_OK ||
        wz_debugger_window_snapshot(&window)->cpu.main.a != 0u) {
        goto cleanup;
    }
    wz_debugger_window_close(&window);
    if (wz_debugger_window_set_register(&window, WZ_DEBUGGER_REGISTER_PC,
                                        0x9000u) !=
        WZ_DEBUGGER_WINDOW_NOT_OPEN) {
        goto cleanup;
    }
    status = 0;
cleanup:
    wz_debugger_window_close(&window);
    wz_machine_destroy(machine);
    free(machine);
    if (status == 0) {
        puts("debugger-register-window-contract status=pass");
    }
    return status;
}
