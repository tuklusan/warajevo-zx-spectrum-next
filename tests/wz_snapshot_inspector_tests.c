/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>
#include <string.h>

#include "app/wz_snapshot_inspector.h"
#include "core/wz_machine.h"

int main(void)
{
    wz_machine_t machine;
    wz_snapshot_inspector_t inspector;
    const wz_debugger_snapshot_t* view;
    const wz_debugger_page_info_t* paging;
    char formatted[2048];

    wz_snapshot_inspector_init(&inspector);
    if (wz_snapshot_inspector_is_open(&inspector) ||
        wz_snapshot_inspector_machine(&inspector) != 0 ||
        wz_snapshot_inspector_paging(&inspector) != 0 ||
        wz_snapshot_inspector_open(0, 0) != WZ_SNAPSHOT_INSPECTOR_INVALID_ARGUMENT ||
        wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        return 1;
    }
    if (wz_snapshot_inspector_open(&inspector, &machine) != WZ_SNAPSHOT_INSPECTOR_OK ||
        !wz_snapshot_inspector_is_open(&inspector) ||
        wz_snapshot_inspector_machine(&inspector) == 0 ||
        wz_snapshot_inspector_paging(&inspector) == 0) {
        wz_machine_destroy(&machine);
        return 1;
    }
    view = wz_snapshot_inspector_machine(&inspector);
    paging = wz_snapshot_inspector_paging(&inspector);
    if (view->cpu.program_counter != machine.cpu.program_counter ||
        view->master_tick != machine.master_tick ||
        paging->paging_value != 0u || paging->screen_bank != 5u ||
        paging->rom_bank != 0u || paging->paging_locked != 0u ||
        inspector.model_kind != WZ_MACHINE_48K_PAL ||
        strcmp(inspector.format_name, "live-machine") != 0 ||
        strcmp(inspector.format_version, "runtime") != 0 ||
        inspector.memory_page_count != 3u || inspector.warning_count != 0u ||
        wz_snapshot_inspector_format(&inspector, formatted, sizeof(formatted)) != WZ_RESULT_OK ||
        strstr(formatted, "Format: live-machine/runtime") == 0 ||
        strstr(formatted, "Model: ZX Spectrum 48K PAL") == 0 ||
        strstr(formatted, "Registers:") == 0 || strstr(formatted, "A'=") == 0 ||
        strstr(formatted, "MEMPTR=") == 0 || strstr(formatted, "Paging:") == 0 ||
        strstr(formatted, "AY: selected=0 registers=") == 0 ||
        strstr(formatted, "Memory pages: count=3 present=1 1 1") == 0 ||
        strstr(formatted, "Warnings: 0") == 0) {
        wz_machine_destroy(&machine);
        return 1;
    }
    wz_snapshot_inspector_close(&inspector);
    if (wz_snapshot_inspector_is_open(&inspector) ||
        wz_snapshot_inspector_machine(&inspector) != 0 ||
        wz_snapshot_inspector_paging(&inspector) != 0) {
        wz_machine_destroy(&machine);
        return 1;
    }
    wz_machine_destroy(&machine);
    puts("wz_snapshot_inspector contract passed");
    return 0;
}
