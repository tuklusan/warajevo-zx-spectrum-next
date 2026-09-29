/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. */

#include "app/wz_tape_save_transaction.h"

#include <stdio.h>

typedef struct {
    bool result;
    size_t calls;
} persist_state_t;

static bool persist(void* context)
{
    persist_state_t* state = (persist_state_t*)context;
    if (state == NULL) return false;
    ++state->calls;
    return state->result;
}

static int fail(const char* message)
{
    (void)fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static bool same_state(const wz_tape_state_t* left,
                       const wz_tape_state_t* right)
{
    return left->tape == right->tape &&
        left->segment_index == right->segment_index &&
        left->segment_elapsed == right->segment_elapsed &&
        left->ear_level == right->ear_level &&
        left->motor_on == right->motor_on && left->at_end == right->at_end;
}

int main(void)
{
    wz_machine_t machine = {0};
    const wz_tape_segment_t original[] = {{7u, 0u, 0u}};
    const wz_tape_segment_t replacement[] = {{11u, 1u, 0u}};
    const wz_tape_segment_t invalid[] = {{0u, 1u, 0u}};
    wz_tape_t saved_tape;
    wz_tape_state_t saved_state;
    wz_byte_t saved_mounted;
    persist_state_t persist_state = {false, 0u};
    if (wz_machine_mount_tape(&machine, original, 1u) != WZ_RESULT_OK)
        return fail("mount initial tape");
    machine.tape_state.segment_elapsed = 3u;
    machine.tape_state.motor_on = true;
    saved_tape = machine.tape;
    saved_state = machine.tape_state;
    saved_mounted = machine.tape_mounted;
    if (wz_tape_save_transaction_commit(&machine, replacement, 1u,
            persist, &persist_state) || persist_state.calls != 1u ||
        machine.tape.segments != saved_tape.segments ||
        machine.tape.segment_count != saved_tape.segment_count ||
        !same_state(&machine.tape_state, &saved_state) ||
        machine.tape_mounted != saved_mounted) {
        return fail("restore mounted tape when atomic persistence fails");
    }
    persist_state.result = true;
    persist_state.calls = 0u;
    if (wz_tape_save_transaction_commit(&machine, invalid, 1u,
            persist, &persist_state) || persist_state.calls != 0u ||
        machine.tape.segments != saved_tape.segments ||
        !same_state(&machine.tape_state, &saved_state)) {
        return fail("reject invalid replacement before persistence");
    }
    if (!wz_tape_save_transaction_commit(&machine, replacement, 1u,
            persist, &persist_state) || persist_state.calls != 1u ||
        machine.tape.segments != replacement ||
        machine.tape_state.tape != &machine.tape ||
        machine.tape_state.segment_index != 0u ||
        machine.tape_state.segment_elapsed != 0u ||
        !machine.tape_mounted) {
        return fail("publish mounted tape only after successful persistence");
    }
    (void)puts("PASS: tape save rolls back in-memory mount on persistence failure");
    return 0;
}
