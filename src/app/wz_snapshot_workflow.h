/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_SNAPSHOT_WORKFLOW_H
#define WZ_APP_WZ_SNAPSHOT_WORKFLOW_H

#include <stdbool.h>

#include "core/wz_state.h"

typedef enum {
    WZ_SNAPSHOT_WORKFLOW_OK = 0,
    WZ_SNAPSHOT_WORKFLOW_INVALID_ARGUMENT,
    WZ_SNAPSHOT_WORKFLOW_INVALID_SNAPSHOT,
    WZ_SNAPSHOT_WORKFLOW_NOT_READY
} wz_snapshot_workflow_result_t;

typedef struct {
    wz_snapshot_state_t committed;
    wz_snapshot_state_t pending;
    bool pending_ready;
} wz_snapshot_workflow_t;

void wz_snapshot_workflow_init(wz_snapshot_workflow_t* workflow);
wz_snapshot_workflow_result_t wz_snapshot_workflow_load(
    wz_snapshot_workflow_t* workflow,
    const wz_byte_t* data,
    size_t length);
wz_snapshot_workflow_result_t wz_snapshot_workflow_commit(
    wz_snapshot_workflow_t* workflow);
const wz_snapshot_state_t* wz_snapshot_workflow_current(
    const wz_snapshot_workflow_t* workflow);

#endif
