/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_HOST_PACING_H
#define WZ_APP_WZ_HOST_PACING_H

#include <stdbool.h>

#include "app/wz_speed_policy.h"
#include "core/wz_types.h"

typedef bool (*wz_pacing_sleep_fn)(wz_qword_t nanoseconds, void* context);

typedef struct {
    wz_master_tick_t anchor_machine_tick;
    wz_qword_t anchor_host_nanoseconds;
    wz_qword_t master_ticks_per_second;
    wz_speed_policy_t speed;
    wz_speed_policy_t pending_speed;
    bool speed_change_pending;
    bool anchored;
} wz_host_pacing_t;

bool wz_host_pacing_init(wz_host_pacing_t* pacing,
                         wz_qword_t master_ticks_per_second,
                         wz_speed_policy_t speed,
                         wz_qword_t host_nanoseconds,
                         wz_master_tick_t machine_tick);
bool wz_host_pacing_set_speed(wz_host_pacing_t* pacing,
                              wz_speed_policy_t speed);
bool wz_host_pacing_wait(wz_host_pacing_t* pacing,
                         wz_qword_t host_nanoseconds,
                         wz_master_tick_t machine_tick,
                         wz_pacing_sleep_fn sleep,
                         void* context,
                         wz_qword_t* requested_sleep_nanoseconds);

#endif
