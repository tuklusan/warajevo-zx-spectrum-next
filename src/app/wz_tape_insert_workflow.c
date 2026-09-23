/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_tape_insert_workflow.h"

#include <string.h>

static bool copy_source(char* destination, const char* source)
{
    size_t length;

    if (destination == 0 || source == 0 || source[0] == '\0') {
        return false;
    }
    length = strlen(source);
    if (length >= WZ_TAPE_SOURCE_CAPACITY) {
        return false;
    }
    (void)memcpy(destination, source, length + 1u);
    return true;
}

void wz_tape_insert_workflow_init(wz_tape_insert_workflow_t* workflow,
                                  wz_machine_t* machine)
{
    if (workflow == 0) {
        return;
    }
    memset(workflow, 0, sizeof(*workflow));
    workflow->machine = machine;
}

bool wz_tape_insert_available(const wz_tape_insert_workflow_t* workflow,
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
    if (wz_machine_networking_mode(workflow->machine) == WZ_NETWORKING_EAR_MIC) {
        if (reason != 0) {
            *reason = "tape-transport-unavailable";
        }
        return false;
    }
    return true;
}

wz_tape_insert_result_t wz_tape_insert(
    wz_tape_insert_workflow_t* workflow,
    const char* source,
    const wz_tape_segment_t* segments,
    size_t segment_count)
{
    const char* reason;
    char candidate_source[WZ_TAPE_SOURCE_CAPACITY];

    if (workflow == 0) {
        return WZ_TAPE_INSERT_INVALID_ARGUMENT;
    }
    if (source == 0) {
        return WZ_TAPE_INSERT_CANCELLED;
    }
    if (!copy_source(candidate_source, source)) {
        return WZ_TAPE_INSERT_INVALID_ARGUMENT;
    }
    if (!wz_tape_insert_available(workflow, &reason)) {
        (void)reason;
        return WZ_TAPE_INSERT_UNAVAILABLE;
    }
    if (segments == 0 || segment_count == 0u ||
        wz_machine_mount_tape(workflow->machine, segments, segment_count) !=
            WZ_RESULT_OK) {
        return WZ_TAPE_INSERT_INVALID_MEDIA;
    }
    (void)memcpy(workflow->source, candidate_source, strlen(candidate_source) + 1u);
    workflow->has_source = true;
    return WZ_TAPE_INSERT_OK;
}

const char* wz_tape_insert_source(
    const wz_tape_insert_workflow_t* workflow)
{
    return workflow != 0 && workflow->has_source ? workflow->source : 0;
}
