/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_SNAPSHOT_SAVE_WORKFLOW_H
#define WZ_APP_WZ_SNAPSHOT_SAVE_WORKFLOW_H

#include <stdbool.h>
#include <stddef.h>

#define WZ_SNAPSHOT_DESTINATION_CAPACITY 1024u

typedef enum {
    WZ_SNAPSHOT_SAVE_OK = 0,
    WZ_SNAPSHOT_SAVE_INVALID_ARGUMENT,
    WZ_SNAPSHOT_SAVE_NEEDS_DESTINATION,
    WZ_SNAPSHOT_SAVE_CANCELLED,
    WZ_SNAPSHOT_SAVE_INVALID_DESTINATION
} wz_snapshot_save_result_t;

typedef struct {
    char current_destination[WZ_SNAPSHOT_DESTINATION_CAPACITY];
    bool has_current_destination;
} wz_snapshot_save_workflow_t;

void wz_snapshot_save_workflow_init(wz_snapshot_save_workflow_t* workflow);
wz_snapshot_save_result_t wz_snapshot_save_set_destination(
    wz_snapshot_save_workflow_t* workflow,
    const char* destination);
wz_snapshot_save_result_t wz_snapshot_save(
    const wz_snapshot_save_workflow_t* workflow);
wz_snapshot_save_result_t wz_snapshot_save_as(
    wz_snapshot_save_workflow_t* workflow,
    const char* destination);
const char* wz_snapshot_save_current_destination(
    const wz_snapshot_save_workflow_t* workflow);

#endif
