/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_INPUT_ARBITER_H
#define WZ_APP_WZ_INPUT_ARBITER_H

#include <stdbool.h>
#include <stddef.h>

#include "app/wz_input_replay.h"

#define WZ_INPUT_ARBITER_SOURCE_COUNT 4u
#define WZ_INPUT_ARBITER_KEY_COUNT WZ_INPUT_TRACE_KEY_COUNT
#define WZ_INPUT_ARBITER_TRACE_CAPACITY 1024u

typedef enum {
    WZ_INPUT_SOURCE_LOCAL = 0,
    WZ_INPUT_SOURCE_TELNET,
    WZ_INPUT_SOURCE_SCRIPT,
    WZ_INPUT_SOURCE_RESERVED
} wz_input_source_t;

typedef struct {
    unsigned char owned[WZ_INPUT_ARBITER_SOURCE_COUNT][WZ_INPUT_ARBITER_KEY_COUNT];
    wz_input_trace_t input_trace;
    wz_input_trace_state_t input_trace_state;
    wz_normalized_input_transition_t input_trace_entries[
        WZ_INPUT_ARBITER_TRACE_CAPACITY];
} wz_input_arbiter_t;

void wz_input_arbiter_init(wz_input_arbiter_t* arbiter);
bool wz_input_arbiter_set(wz_input_arbiter_t* arbiter,
                          wz_input_source_t source, size_t key, bool pressed);
bool wz_input_arbiter_source_key_down(const wz_input_arbiter_t* arbiter,
                                      wz_input_source_t source, size_t key);
bool wz_input_arbiter_key_down(const wz_input_arbiter_t* arbiter, size_t key);
bool wz_input_arbiter_release_source(wz_input_arbiter_t* arbiter,
                                     wz_input_source_t source);
bool wz_input_arbiter_record_effective(wz_input_arbiter_t* arbiter,
                                       size_t key,
                                       bool pressed,
                                       wz_master_tick_t master_tick);
const wz_input_trace_t* wz_input_arbiter_trace(
    const wz_input_arbiter_t* arbiter);

#endif
