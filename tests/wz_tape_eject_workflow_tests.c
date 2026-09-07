/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <string.h>

#include "app/wz_tape_eject_workflow.h"

int main(void)
{
    const wz_tape_segment_t segment = {8u, 1u};
    wz_machine_t machine;
    wz_tape_eject_workflow_t workflow;
    const char* reason;

    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        return 1;
    }
    wz_tape_eject_workflow_init(&workflow, &machine);
    if (wz_tape_eject_available(&workflow, &reason) || reason == 0 ||
        strcmp(reason, "no-tape-mounted") != 0 ||
        wz_tape_eject(&workflow) != WZ_TAPE_EJECT_UNAVAILABLE ||
        machine.tape_mounted != 0u ||
        wz_machine_mount_tape(&machine, &segment, 1u) != WZ_RESULT_OK ||
        !wz_tape_eject_available(&workflow, &reason) || reason != 0 ||
        wz_tape_eject(&workflow) != WZ_TAPE_EJECT_OK ||
        machine.tape_mounted != 0u || machine.tape.segment_count != 0u ||
        wz_tape_eject(&workflow) != WZ_TAPE_EJECT_UNAVAILABLE) {
        wz_machine_destroy(&machine);
        return 1;
    }
    wz_machine_destroy(&machine);
    return 0;
}
