/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_WZ_RUNNER_H
#define WZ_CORE_WZ_RUNNER_H

#include "core/wz_machine.h"
#include "core/wz_trace.h"

typedef struct {
    wz_machine_t* machine;
    wz_trace_sink_t* trace_sink;
} wz_headless_runner_t;

wz_result_t wz_headless_runner_init(wz_headless_runner_t* runner,
                                     wz_machine_t* machine,
                                     wz_trace_sink_t* trace_sink);
wz_result_t wz_headless_runner_advance(wz_headless_runner_t* runner,
                                       wz_master_tick_t ticks);

#endif
