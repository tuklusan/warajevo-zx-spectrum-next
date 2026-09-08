/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>
#include <string.h>

#include "app/wz_debugger_window.h"

int main(void)
{
    wz_machine_t machine;
    wz_debugger_window_t window;
    wz_command_metadata_t commands[4];
    wz_command_registry_t registry;
    const wz_debugger_snapshot_t* snapshot;

    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        return 1;
    }
    machine.memory[0x5000u] = 0x00u;
    wz_debugger_window_init(&window);
    if (wz_debugger_window_open(&window, &machine, false) != WZ_DEBUGGER_WINDOW_OK ||
        !wz_debugger_window_is_open(&window) ||
        wz_debugger_window_step(&window) != WZ_DEBUGGER_WINDOW_NOT_PAUSED ||
        wz_debugger_window_disassembly(&window) == 0 ||
        strcmp(wz_debugger_window_disassembly(&window), "NOP") != 0) {
        fputs("debugger window read-only presentation failed\n", stderr);
        wz_machine_destroy(&machine);
        return 1;
    }
    snapshot = wz_debugger_window_snapshot(&window);
    if (snapshot == 0 || snapshot->cpu.program_counter != 0u) {
        fputs("debugger window snapshot failed\n", stderr);
        wz_machine_destroy(&machine);
        return 1;
    }
    if (wz_debugger_window_set_paused(&window, true) != WZ_DEBUGGER_WINDOW_OK ||
        wz_debugger_window_write_memory(&window, 0x5000u, 0x3eu) !=
            WZ_DEBUGGER_WINDOW_OK ||
        wz_debugger_window_select_memory(&window, 0x5000u) != WZ_DEBUGGER_WINDOW_OK ||
        wz_debugger_window_disassembly(&window) == 0 ||
        strcmp(wz_debugger_window_disassembly(&window), "LD A,$00") != 0) {
        fputs("debugger window paused mutation failed\n", stderr);
        wz_machine_destroy(&machine);
        return 1;
    }
    if (wz_command_registry_init(&registry, commands, 4u) != WZ_RESULT_OK ||
        wz_debugger_window_register_commands(&registry, 0, 0, 0) != WZ_RESULT_OK ||
        wz_command_registry_finalize(&registry) != WZ_RESULT_OK ||
        wz_command_registry_find(&registry, WZ_DEBUGGER_WINDOW_COMMAND_ID) == 0 ||
        wz_command_registry_find(&registry, WZ_DEBUGGER_WINDOW_STEP_COMMAND_ID) == 0) {
        fputs("debugger window command surface failed\n", stderr);
        wz_machine_destroy(&machine);
        return 1;
    }
    wz_debugger_window_close(&window);
    if (wz_debugger_window_is_open(&window)) {
        wz_machine_destroy(&machine);
        return 1;
    }
    wz_machine_destroy(&machine);
    puts("wz_debugger_window contract passed");
    return 0;
}
