/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_WZ_SCHEDULER_H
#define WZ_CORE_WZ_SCHEDULER_H

#include <stddef.h>

#include "core/wz_types.h"

typedef enum {
    WZ_EVENT_CPU = 0,
    WZ_EVENT_ULA,
    WZ_EVENT_BUS,
    WZ_EVENT_EXTERNAL
} wz_event_priority_t;

typedef void (*wz_event_callback_t)(void* context);

typedef struct {
    wz_master_tick_t tick;
    wz_event_priority_t priority;
    wz_qword_t sequence;
    wz_event_callback_t callback;
    void* context;
} wz_scheduled_event_t;

#define WZ_SCHEDULER_CAPACITY 64u

typedef struct {
    wz_scheduled_event_t events[WZ_SCHEDULER_CAPACITY];
    size_t count;
    wz_qword_t next_sequence;
} wz_scheduler_t;

void wz_scheduler_init(wz_scheduler_t* scheduler);
wz_result_t wz_scheduler_schedule(wz_scheduler_t* scheduler,
                                   wz_master_tick_t tick,
                                   wz_event_priority_t priority,
                                   wz_event_callback_t callback,
                                   void* context);
wz_result_t wz_scheduler_dispatch_next(wz_scheduler_t* scheduler);

#endif
