/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#ifndef WZ_CORE_WZ_MACHINE_INTERNAL_H
#define WZ_CORE_WZ_MACHINE_INTERNAL_H

#include "core/wz_machine.h"

void wz_machine_advance_tape_ticks_slow(wz_machine_t* machine,
                                        wz_master_tick_t ticks);

static inline void wz_machine_advance_master_ticks(
    wz_machine_t* machine, wz_master_tick_t ticks)
{
    if (machine == 0) {
        return;
    }
    if (!machine->tape_mounted || !machine->tape_state.motor_on ||
        machine->tape_state.at_end ||
        machine->networking_mode == WZ_NETWORKING_EAR_MIC) {
        machine->master_tick += ticks;
    } else {
        wz_machine_advance_tape_ticks_slow(machine, ticks);
    }
    if (machine->timing_trace != 0) {
        wz_zxnet_set_master_tick(&machine->zxnet, machine->master_tick);
    }
}

wz_result_t wz_machine_memory_write_at_tick_deferred(
    wz_machine_t* machine, wz_word_t address, wz_byte_t value,
    wz_master_tick_t master_tick);
wz_result_t wz_machine_trace_ula_until(wz_machine_t* machine,
                                      wz_master_tick_t master_tick,
                                      bool inclusive);
void wz_z80_trace_bus_context(wz_machine_t* machine,
                              wz_master_tick_t master_tick);

#endif
