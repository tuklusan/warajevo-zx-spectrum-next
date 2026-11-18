/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_input_replay.h"

#include <stdint.h>
#include <string.h>

void wz_input_trace_init(wz_input_trace_t* trace,
                         wz_normalized_input_transition_t* entries,
                         size_t capacity)
{
    if (trace == 0) return;
    trace->entries = entries;
    trace->capacity = capacity;
    trace->count = 0u;
    trace->next_sequence = 0u;
}

void wz_input_trace_state_init(wz_input_trace_state_t* state)
{
    if (state == 0) return;
    memset(state->key_down, 0, sizeof(state->key_down));
}

bool wz_input_trace_record(wz_input_trace_t* trace,
                           unsigned char key,
                           bool pressed,
                           wz_master_tick_t master_tick)
{
    wz_normalized_input_transition_t* entry;
    if (trace == 0 || trace->entries == 0 || trace->count >= trace->capacity ||
        trace->next_sequence == UINT64_MAX ||
        (trace->count != 0u &&
         master_tick < trace->entries[trace->count - 1u].master_tick)) {
        return false;
    }
    entry = &trace->entries[trace->count];
    entry->key = key;
    entry->pressed = pressed ? 1u : 0u;
    entry->master_tick = master_tick;
    entry->sequence = trace->next_sequence;
    trace->count += 1u;
    trace->next_sequence += 1u;
    return true;
}

bool wz_input_trace_record_effective(wz_input_trace_t* trace,
                                     wz_input_trace_state_t* state,
                                     unsigned char key,
                                     bool pressed,
                                     wz_master_tick_t master_tick)
{
    wz_normalized_input_transition_t* entry;
    const unsigned char normalized = pressed ? 1u : 0u;
    if (trace == 0 || state == 0 || key >= WZ_INPUT_TRACE_KEY_COUNT ||
        trace->entries == 0 || trace->count >= trace->capacity ||
        trace->next_sequence == UINT64_MAX ||
        (trace->count != 0u &&
         master_tick < trace->entries[trace->count - 1u].master_tick)) {
        return false;
    }
    if (state->key_down[key] == normalized) return false;
    entry = &trace->entries[trace->count];
    entry->key = key;
    entry->pressed = normalized;
    entry->master_tick = master_tick;
    entry->sequence = trace->next_sequence;
    ++trace->count;
    ++trace->next_sequence;
    state->key_down[key] = normalized;
    return true;
}

bool wz_input_trace_replay(const wz_input_trace_t* trace,
                           wz_input_trace_sink_t sink,
                           void* context)
{
    wz_master_tick_t previous_tick = 0u;
    size_t index;
    if (trace == 0 || sink == 0 || (trace->count != 0u && trace->entries == 0)) {
        return false;
    }
    for (index = 0u; index < trace->count; ++index) {
        const wz_normalized_input_transition_t* entry = &trace->entries[index];
        if (entry->sequence != (wz_qword_t)index ||
            entry->key >= WZ_INPUT_TRACE_KEY_COUNT ||
            (index != 0u && entry->master_tick < previous_tick) ||
            entry->pressed > 1u || !sink(entry, context)) {
            return false;
        }
        previous_tick = entry->master_tick;
    }
    return true;
}
