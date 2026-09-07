/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <string.h>

#include "app/wz_snapshot_workflow.h"
#include "core/wz_machine.h"
#include "core/wz_machine_profile.h"

int main(void)
{
    wz_snapshot_workflow_t workflow;
    wz_byte_t valid[WZ_STATE_MACHINE_LENGTH];
    wz_byte_t invalid[WZ_STATE_MACHINE_LENGTH];
    wz_machine_t machine;
    wz_state_writer_t writer;

    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        return 1;
    }
    wz_state_writer_init(&writer, valid, sizeof(valid));
    if (wz_state_serialize_machine(&machine, &writer) != WZ_RESULT_OK) {
        wz_machine_destroy(&machine);
        return 1;
    }
    valid[0u] = 13u;
    memset(invalid, 0, sizeof(invalid));
    wz_snapshot_workflow_init(&workflow);
    if (wz_snapshot_workflow_load(&workflow, valid, writer.length) !=
            WZ_SNAPSHOT_WORKFLOW_OK ||
        wz_snapshot_workflow_commit(&workflow) !=
            WZ_SNAPSHOT_WORKFLOW_OK ||
        wz_snapshot_workflow_current(&workflow)->length != writer.length ||
        wz_snapshot_workflow_load(&workflow, invalid, sizeof(invalid)) !=
            WZ_SNAPSHOT_WORKFLOW_INVALID_SNAPSHOT ||
        wz_snapshot_workflow_current(&workflow)->length != writer.length ||
        wz_snapshot_workflow_commit(&workflow) !=
            WZ_SNAPSHOT_WORKFLOW_NOT_READY ||
        wz_snapshot_workflow_current(&workflow)->data[0u] != 13u) {
        wz_machine_destroy(&machine);
        return 1;
    }
    wz_machine_destroy(&machine);
    return 0;
}
