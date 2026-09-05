/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_runner.h"

wz_result_t wz_headless_runner_init(wz_headless_runner_t* runner,
                                     wz_machine_t* machine,
                                     wz_trace_sink_t* trace_sink)
{
    if (runner == 0 || machine == 0 || machine->profile == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }

    runner->machine = machine;
    runner->trace_sink = trace_sink;
    return WZ_RESULT_OK;
}

wz_result_t wz_headless_runner_advance(wz_headless_runner_t* runner,
                                       wz_master_tick_t ticks)
{
    if (runner == 0 || runner->machine == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (ticks > UINT64_MAX - runner->machine->master_tick) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }

    wz_machine_update_interrupt_line(runner->machine);
    for (wz_master_tick_t offset = 0u; offset < ticks; ++offset) {
        if (wz_machine_advance_tape(runner->machine, 1u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        ++runner->machine->master_tick;
        if (wz_ay_advance_master_ticks(&runner->machine->ay, 1u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        wz_machine_update_interrupt_line(runner->machine);
        wz_trace_emit(runner->trace_sink,
                      WZ_TRACE_MASTER_TICK_ADVANCED,
                      runner->machine->master_tick);
    }
    return WZ_RESULT_OK;
}
