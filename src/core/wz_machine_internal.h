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

wz_result_t wz_machine_memory_write_at_tick_deferred(
    wz_machine_t* machine, wz_word_t address, wz_byte_t value,
    wz_master_tick_t master_tick);
wz_result_t wz_machine_trace_ula_until(wz_machine_t* machine,
                                      wz_master_tick_t master_tick,
                                      bool inclusive);
void wz_z80_trace_bus_context(wz_machine_t* machine);

#endif
