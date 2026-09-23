/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_snapshot_workflow.h"

#include <string.h>

void wz_snapshot_workflow_init(wz_snapshot_workflow_t* workflow)
{
    if (workflow != NULL) {
        wz_snapshot_state_init(&workflow->committed);
        wz_snapshot_state_init(&workflow->pending);
        workflow->pending_ready = false;
    }
}

wz_snapshot_workflow_result_t wz_snapshot_workflow_load(
    wz_snapshot_workflow_t* workflow,
    const wz_byte_t* data,
    size_t length)
{
    wz_snapshot_state_t candidate;

    if (workflow == NULL || data == NULL) {
        return WZ_SNAPSHOT_WORKFLOW_INVALID_ARGUMENT;
    }
    wz_snapshot_state_init(&candidate);
    if (wz_snapshot_state_load(&candidate, data, length) != WZ_RESULT_OK) {
        workflow->pending_ready = false;
        return WZ_SNAPSHOT_WORKFLOW_INVALID_SNAPSHOT;
    }
    workflow->pending = candidate;
    workflow->pending_ready = true;
    return WZ_SNAPSHOT_WORKFLOW_OK;
}

wz_snapshot_workflow_result_t wz_snapshot_workflow_commit(
    wz_snapshot_workflow_t* workflow)
{
    if (workflow == NULL) {
        return WZ_SNAPSHOT_WORKFLOW_INVALID_ARGUMENT;
    }
    if (!workflow->pending_ready) {
        return WZ_SNAPSHOT_WORKFLOW_NOT_READY;
    }
    (void)memcpy(&workflow->committed, &workflow->pending,
                 sizeof(workflow->committed));
    workflow->pending_ready = false;
    return WZ_SNAPSHOT_WORKFLOW_OK;
}

const wz_snapshot_state_t* wz_snapshot_workflow_current(
    const wz_snapshot_workflow_t* workflow)
{
    return workflow == NULL ? NULL : &workflow->committed;
}
