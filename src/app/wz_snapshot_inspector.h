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
#include <stddef.h>

#include "core/wz_debugger.h"

#define WZ_SNAPSHOT_INSPECTOR_COMMAND_ID "tools.snapshot_inspector"
#define WZ_SNAPSHOT_INSPECTOR_MAX_WARNINGS 4u

typedef enum {
    WZ_SNAPSHOT_INSPECTOR_OK = 0,
    WZ_SNAPSHOT_INSPECTOR_INVALID_ARGUMENT,
    WZ_SNAPSHOT_INSPECTOR_DEBUGGER_UNAVAILABLE
} wz_snapshot_inspector_result_t;

typedef struct {
    wz_debugger_snapshot_t machine;
    wz_debugger_page_info_t paging;
    const char* format_name;
    const char* format_version;
    const char* model_name;
    wz_machine_kind_t model_kind;
    wz_byte_t ay_selected_register;
    wz_byte_t ay_registers[WZ_AY_REGISTER_COUNT];
    wz_byte_t memory_pages[WZ_128K_RAM_BANK_COUNT];
    size_t memory_page_count;
    size_t warning_count;
    const char* warnings[WZ_SNAPSHOT_INSPECTOR_MAX_WARNINGS];
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
const wz_snapshot_inspector_t* wz_snapshot_inspector_metadata(
    const wz_snapshot_inspector_t* inspector);
wz_result_t wz_snapshot_inspector_format(const wz_snapshot_inspector_t* inspector,
                                        char* output, size_t capacity);

#endif
