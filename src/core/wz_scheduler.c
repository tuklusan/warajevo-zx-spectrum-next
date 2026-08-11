/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_scheduler.h"

static bool wz_event_precedes(const wz_scheduled_event_t* left,
                              const wz_scheduled_event_t* right)
{
    if (left->tick != right->tick) {
        return left->tick < right->tick;
    }
    if (left->priority != right->priority) {
        return left->priority < right->priority;
    }
    return left->sequence < right->sequence;
}

void wz_scheduler_init(wz_scheduler_t* scheduler)
{
    if (scheduler != 0) {
        scheduler->count = 0u;
        scheduler->next_sequence = 0u;
    }
}

wz_result_t wz_scheduler_schedule(wz_scheduler_t* scheduler,
                                   wz_master_tick_t tick,
                                   wz_event_priority_t priority,
                                   wz_event_callback_t callback,
                                   void* context)
{
    size_t index;

    if (scheduler == 0 || callback == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (scheduler->count >= WZ_SCHEDULER_CAPACITY) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }

    index = scheduler->count++;
    scheduler->events[index].tick = tick;
    scheduler->events[index].priority = priority;
    scheduler->events[index].sequence = scheduler->next_sequence++;
    scheduler->events[index].callback = callback;
    scheduler->events[index].context = context;
    return WZ_RESULT_OK;
}

wz_result_t wz_scheduler_dispatch_next(wz_scheduler_t* scheduler)
{
    size_t selected;
    size_t index;
    wz_scheduled_event_t event;

    if (scheduler == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (scheduler->count == 0u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }

    selected = 0u;
    for (index = 1u; index < scheduler->count; ++index) {
        if (wz_event_precedes(&scheduler->events[index],
                              &scheduler->events[selected])) {
            selected = index;
        }
    }

    event = scheduler->events[selected];
    scheduler->events[selected] = scheduler->events[scheduler->count - 1u];
    --scheduler->count;
    event.callback(event.context);
    return WZ_RESULT_OK;
}
