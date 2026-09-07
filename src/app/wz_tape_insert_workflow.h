/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_TAPE_INSERT_WORKFLOW_H
#define WZ_APP_WZ_TAPE_INSERT_WORKFLOW_H

#include <stdbool.h>
#include <stddef.h>

#include "core/wz_machine.h"

#define WZ_TAPE_SOURCE_CAPACITY 1024u

typedef enum {
    WZ_TAPE_INSERT_OK = 0,
    WZ_TAPE_INSERT_INVALID_ARGUMENT,
    WZ_TAPE_INSERT_CANCELLED,
    WZ_TAPE_INSERT_INVALID_MEDIA,
    WZ_TAPE_INSERT_UNAVAILABLE
} wz_tape_insert_result_t;

typedef struct {
    wz_machine_t* machine;
    char source[WZ_TAPE_SOURCE_CAPACITY];
    bool has_source;
} wz_tape_insert_workflow_t;

void wz_tape_insert_workflow_init(wz_tape_insert_workflow_t* workflow,
                                  wz_machine_t* machine);
bool wz_tape_insert_available(const wz_tape_insert_workflow_t* workflow,
                              const char** reason);
wz_tape_insert_result_t wz_tape_insert(
    wz_tape_insert_workflow_t* workflow,
    const char* source,
    const wz_tape_segment_t* segments,
    size_t segment_count);
const char* wz_tape_insert_source(
    const wz_tape_insert_workflow_t* workflow);

#endif
