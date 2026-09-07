/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <string.h>

#include "app/wz_snapshot_save_workflow.h"

int main(void)
{
    wz_snapshot_save_workflow_t workflow;
    char too_long[WZ_SNAPSHOT_DESTINATION_CAPACITY + 1u];

    wz_snapshot_save_workflow_init(&workflow);
    memset(too_long, 'x', sizeof(too_long));
    too_long[sizeof(too_long) - 1u] = '\0';
    if (wz_snapshot_save(&workflow) != WZ_SNAPSHOT_SAVE_NEEDS_DESTINATION ||
        wz_snapshot_save_as(&workflow, "first.sna") != WZ_SNAPSHOT_SAVE_OK ||
        wz_snapshot_save(&workflow) != WZ_SNAPSHOT_SAVE_OK ||
        strcmp(wz_snapshot_save_current_destination(&workflow), "first.sna") != 0 ||
        wz_snapshot_save_as(&workflow, "") != WZ_SNAPSHOT_SAVE_INVALID_DESTINATION ||
        strcmp(wz_snapshot_save_current_destination(&workflow), "first.sna") != 0 ||
        wz_snapshot_save_as(&workflow, too_long) != WZ_SNAPSHOT_SAVE_INVALID_DESTINATION ||
        strcmp(wz_snapshot_save_current_destination(&workflow), "first.sna") != 0 ||
        wz_snapshot_save_set_destination(&workflow, NULL) !=
            WZ_SNAPSHOT_SAVE_INVALID_DESTINATION) {
        return 1;
    }
    return 0;
}
