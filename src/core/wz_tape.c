/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_tape.h"

#include <stdint.h>

wz_result_t wz_tape_validate(const wz_tape_segment_t* segments,
                             size_t segment_count)
{
    size_t index;

    if (segments == 0 || segment_count == 0u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    for (index = 0u; index < segment_count; ++index) {
        if (segments[index].duration == 0u || segments[index].ear_level > 1u) {
            return WZ_RESULT_INVALID_ARGUMENT;
        }
    }
    return WZ_RESULT_OK;
}

wz_result_t wz_tape_mount(wz_tape_t* tape,
                          const wz_tape_segment_t* segments,
                          size_t segment_count)
{
    if (tape == 0 || wz_tape_validate(segments, segment_count) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    tape->segments = segments;
    tape->segment_count = segment_count;
    return WZ_RESULT_OK;
}

wz_result_t wz_tape_state_init(wz_tape_state_t* state,
                               const wz_tape_t* tape)
{
    if (state == 0 || tape == 0 || tape->segments == 0 ||
        tape->segment_count == 0u ||
        wz_tape_validate(tape->segments, tape->segment_count) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    state->tape = tape;
    state->segment_index = 0u;
    state->segment_elapsed = 0u;
    state->ear_level = tape->segments[0u].ear_level;
    state->motor_on = false;
    state->at_end = false;
    return WZ_RESULT_OK;
}

wz_result_t wz_tape_state_advance(wz_tape_state_t* state,
                                  wz_master_tick_t ticks)
{
    wz_master_tick_t remaining = ticks;

    if (state == 0 || state->tape == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (!state->motor_on || state->at_end || remaining == 0u) {
        return WZ_RESULT_OK;
    }
    while (remaining != 0u && !state->at_end) {
        const wz_tape_segment_t* segment =
            &state->tape->segments[state->segment_index];
        wz_master_tick_t available = segment->duration - state->segment_elapsed;

        if (remaining < available) {
            state->segment_elapsed += remaining;
            remaining = 0u;
            continue;
        }
        remaining -= available;
        ++state->segment_index;
        state->segment_elapsed = 0u;
        if (state->segment_index >= state->tape->segment_count) {
            state->segment_index = state->tape->segment_count - 1u;
            state->ear_level = segment->ear_level;
            state->at_end = true;
        } else {
            state->ear_level = state->tape->segments[state->segment_index].ear_level;
        }
    }
    return WZ_RESULT_OK;
}

wz_result_t wz_tape_state_rewind(wz_tape_state_t* state)
{
    if (state == 0 || state->tape == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    state->segment_index = 0u;
    state->segment_elapsed = 0u;
    state->ear_level = state->tape->segments[0u].ear_level;
    state->at_end = false;
    return WZ_RESULT_OK;
}

wz_result_t wz_tape_state_set_motor(wz_tape_state_t* state, bool motor_on)
{
    if (state == 0 || state->tape == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    state->motor_on = motor_on && !state->at_end;
    return WZ_RESULT_OK;
}

wz_byte_t wz_tape_state_ear_level(const wz_tape_state_t* state)
{
    return state == 0 ? 0u : state->ear_level;
}

bool wz_tape_state_at_end(const wz_tape_state_t* state)
{
    return state != 0 && state->at_end;
}
