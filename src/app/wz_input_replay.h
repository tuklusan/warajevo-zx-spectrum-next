/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_INPUT_REPLAY_H
#define WZ_APP_WZ_INPUT_REPLAY_H

#include <stdbool.h>
#include <stddef.h>

#include "core/wz_types.h"

typedef struct {
    unsigned char key;
    unsigned char pressed;
    wz_master_tick_t master_tick;
    wz_qword_t sequence;
} wz_normalized_input_transition_t;

typedef struct {
    wz_normalized_input_transition_t* entries;
    size_t capacity;
    size_t count;
    wz_qword_t next_sequence;
} wz_input_trace_t;

typedef bool (*wz_input_trace_sink_t)(
    const wz_normalized_input_transition_t* transition,
    void* context);

void wz_input_trace_init(wz_input_trace_t* trace,
                         wz_normalized_input_transition_t* entries,
                         size_t capacity);

bool wz_input_trace_record(wz_input_trace_t* trace,
                           unsigned char key,
                           bool pressed,
                           wz_master_tick_t master_tick);

bool wz_input_trace_replay(const wz_input_trace_t* trace,
                           wz_input_trace_sink_t sink,
                           void* context);

#endif
