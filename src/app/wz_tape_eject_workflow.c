/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_tape_eject_workflow.h"

#include <string.h>

void wz_tape_eject_workflow_init(wz_tape_eject_workflow_t* workflow,
                                 wz_machine_t* machine)
{
    if (workflow == 0) {
        return;
    }
    memset(workflow, 0, sizeof(*workflow));
    workflow->machine = machine;
}

bool wz_tape_eject_available(const wz_tape_eject_workflow_t* workflow,
                             const char** reason)
{
    if (reason != 0) {
        *reason = 0;
    }
    if (workflow == 0 || workflow->machine == 0) {
        if (reason != 0) {
            *reason = "machine-unavailable";
        }
        return false;
    }
    if (workflow->machine->tape_mounted == 0u) {
        if (reason != 0) {
            *reason = "no-tape-mounted";
        }
        return false;
    }
    return true;
}

wz_tape_eject_result_t wz_tape_eject(wz_tape_eject_workflow_t* workflow)
{
    if (workflow == 0) {
        return WZ_TAPE_EJECT_INVALID_ARGUMENT;
    }
    if (!wz_tape_eject_available(workflow, 0)) {
        return WZ_TAPE_EJECT_UNAVAILABLE;
    }
    return wz_machine_unmount_tape(workflow->machine) == WZ_RESULT_OK
        ? WZ_TAPE_EJECT_OK
        : WZ_TAPE_EJECT_INVALID_ARGUMENT;
}
