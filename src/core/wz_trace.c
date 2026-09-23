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
        sink->cpu_state_valid = false;
    }
}

void wz_trace_emit(wz_trace_sink_t* sink,
                   wz_trace_event_kind_t kind,
                   wz_master_tick_t master_tick)
{
    wz_trace_event_t event;

    event.kind = kind;
    event.master_tick = master_tick;
    event.address = 0u;
    event.program_counter = 0u;
    event.stack_pointer = 0u;
    event.register_snapshot = 0u;
    event.value = 0u;
    event.auxiliary = 0u;
    event.cycle = 0u;
    event.t_states = 0u;
    wz_trace_emit_detail(sink, &event);
}

void wz_trace_emit_detail(wz_trace_sink_t* sink,
                          const wz_trace_event_t* event)
{
    wz_trace_event_t recorded;

    if (sink == 0 || sink->emit == 0 || event == 0) {
        return;
    }
    recorded = *event;
    recorded.sequence = sink->next_sequence++;
    sink->emit(&recorded, sink->context);
}
