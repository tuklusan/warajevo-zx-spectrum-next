/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>
#include "core/wz_debugger.h"

int main(void)
{
    wz_machine_t machine;
    wz_z80_state_t original;
    wz_z80_state_t changed;
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) return 1;
    original = machine.cpu;
    changed = original;
    changed.program_counter = 0x2345u;
    changed.main.a = 0x12u;
    changed.main.f = 0x00u;
    if (wz_debugger_set_access_mode(&machine, WZ_DEBUGGER_PAUSED_MUTATION) != WZ_RESULT_OK ||
        wz_debugger_jump(&machine, 0x4321u) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x4321u ||
        wz_debugger_set_cpu_state(&machine, &changed) != WZ_RESULT_OK ||
        !wz_debugger_undo_available(&machine) ||
        wz_debugger_undo_registers(&machine) != WZ_RESULT_OK ||
        wz_debugger_undo_available(&machine) || machine.cpu.program_counter != 0x4321u ||
        machine.cpu.main.a != original.main.a ||
        machine.cpu.main.f != original.main.f ||
        wz_debugger_undo_registers(&machine) != WZ_RESULT_INVALID_STATE) {
        fputs("debugger jump/undo contract failed\n", stderr);
        wz_machine_destroy(&machine);
        return 1;
    }
    if (wz_debugger_set_access_mode(&machine, WZ_DEBUGGER_READ_ONLY) != WZ_RESULT_OK ||
        wz_debugger_jump(&machine, 0u) != WZ_RESULT_INVALID_STATE) {
        fputs("debugger jump access policy failed\n", stderr);
        wz_machine_destroy(&machine);
        return 1;
    }
    wz_machine_destroy(&machine);
    puts("wz_debugger_jump_undo contract passed");
    return 0;
}
