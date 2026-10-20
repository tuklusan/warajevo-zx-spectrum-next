/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdbool.h>
#include <stdio.h>

#include "core/wz_bus.h"
#include "core/wz_machine.h"
#include "core/wz_machine_profile.h"
#include "diagnostics/wz_trace_file.h"

typedef struct {
    wz_trace_file_t* file;
    wz_master_tick_t last_bus_tick;
    wz_master_tick_t last_ula_tick;
    wz_qword_t last_bus_sequence;
    unsigned same_tick_pairs;
    unsigned target_events;
    bool have_bus;
    bool have_ula;
    bool ordering_failure;
    bool values_valid;
} trace_check_t;

static void record_trace(const wz_trace_event_t* event, void* context)
{
    trace_check_t* check = (trace_check_t*)context;
    if (event->kind == WZ_TRACE_CPU_BUS) {
        if (check->have_ula && check->last_ula_tick == event->master_tick) {
            check->ordering_failure = true;
        }
        check->last_bus_tick = event->master_tick;
        check->last_bus_sequence = event->sequence;
        check->have_bus = true;
    } else if (event->kind == WZ_TRACE_ULA_FETCH) {
        wz_dword_t row = (wz_dword_t)(event->register_snapshot >> 8u);
        wz_dword_t cell = (wz_dword_t)event->register_snapshot;
        if (check->have_bus && check->last_bus_tick == event->master_tick) {
            if (check->last_bus_sequence >= event->sequence) {
                check->ordering_failure = true;
            } else {
                ++check->same_tick_pairs;
            }
        }
        if (row == 0u && cell < 2u) {
            bool attribute = event->auxiliary == WZ_TRACE_ULA_ATTRIBUTE_FETCH;
            wz_word_t expected_address = (wz_word_t)(
                (attribute ? 0x5800u : 0x4000u) + cell);
            wz_byte_t expected_value = cell == 0u ?
                (attribute ? 0x47u : 0xa5u) :
                (attribute ? 0x15u : 0x5bu);
            if (event->address != expected_address ||
                event->value != expected_value) {
                (void)fprintf(stderr,
                    "fetch tick=%llu row=%u cell=%u kind=%u address=%04x value=%02x expected=%04x/%02x\n",
                    (unsigned long long)event->master_tick, (unsigned)row,
                    (unsigned)cell, (unsigned)event->auxiliary,
                    (unsigned)event->address, (unsigned)event->value,
                    (unsigned)expected_address, (unsigned)expected_value);
                check->values_valid = false;
            }
            ++check->target_events;
        }
        check->last_ula_tick = event->master_tick;
        check->have_ula = true;
        wz_trace_file_emit(event, check->file);
    }
}

static wz_result_t request_at(wz_machine_t* machine, wz_master_tick_t tick,
                              bool page_write, wz_byte_t page_value)
{
    wz_bus_request_t request;
    wz_bus_request_init(&request,
        page_write ? WZ_BUS_IO_WRITE : WZ_BUS_INTERNAL,
        tick, page_write ? 0x7ffdu : 0u, page_value, 1u);
    return wz_machine_bus_request(machine, &request);
}

int main(int argc, char** argv)
{
    wz_machine_t machine = {0};
    wz_trace_file_t trace = {0};
    wz_trace_sink_t sink = {0};
    trace_check_t check = {0};
    wz_master_tick_t frame_ticks;
    wz_master_tick_t bitmap_tick;
    wz_master_tick_t attribute_tick;
    wz_master_tick_t next_attribute_tick;
    bool machine_initialized = false;
    int result = 1;

    if (argc != 2) return 2;
    if (wz_machine_init(&machine, wz_machine_profile_128k_pal()) !=
        WZ_RESULT_OK) goto cleanup;
    machine_initialized = true;
    if (machine.ram_128k == 0 || machine.ula_frame_captures == 0) goto cleanup;
    machine.hardware_io_decode_enabled = 1u;
    machine.ram_128k[5u * WZ_128K_RAM_BANK_SIZE] = 0xa5u;
    machine.ram_128k[7u * WZ_128K_RAM_BANK_SIZE] = 0xb7u;
    machine.ram_128k[5u * WZ_128K_RAM_BANK_SIZE + 1u] = 0x26u;
    machine.ram_128k[7u * WZ_128K_RAM_BANK_SIZE + 1u] = 0x5bu;
    machine.ram_128k[5u * WZ_128K_RAM_BANK_SIZE + 0x1800u] = 0x11u;
    machine.ram_128k[7u * WZ_128K_RAM_BANK_SIZE + 0x1800u] = 0x47u;
    machine.ram_128k[5u * WZ_128K_RAM_BANK_SIZE + 0x1801u] = 0x15u;
    machine.ram_128k[7u * WZ_128K_RAM_BANK_SIZE + 0x1801u] = 0x73u;

    frame_ticks = (wz_master_tick_t)machine.profile->tstates_per_frame *
        machine.profile->master_ticks_per_cpu_tstate;
    bitmap_tick = (wz_master_tick_t)machine.profile->ula_fetch_start_tstate *
        machine.profile->master_ticks_per_cpu_tstate;
    attribute_tick = bitmap_tick +
        (wz_master_tick_t)machine.profile->ula_attribute_offset_tstates *
            machine.profile->master_ticks_per_cpu_tstate;
    next_attribute_tick = attribute_tick +
        (wz_master_tick_t)machine.profile->ula_fetch_interval_tstates *
            machine.profile->master_ticks_per_cpu_tstate;
    if (wz_trace_file_create(&trace, argv[1], 3u,
            (wz_dword_t)WZ_MACHINE_128K_PAL, 0u,
            UINT32_C(1) << WZ_TRACE_ULA_FETCH) != WZ_RESULT_OK) {
        goto cleanup;
    }
    check.file = &trace;
    check.values_valid = true;
    wz_trace_sink_init(&sink, record_trace, &check);
    wz_machine_set_timing_trace(&machine, &sink);

    for (wz_master_tick_t tick = 0u; tick < frame_ticks; tick += 2u) {
        bool switch_to_7 = tick == attribute_tick;
        bool switch_to_5 = tick == next_attribute_tick;
        if (request_at(&machine, tick, switch_to_7 || switch_to_5,
                       switch_to_7 ? 0x08u : 0u) != WZ_RESULT_OK) {
            (void)fprintf(stderr, "bus request failed at tick %llu\n",
                          (unsigned long long)tick);
            goto cleanup;
        }
    }
    if (trace.failed || trace.record_count != 12288u ||
        check.target_events != 4u || !check.values_valid ||
        check.same_tick_pairs == 0u || check.ordering_failure) {
        (void)fprintf(stderr,
            "trace=%llu targets=%u values=%u same_tick=%u ordering=%u\n",
            (unsigned long long)trace.record_count, check.target_events,
            check.values_valid ? 1u : 0u, check.same_tick_pairs,
            check.ordering_failure ? 1u : 0u);
        goto cleanup;
    }
    wz_machine_set_timing_trace(&machine, NULL);
    if (wz_trace_file_freeze(&trace) != WZ_RESULT_OK) goto cleanup;
    result = 0;

cleanup:
    wz_machine_set_timing_trace(&machine, NULL);
    if (trace.file != NULL) wz_trace_file_close(&trace);
    if (machine_initialized) wz_machine_destroy(&machine);
    return result;
}
