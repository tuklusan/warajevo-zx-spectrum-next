/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_TAPE_EJECT_WORKFLOW_H
#define WZ_TAPE_EJECT_WORKFLOW_H

#include <stdbool.h>

#include "core/wz_machine.h"

typedef enum {
    WZ_TAPE_EJECT_OK = 0,
    WZ_TAPE_EJECT_INVALID_ARGUMENT,
    WZ_TAPE_EJECT_UNAVAILABLE
} wz_tape_eject_result_t;

typedef struct {
    wz_machine_t* machine;
} wz_tape_eject_workflow_t;

void wz_tape_eject_workflow_init(wz_tape_eject_workflow_t* workflow,
                                 wz_machine_t* machine);
bool wz_tape_eject_available(const wz_tape_eject_workflow_t* workflow,
                             const char** reason);
wz_tape_eject_result_t wz_tape_eject(wz_tape_eject_workflow_t* workflow);

#endif
