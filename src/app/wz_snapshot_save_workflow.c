/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_snapshot_save_workflow.h"

#include <string.h>

static bool copy_destination(char* output, const char* destination)
{
    size_t length;

    if (output == NULL || destination == NULL || destination[0] == '\0') {
        return false;
    }
    length = strlen(destination);
    if (length >= WZ_SNAPSHOT_DESTINATION_CAPACITY) {
        return false;
    }
    (void)memcpy(output, destination, length + 1u);
    return true;
}

void wz_snapshot_save_workflow_init(wz_snapshot_save_workflow_t* workflow)
{
    if (workflow != NULL) {
        memset(workflow, 0, sizeof(*workflow));
    }
}

wz_snapshot_save_result_t wz_snapshot_save_set_destination(
    wz_snapshot_save_workflow_t* workflow,
    const char* destination)
{
    if (workflow == NULL) {
        return WZ_SNAPSHOT_SAVE_INVALID_ARGUMENT;
    }
    if (!copy_destination(workflow->current_destination, destination)) {
        return WZ_SNAPSHOT_SAVE_INVALID_DESTINATION;
    }
    workflow->has_current_destination = true;
    return WZ_SNAPSHOT_SAVE_OK;
}

wz_snapshot_save_result_t wz_snapshot_save(
    const wz_snapshot_save_workflow_t* workflow)
{
    if (workflow == NULL) {
        return WZ_SNAPSHOT_SAVE_INVALID_ARGUMENT;
    }
    return workflow->has_current_destination ? WZ_SNAPSHOT_SAVE_OK :
                                               WZ_SNAPSHOT_SAVE_NEEDS_DESTINATION;
}

wz_snapshot_save_result_t wz_snapshot_save_as(
    wz_snapshot_save_workflow_t* workflow,
    const char* destination)
{
    return wz_snapshot_save_set_destination(workflow, destination);
}

const char* wz_snapshot_save_current_destination(
    const wz_snapshot_save_workflow_t* workflow)
{
    return workflow != NULL && workflow->has_current_destination ?
        workflow->current_destination : NULL;
}
