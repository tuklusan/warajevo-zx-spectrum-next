/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <string.h>

#include "app/wz_tape_insert_workflow.h"

int main(void)
{
    const wz_tape_segment_t initial = {8u, 1u};
    const wz_tape_segment_t replacement = {12u, 0u};
    const wz_tape_segment_t invalid = {0u, 1u};
    wz_machine_t machine;
    wz_tape_insert_workflow_t workflow;
    const char* reason;
    char too_long[WZ_TAPE_SOURCE_CAPACITY + 1u];

    memset(too_long, 'x', sizeof(too_long));
    too_long[sizeof(too_long) - 1u] = '\0';
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        return 1;
    }
    wz_tape_insert_workflow_init(&workflow, &machine);
    if (!wz_tape_insert_available(&workflow, &reason) || reason != 0 ||
        wz_tape_insert(&workflow, 0, &initial, 1u) != WZ_TAPE_INSERT_CANCELLED ||
        wz_tape_insert_source(&workflow) != 0 || machine.tape_mounted != 0u ||
        wz_tape_insert(&workflow, "first.tap", &initial, 1u) != WZ_TAPE_INSERT_OK ||
        strcmp(wz_tape_insert_source(&workflow), "first.tap") != 0 ||
        machine.tape_mounted == 0u ||
        wz_tape_insert(&workflow, "bad.tap", &invalid, 1u) !=
            WZ_TAPE_INSERT_INVALID_MEDIA ||
        strcmp(wz_tape_insert_source(&workflow), "first.tap") != 0 ||
        machine.tape.segment_count != 1u || machine.tape.segments[0].duration != 8u ||
        wz_tape_insert(&workflow, too_long, &replacement, 1u) !=
            WZ_TAPE_INSERT_INVALID_ARGUMENT ||
        wz_tape_insert(&workflow, "second.tap", &replacement, 1u) != WZ_TAPE_INSERT_OK ||
        strcmp(wz_tape_insert_source(&workflow), "second.tap") != 0 ||
        machine.tape.segments[0].duration != 12u) {
        wz_machine_destroy(&machine);
        return 1;
    }
    if (wz_machine_set_networking_mode(&machine, WZ_NETWORKING_EAR_MIC) !=
            WZ_RESULT_OK ||
        wz_tape_insert_available(&workflow, &reason) || reason == 0 ||
        strcmp(reason, "tape-transport-unavailable") != 0 ||
        wz_tape_insert(&workflow, "blocked.tap", &initial, 1u) !=
            WZ_TAPE_INSERT_UNAVAILABLE ||
        strcmp(wz_tape_insert_source(&workflow), "second.tap") != 0 ||
        machine.tape.segments[0].duration != 12u) {
        wz_machine_destroy(&machine);
        return 1;
    }
    wz_machine_destroy(&machine);
    return 0;
}
