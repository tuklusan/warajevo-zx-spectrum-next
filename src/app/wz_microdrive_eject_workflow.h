/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#ifndef WZ_APP_WZ_MICRODRIVE_EJECT_WORKFLOW_H
#define WZ_APP_WZ_MICRODRIVE_EJECT_WORKFLOW_H

#include <stdbool.h>
#include <stddef.h>

#include "core/wz_machine.h"

typedef enum {
    WZ_MICRODRIVE_EJECT_SAVE = 0,
    WZ_MICRODRIVE_EJECT_DISCARD,
    WZ_MICRODRIVE_EJECT_CANCEL
} wz_microdrive_eject_decision_t;

/* Cancel leaves the drive untouched and returns OK with ejected set to false. */
wz_result_t wz_microdrive_eject_resolve(
    wz_machine_t* machine,
    size_t slot,
    wz_microdrive_eject_decision_t decision,
    wz_mdr_flush_callback_t persist,
    void* persist_context,
    bool* ejected);

#endif
