/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>

#include "core/wz_debugger.h"

static int check_48k(void)
{
    wz_machine_t machine;
    wz_debugger_page_info_t info;

    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK ||
        wz_debugger_read_page_info(&machine, &info) != WZ_RESULT_OK ||
        info.paging_value != 0u || info.screen_bank != 5u ||
        info.rom_bank != 0u || info.paging_locked != 0u ||
        wz_debugger_set_page_config(&machine, 0u) !=
            WZ_RESULT_INVALID_STATE ||
        wz_debugger_set_access_mode(&machine, WZ_DEBUGGER_PAUSED_MUTATION) !=
            WZ_RESULT_OK ||
        wz_debugger_set_page_config(&machine, 0u) !=
            WZ_RESULT_UNSUPPORTED_OPERATION) {
        wz_machine_destroy(&machine);
        return 1;
    }
    wz_machine_destroy(&machine);
    return 0;
}

static int check_128k(void)
{
    wz_machine_t machine;
    wz_debugger_page_info_t info;

    if (wz_machine_init(&machine, wz_machine_profile_128k_pal()) != WZ_RESULT_OK ||
        wz_debugger_set_page_config(&machine, 0x2bu) !=
            WZ_RESULT_INVALID_STATE ||
        wz_debugger_set_access_mode(&machine, WZ_DEBUGGER_PAUSED_MUTATION) !=
            WZ_RESULT_OK ||
        wz_debugger_set_page_config(&machine, 0x2bu) != WZ_RESULT_OK ||
        wz_debugger_read_page_info(&machine, &info) != WZ_RESULT_OK ||
        info.paging_value != 0x2bu || info.screen_bank != 7u ||
        info.rom_bank != 0u || info.paging_locked != 1u ||
        wz_debugger_set_page_config(&machine, 0x03u) != WZ_RESULT_OK ||
        wz_machine_128k_paging_value(&machine) != 0x2bu ||
        wz_debugger_set_page_config(&machine, 0x40u) !=
            WZ_RESULT_INVALID_ARGUMENT ||
        wz_machine_128k_paging_value(&machine) != 0x2bu) {
        wz_machine_destroy(&machine);
        return 1;
    }
    wz_machine_destroy(&machine);
    return 0;
}

int main(void)
{
    if (check_48k() != 0 || check_128k() != 0) {
        fputs("debugger page controls contract failed\n", stderr);
        return 1;
    }
    puts("wz_debugger_page_controls contract passed");
    return 0;
}
