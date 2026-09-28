/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "core/wz_runner.h"

#include "core/wz_z80.h"

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

wz_result_t wz_headless_runner_execute(wz_headless_runner_t* runner,
                                       wz_master_tick_t minimum_ticks)
{
    wz_master_tick_t target;
    if (runner == 0 || runner->machine == 0 ||
        minimum_ticks > UINT64_MAX - runner->machine->master_tick) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    target = runner->machine->master_tick + minimum_ticks;
    while (runner->machine->master_tick < target) {
        wz_master_tick_t before = runner->machine->master_tick;
        wz_master_tick_t elapsed;
        wz_machine_update_interrupt_line(runner->machine);
        wz_result_t step_result;
        if (wz_machine_maskable_interrupt_line_low(runner->machine) &&
            wz_z80_maskable_interrupts_acceptable(&runner->machine->cpu)) {
            step_result = wz_z80_accept_maskable_interrupt(runner->machine);
        } else {
            step_result = wz_z80_step(runner->machine);
        }
        if (step_result != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        elapsed = runner->machine->master_tick - before;
        if (elapsed == 0u) return WZ_RESULT_INVALID_STATE;
        for (wz_master_tick_t offset = 0u; offset < elapsed; ++offset) {
            if (wz_machine_advance_tape(runner->machine, 1u) != WZ_RESULT_OK ||
                wz_ay_advance_master_ticks(&runner->machine->ay, 1u) != WZ_RESULT_OK) {
                return WZ_RESULT_INVALID_STATE;
            }
            wz_machine_update_interrupt_line(runner->machine);
            wz_trace_emit(runner->trace_sink, WZ_TRACE_MASTER_TICK_ADVANCED,
                          before + offset + 1u);
        }
    }
    return WZ_RESULT_OK;
}
