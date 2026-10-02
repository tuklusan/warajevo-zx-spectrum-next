/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#include "app/wz_microdrive_eject_workflow.h"

wz_result_t wz_microdrive_eject_resolve(
    wz_machine_t* machine,
    size_t slot,
    wz_microdrive_eject_decision_t decision,
    wz_mdr_flush_callback_t persist,
    void* persist_context,
    bool* ejected)
{
    wz_mdr_transport_t* transport;
    wz_result_t result;

    if (ejected != 0) *ejected = false;
    if (machine == 0 || ejected == 0 ||
        slot >= WZ_MACHINE_MICRODRIVE_COUNT ||
        (decision != WZ_MICRODRIVE_EJECT_SAVE &&
         decision != WZ_MICRODRIVE_EJECT_DISCARD &&
         decision != WZ_MICRODRIVE_EJECT_CANCEL)) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (decision == WZ_MICRODRIVE_EJECT_CANCEL) return WZ_RESULT_OK;

    transport = wz_machine_microdrive_at(machine, slot);
    if (transport == 0 || transport->image_present == 0u) {
        return WZ_RESULT_INVALID_STATE;
    }
    if (decision == WZ_MICRODRIVE_EJECT_SAVE &&
        wz_mdr_transport_is_dirty(transport) != 0u) {
        if (persist == 0) return WZ_RESULT_INVALID_STATE;
        result = wz_mdr_transport_flush(transport, persist, persist_context);
        if (result != WZ_RESULT_OK) return result;
    }
    result = wz_machine_eject_microdrive(machine, slot + 1u,
        decision == WZ_MICRODRIVE_EJECT_DISCARD);
    if (result == WZ_RESULT_OK) *ejected = true;
    return result;
}
