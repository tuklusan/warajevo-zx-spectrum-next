/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_tape.h"

#include <limits.h>
#include <stdint.h>

#define WZ_TAP_PILOT_TSTATES 2168u
#define WZ_TAP_SYNC_FIRST_TSTATES 667u
#define WZ_TAP_SYNC_SECOND_TSTATES 735u
#define WZ_TAP_ZERO_TSTATES 855u
#define WZ_TAP_ONE_TSTATES 1710u
#define WZ_TAP_PAUSE_TSTATES 3500000u

static wz_result_t wz_tape_tap_add_count(size_t* total, size_t amount)
{
    if (total == 0 || amount > SIZE_MAX - *total) {
        return WZ_RESULT_PARSE_ERROR;
    }
    *total += amount;
    return WZ_RESULT_OK;
}

static wz_result_t wz_tape_tap_duration(wz_dword_t tstates,
                                        wz_dword_t ticks_per_tstate,
                                        wz_master_tick_t* duration)
{
    if (duration == 0 || ticks_per_tstate == 0u ||
        (wz_master_tick_t)tstates > UINT64_MAX / ticks_per_tstate) {
        return WZ_RESULT_PARSE_ERROR;
    }
    *duration = (wz_master_tick_t)tstates * ticks_per_tstate;
    return WZ_RESULT_OK;
}

static wz_result_t wz_tape_tap_count_block(const wz_byte_t* block,
                                           size_t block_length,
                                           size_t* total)
{
    size_t pilot_count;
    size_t data_pulse_count;

    if (block == 0 || total == 0 || block_length < 2u) {
        return WZ_RESULT_PARSE_ERROR;
    }
    pilot_count = block[0u] == 0u ? 8063u : 3223u;
    if (block_length > (SIZE_MAX - pilot_count - 3u) / 16u) {
        return WZ_RESULT_PARSE_ERROR;
    }
    data_pulse_count = block_length * 16u;
    if (wz_tape_tap_add_count(total, pilot_count) != WZ_RESULT_OK ||
        wz_tape_tap_add_count(total, 2u) != WZ_RESULT_OK ||
        wz_tape_tap_add_count(total, data_pulse_count) != WZ_RESULT_OK ||
        wz_tape_tap_add_count(total, 1u) != WZ_RESULT_OK) {
        return WZ_RESULT_PARSE_ERROR;
    }
    return WZ_RESULT_OK;
}

static wz_result_t wz_tape_tap_validate(const wz_byte_t* data,
                                        size_t length,
                                        size_t* count)
{
    size_t offset = 0u;
    size_t total = 0u;

    if (data == 0 || length == 0u || count == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    while (offset < length) {
        size_t block_length;
        wz_byte_t checksum = 0u;

        if (length - offset < 2u) {
            return WZ_RESULT_PARSE_ERROR;
        }
        block_length = (size_t)wz_read_le16(&data[offset]);
        offset += 2u;
        if (block_length < 2u || block_length > length - offset) {
            return WZ_RESULT_PARSE_ERROR;
        }
        for (size_t index = 0u; index < block_length; ++index) {
            checksum ^= data[offset + index];
        }
        if (checksum != 0u ||
            wz_tape_tap_count_block(&data[offset], block_length, &total) != WZ_RESULT_OK) {
            return WZ_RESULT_PARSE_ERROR;
        }
        offset += block_length;
    }
    *count = total;
    return WZ_RESULT_OK;
}

static wz_result_t wz_tape_tap_append(wz_tape_segment_t* segments,
                                      size_t capacity,
                                      size_t* index,
                                      wz_dword_t tstates,
                                      wz_dword_t ticks_per_tstate,
                                      wz_byte_t level)
{
    wz_master_tick_t duration;

    if (wz_tape_tap_duration(tstates, ticks_per_tstate, &duration) != WZ_RESULT_OK ||
        segments == 0 || index == 0 || *index >= capacity) {
        return WZ_RESULT_PARSE_ERROR;
    }
    segments[*index].duration = duration;
    segments[*index].ear_level = level;
    *index += 1u;
    return WZ_RESULT_OK;
}

static wz_result_t wz_tape_tap_expand_block(const wz_byte_t* block,
                                            size_t block_length,
                                            wz_dword_t ticks_per_tstate,
                                            wz_tape_segment_t* segments,
                                            size_t capacity,
                                            size_t* index)
{
    wz_byte_t level = 1u;
    size_t pilot_count = block[0u] == 0u ? 8063u : 3223u;

    for (size_t pulse = 0u; pulse < pilot_count; ++pulse) {
        if (wz_tape_tap_append(segments, capacity, index, WZ_TAP_PILOT_TSTATES,
                               ticks_per_tstate, level) != WZ_RESULT_OK) {
            return WZ_RESULT_PARSE_ERROR;
        }
        level ^= 1u;
    }
    if (wz_tape_tap_append(segments, capacity, index, WZ_TAP_SYNC_FIRST_TSTATES,
                           ticks_per_tstate, level) != WZ_RESULT_OK) {
        return WZ_RESULT_PARSE_ERROR;
    }
    level ^= 1u;
    if (wz_tape_tap_append(segments, capacity, index, WZ_TAP_SYNC_SECOND_TSTATES,
                           ticks_per_tstate, level) != WZ_RESULT_OK) {
        return WZ_RESULT_PARSE_ERROR;
    }
    level ^= 1u;
    for (size_t byte_index = 0u; byte_index < block_length; ++byte_index) {
        for (unsigned bit = 0u; bit < 8u; ++bit) {
            wz_dword_t pulse = (block[byte_index] & (wz_byte_t)(0x80u >> bit)) != 0u ?
                WZ_TAP_ONE_TSTATES : WZ_TAP_ZERO_TSTATES;
            if (wz_tape_tap_append(segments, capacity, index, pulse,
                                   ticks_per_tstate, level) != WZ_RESULT_OK) {
                return WZ_RESULT_PARSE_ERROR;
            }
            level ^= 1u;
            if (wz_tape_tap_append(segments, capacity, index, pulse,
                                   ticks_per_tstate, level) != WZ_RESULT_OK) {
                return WZ_RESULT_PARSE_ERROR;
            }
            level ^= 1u;
        }
    }
    return wz_tape_tap_append(segments, capacity, index, WZ_TAP_PAUSE_TSTATES,
                              ticks_per_tstate, 0u);
}

wz_result_t wz_tape_parse_standard_tap(const wz_byte_t* data,
                                       size_t length,
                                       wz_dword_t master_ticks_per_tstate,
                                       wz_tape_segment_t* segments,
                                       size_t capacity,
                                       size_t* count)
{
    size_t required;
    size_t offset = 0u;
    size_t index = 0u;

    if (master_ticks_per_tstate == 0u || count == 0 ||
        wz_tape_tap_validate(data, length, &required) != WZ_RESULT_OK) {
        return WZ_RESULT_PARSE_ERROR;
    }
    *count = required;
    if (segments == 0 || capacity < required) {
        return WZ_RESULT_BUFFER_TOO_SMALL;
    }
    while (offset < length) {
        size_t block_length = (size_t)wz_read_le16(&data[offset]);
        offset += 2u;
        if (wz_tape_tap_expand_block(&data[offset], block_length,
                                     master_ticks_per_tstate, segments,
                                     capacity, &index) != WZ_RESULT_OK) {
            return WZ_RESULT_PARSE_ERROR;
        }
        offset += block_length;
    }
    return index == required ? WZ_RESULT_OK : WZ_RESULT_PARSE_ERROR;
}

wz_result_t wz_tape_write_standard_tap(const wz_tap_block_t* blocks,
                                       size_t block_count,
                                       wz_byte_t* output,
                                       size_t capacity,
                                       size_t* length)
{
    size_t required = 0u;
    size_t offset = 0u;

    if (blocks == 0 || block_count == 0u || length == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    for (size_t block_index = 0u; block_index < block_count; ++block_index) {
        const wz_tap_block_t* block = &blocks[block_index];

        if (block->data == 0 || block->length == 0u || block->length > 65534u ||
            block->length > SIZE_MAX - 3u ||
            wz_tape_tap_add_count(&required, block->length + 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_PARSE_ERROR;
        }
    }
    *length = required;
    if (output == 0 || capacity < required) {
        return WZ_RESULT_BUFFER_TOO_SMALL;
    }
    for (size_t block_index = 0u; block_index < block_count; ++block_index) {
        const wz_tap_block_t* block = &blocks[block_index];
        wz_byte_t checksum = 0u;
        wz_word_t encoded_length = (wz_word_t)(block->length + 1u);

        wz_write_le16(&output[offset], encoded_length);
        offset += 2u;
        for (size_t byte_index = 0u; byte_index < block->length; ++byte_index) {
            output[offset + byte_index] = block->data[byte_index];
            checksum ^= block->data[byte_index];
        }
        offset += block->length;
        output[offset++] = checksum;
    }
    return WZ_RESULT_OK;
}

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
