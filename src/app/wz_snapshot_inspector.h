/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_SNAPSHOT_INSPECTOR_H
#define WZ_APP_WZ_SNAPSHOT_INSPECTOR_H

#include <stdbool.h>

#include "core/wz_debugger.h"

#define WZ_SNAPSHOT_INSPECTOR_COMMAND_ID "tools.snapshot_inspector"

typedef enum {
    WZ_SNAPSHOT_INSPECTOR_OK = 0,
    WZ_SNAPSHOT_INSPECTOR_INVALID_ARGUMENT,
    WZ_SNAPSHOT_INSPECTOR_DEBUGGER_UNAVAILABLE
} wz_snapshot_inspector_result_t;

typedef struct {
    wz_debugger_snapshot_t machine;
    wz_debugger_page_info_t paging;
    bool open;
} wz_snapshot_inspector_t;

void wz_snapshot_inspector_init(wz_snapshot_inspector_t* inspector);
wz_snapshot_inspector_result_t wz_snapshot_inspector_open(
    wz_snapshot_inspector_t* inspector,
    const wz_machine_t* machine);
void wz_snapshot_inspector_close(wz_snapshot_inspector_t* inspector);
bool wz_snapshot_inspector_is_open(const wz_snapshot_inspector_t* inspector);
const wz_debugger_snapshot_t* wz_snapshot_inspector_machine(
    const wz_snapshot_inspector_t* inspector);
const wz_debugger_page_info_t* wz_snapshot_inspector_paging(
    const wz_snapshot_inspector_t* inspector);

#endif
