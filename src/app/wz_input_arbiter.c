/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_input_arbiter.h"

#include <stdint.h>

static void wz_input_arbiter_trace_state_clear(
    wz_input_trace_state_t* state)
{
    if (state == 0) {
        return;
    }
    for (size_t key = 0u; key < WZ_INPUT_TRACE_KEY_COUNT; ++key) {
        state->key_down[key] = 0u;
    }
}

static bool wz_input_arbiter_trace_record(
    wz_input_trace_t* trace,
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
    if (state->key_down[key] == normalized) {
        return false;
    }
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

static bool wz_input_source_valid(wz_input_source_t source)
{
    return (size_t)source < WZ_INPUT_ARBITER_SOURCE_COUNT;
}

void wz_input_arbiter_init(wz_input_arbiter_t* arbiter)
{
    if (arbiter == 0) {
        return;
    }
    for (size_t source = 0u; source < WZ_INPUT_ARBITER_SOURCE_COUNT; ++source) {
        for (size_t key = 0u; key < WZ_INPUT_ARBITER_KEY_COUNT; ++key) {
            arbiter->owned[source][key] = 0u;
        }
    }
    arbiter->input_trace.entries = arbiter->input_trace_entries;
    arbiter->input_trace.capacity =
        sizeof(arbiter->input_trace_entries) /
        sizeof(arbiter->input_trace_entries[0]);
    arbiter->input_trace.count = 0u;
    arbiter->input_trace.next_sequence = 0u;
    wz_input_arbiter_trace_state_clear(&arbiter->input_trace_state);
}

bool wz_input_arbiter_set(wz_input_arbiter_t* arbiter,
                          wz_input_source_t source, size_t key, bool pressed)
{
    if (arbiter == 0 || !wz_input_source_valid(source) ||
        key >= WZ_INPUT_ARBITER_KEY_COUNT) {
        return false;
    }
    arbiter->owned[source][key] = pressed ? 1u : 0u;
    return true;
}

bool wz_input_arbiter_source_key_down(const wz_input_arbiter_t* arbiter,
                                      wz_input_source_t source, size_t key)
{
    if (arbiter == 0 || !wz_input_source_valid(source) ||
        key >= WZ_INPUT_ARBITER_KEY_COUNT) {
        return false;
    }
    return arbiter->owned[source][key] != 0u;
}

bool wz_input_arbiter_key_down(const wz_input_arbiter_t* arbiter, size_t key)
{
    if (arbiter == 0 || key >= WZ_INPUT_ARBITER_KEY_COUNT) {
        return false;
    }
    for (size_t source = 0u; source < WZ_INPUT_ARBITER_SOURCE_COUNT; ++source) {
        if (arbiter->owned[source][key] != 0u) {
            return true;
        }
    }
    return false;
}

bool wz_input_arbiter_release_source(wz_input_arbiter_t* arbiter,
                                     wz_input_source_t source)
{
    if (arbiter == 0 || !wz_input_source_valid(source)) {
        return false;
    }
    for (size_t key = 0u; key < WZ_INPUT_ARBITER_KEY_COUNT; ++key) {
        arbiter->owned[source][key] = 0u;
    }
    return true;
}

bool wz_input_arbiter_record_effective(wz_input_arbiter_t* arbiter,
                                       size_t key,
                                       bool pressed,
                                       wz_master_tick_t master_tick)
{
    if (arbiter == 0 || key >= WZ_INPUT_TRACE_KEY_COUNT) {
        return false;
    }
    if (arbiter->input_trace.entries == 0) {
        return false;
    }
    if (arbiter->input_trace.count >= arbiter->input_trace.capacity) {
        return false;
    }
    if (arbiter->input_trace.count != 0u &&
        master_tick < arbiter->input_trace.entries[
            arbiter->input_trace.count - 1u].master_tick) {
        arbiter->input_trace.count = 0u;
        arbiter->input_trace.next_sequence = 0u;
        wz_input_arbiter_trace_state_clear(&arbiter->input_trace_state);
    }
    return wz_input_arbiter_trace_record(
        &arbiter->input_trace, &arbiter->input_trace_state,
        (unsigned char)key, pressed, master_tick);
}

const wz_input_trace_t* wz_input_arbiter_trace(
    const wz_input_arbiter_t* arbiter)
{
    return arbiter == 0 ? 0 : &arbiter->input_trace;
}
