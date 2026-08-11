/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_trace.h"

void wz_trace_sink_init(wz_trace_sink_t* sink,
                        wz_trace_emit_fn emit,
                        void* context)
{
    if (sink != 0) {
        sink->emit = emit;
        sink->context = context;
        sink->next_sequence = 0u;
    }
}

void wz_trace_emit(wz_trace_sink_t* sink,
                   wz_trace_event_kind_t kind,
                   wz_master_tick_t master_tick)
{
    wz_trace_event_t event;

    if (sink == 0 || sink->emit == 0) {
        return;
    }

    event.kind = kind;
    event.master_tick = master_tick;
    event.sequence = sink->next_sequence++;
    sink->emit(&event, sink->context);
}
