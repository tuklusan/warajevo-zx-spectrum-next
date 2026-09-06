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
    wz_word_t found = 0u;
    wz_byte_t value = 0u;
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) return 1;
    if (wz_debugger_set_access_mode(&machine, WZ_DEBUGGER_PAUSED_MUTATION) != WZ_RESULT_OK ||
        wz_debugger_fill_memory(&machine, 0x5000u, 4u, 0xa5u) != WZ_RESULT_OK ||
        wz_debugger_write_memory_word(&machine, 0x5004u, 0x1234u) != WZ_RESULT_OK ||
        wz_debugger_find_memory(&machine, 0x5000u, 6u, 0x34u, &found) != WZ_RESULT_OK ||
        found != 0x5004u) {
        fputs("debugger fill/word/find failed\n", stderr);
        wz_machine_destroy(&machine);
        return 1;
    }
    if (wz_debugger_copy_memory(&machine, 0x5000u, 0x5002u, 4u) != WZ_RESULT_OK ||
        wz_debugger_read_memory(&machine, 0x5002u, &value) != WZ_RESULT_OK ||
        value != 0xa5u ||
        wz_debugger_write_memory_word(&machine, 0xffffu, 0x1234u) !=
            WZ_RESULT_INVALID_ARGUMENT ||
        wz_debugger_fill_memory(&machine, 0xfffdu, 4u, 0u) !=
            WZ_RESULT_INVALID_ARGUMENT) {
        fputs("debugger range/copy policy failed\n", stderr);
        wz_machine_destroy(&machine);
        return 1;
    }
    if (wz_debugger_set_access_mode(&machine, WZ_DEBUGGER_READ_ONLY) != WZ_RESULT_OK ||
        wz_debugger_fill_memory(&machine, 0x5000u, 1u, 0u) != WZ_RESULT_INVALID_STATE ||
        wz_debugger_find_memory(&machine, 0x5000u, 1u, 0xffu, &found) != WZ_RESULT_NOT_FOUND) {
        fputs("debugger access policy failed\n", stderr);
        wz_machine_destroy(&machine);
        return 1;
    }
    wz_machine_destroy(&machine);
    puts("wz_debugger_memory_tools contract passed");
    return 0;
}
