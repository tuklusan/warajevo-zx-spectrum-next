/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_snapshot_inspector.h"

#include <string.h>

void wz_snapshot_inspector_init(wz_snapshot_inspector_t* inspector)
{
    if (inspector == 0) {
        return;
    }
    memset(inspector, 0, sizeof(*inspector));
}

wz_snapshot_inspector_result_t wz_snapshot_inspector_open(
    wz_snapshot_inspector_t* inspector,
    const wz_machine_t* machine)
{
    wz_result_t result;

    if (inspector == 0 || machine == 0) {
        return WZ_SNAPSHOT_INSPECTOR_INVALID_ARGUMENT;
    }
    result = wz_debugger_snapshot(machine, &inspector->machine);
    if (result != WZ_RESULT_OK ||
        wz_debugger_read_page_info(machine, &inspector->paging) != WZ_RESULT_OK) {
        wz_snapshot_inspector_close(inspector);
        return WZ_SNAPSHOT_INSPECTOR_DEBUGGER_UNAVAILABLE;
    }
    inspector->open = true;
    return WZ_SNAPSHOT_INSPECTOR_OK;
}

void wz_snapshot_inspector_close(wz_snapshot_inspector_t* inspector)
{
    if (inspector == 0) {
        return;
    }
    memset(inspector, 0, sizeof(*inspector));
}

bool wz_snapshot_inspector_is_open(const wz_snapshot_inspector_t* inspector)
{
    return inspector != 0 && inspector->open;
}

const wz_debugger_snapshot_t* wz_snapshot_inspector_machine(
    const wz_snapshot_inspector_t* inspector)
{
    return wz_snapshot_inspector_is_open(inspector) ? &inspector->machine : 0;
}

const wz_debugger_page_info_t* wz_snapshot_inspector_paging(
    const wz_snapshot_inspector_t* inspector)
{
    return wz_snapshot_inspector_is_open(inspector) ? &inspector->paging : 0;
}
