/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_INPUT_TIMING_H
#define WZ_APP_WZ_INPUT_TIMING_H

#include <stdbool.h>

#include "core/wz_input_types.h"

typedef struct {
    wz_master_tick_t last_tick;
    wz_qword_t next_sequence;
    bool assigned;
} wz_input_timestamp_assigner_t;

typedef struct {
    wz_input_event_t event;
    wz_master_tick_t master_tick;
    wz_qword_t sequence;
} wz_timed_input_event_t;

void wz_input_timestamp_assigner_init(wz_input_timestamp_assigner_t* assigner);
bool wz_input_timestamp_assign(wz_input_timestamp_assigner_t* assigner,
                               const wz_input_event_t* event,
                               wz_master_tick_t requested_tick,
                               wz_timed_input_event_t* timed_event);

#endif
