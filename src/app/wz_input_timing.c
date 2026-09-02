/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_input_timing.h"

void wz_input_timestamp_assigner_init(wz_input_timestamp_assigner_t* assigner)
{
    if (assigner != 0) {
        assigner->last_tick = 0u;
        assigner->next_sequence = 0u;
        assigner->assigned = false;
    }
}

bool wz_input_timestamp_assign(wz_input_timestamp_assigner_t* assigner,
                               const wz_input_event_t* event,
                               wz_master_tick_t requested_tick,
                               wz_timed_input_event_t* timed_event)
{
    wz_master_tick_t tick;

    if (assigner == 0 || event == 0 || timed_event == 0 ||
        assigner->next_sequence == UINT64_MAX) {
        return false;
    }
    tick = requested_tick;
    if (assigner->assigned && tick < assigner->last_tick) {
        tick = assigner->last_tick;
    }
    timed_event->event = *event;
    timed_event->master_tick = tick;
    timed_event->sequence = assigner->next_sequence;
    assigner->last_tick = tick;
    assigner->next_sequence += 1u;
    assigner->assigned = true;
    return true;
}
