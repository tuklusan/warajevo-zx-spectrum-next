/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. */

#include "app/wz_tape_save_transaction.h"

bool wz_tape_save_transaction_commit(
    wz_machine_t* machine, const wz_tape_segment_t* segments,
    size_t segment_count, wz_tape_save_persist_fn persist, void* context)
{
    wz_tape_t previous_tape;
    wz_tape_state_t previous_state;
    wz_byte_t previous_mounted;
    if (machine == NULL || segments == NULL || segment_count == 0u ||
        persist == NULL) return false;
    previous_tape = machine->tape;
    previous_state = machine->tape_state;
    previous_mounted = machine->tape_mounted;
    if (wz_machine_mount_tape(machine, segments, segment_count) != WZ_RESULT_OK) {
        machine->tape = previous_tape;
        machine->tape_state = previous_state;
        machine->tape_mounted = previous_mounted;
        return false;
    }
    if (!persist(context)) {
        machine->tape = previous_tape;
        machine->tape_state = previous_state;
        machine->tape_mounted = previous_mounted;
        return false;
    }
    return true;
}
