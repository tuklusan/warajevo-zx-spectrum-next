/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdbool.h>
#include <stdio.h>

#include "core/wz_machine.h"

typedef struct {
    wz_qword_t previous_sequence;
    wz_master_tick_t previous_tick;
    wz_master_tick_t sync_tick;
    wz_qword_t sync_chunks[5];
    unsigned events;
    unsigned bus_fetches;
    unsigned opcode_bytes;
    unsigned instructions;
    unsigned sync_groups;
    unsigned sync_chunk;
    bool valid;
    bool saw_halted_sync;
} capture_t;

static void record_trace(const wz_trace_event_t* event, void* context)
{
    capture_t* capture = (capture_t*)context;
    if (capture->events != 0u &&
        (event->sequence <= capture->previous_sequence ||
         event->master_tick < capture->previous_tick)) capture->valid = false;
    capture->previous_sequence = event->sequence;
    capture->previous_tick = event->master_tick;
    ++capture->events;
    switch (event->kind) {
    case WZ_TRACE_CPU_BUS:
        if (event->cycle != WZ_BUS_M1_OPCODE_FETCH ||
            event->address != 0x8000u || event->value != 0x76u ||
            event->t_states != 4u) capture->valid = false;
        ++capture->bus_fetches;
        break;
    case WZ_TRACE_CPU_OPCODE_BYTE:
        if (event->address != 0x8000u || event->value != 0x76u) {
            capture->valid = false;
        }
        ++capture->opcode_bytes;
        break;
    case WZ_TRACE_CPU_INSTRUCTION:
        if (event->program_counter != 0x8000u || event->value != 0x76u) {
            capture->valid = false;
        }
        ++capture->instructions;
        break;
    case WZ_TRACE_CPU_STATE_SYNC:
        if (event->cycle != capture->sync_chunk ||
            (capture->sync_chunk != 0u && event->master_tick != capture->sync_tick)) {
            capture->valid = false;
        }
        if (capture->sync_chunk == 0u) capture->sync_tick = event->master_tick;
        capture->sync_chunks[capture->sync_chunk] = event->register_snapshot;
        ++capture->sync_chunk;
        if (capture->sync_chunk == 5u) {
            ++capture->sync_groups;
            capture->sync_chunk = 0u;
            if (capture->sync_groups > 1u &&
                (capture->sync_chunks[4] & 0xffu) == 1u) {
                capture->saw_halted_sync = true;
            }
        }
        break;
    default:
        break;
    }
}

int main(void)
{
    wz_machine_t machine;
    wz_trace_sink_t sink;
    capture_t capture = {0};
    capture.valid = true;
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        (void)fputs("machine init failed\n", stderr);
        return 1;
    }
    wz_machine_memory_write(&machine, 0x8000u, 0x76u);
    machine.cpu.program_counter = 0x8000u;
    wz_trace_sink_init(&sink, record_trace, &capture);
    wz_machine_set_timing_trace(&machine, &sink);
    for (unsigned fetch = 0u; fetch < 5000u; ++fetch) {
        if (wz_z80_step(&machine) != WZ_RESULT_OK) {
            (void)fprintf(stderr, "HALT fetch failed at %u\n", fetch);
            wz_machine_destroy(&machine);
            return 1;
        }
    }
    if (!capture.valid || capture.sync_chunk != 0u ||
        capture.sync_groups < 2u || !capture.saw_halted_sync ||
        capture.bus_fetches != 5000u || capture.opcode_bytes != 5000u ||
        capture.instructions != 1u || machine.cpu.halted != 1u ||
        machine.cpu.program_counter != 0x8000u ||
        machine.master_tick != 40000u) {
        (void)fprintf(stderr,
            "trace sync failure: groups=%u halted=%u fetches=%u "
            "bytes=%u instructions=%u valid=%u tick=%llu\n",
            capture.sync_groups, (unsigned)capture.saw_halted_sync,
            capture.bus_fetches, capture.opcode_bytes, capture.instructions,
            (unsigned)capture.valid, (unsigned long long)machine.master_tick);
        wz_machine_destroy(&machine);
        return 1;
    }
    wz_machine_destroy(&machine);
    (void)printf("{\"testId\":\"cpu-trace-sync\",\"status\":\"pass\","
                 "\"haltFetches\":5000,\"stateSyncGroups\":%u}\n",
                 capture.sync_groups);
    return 0;
}
