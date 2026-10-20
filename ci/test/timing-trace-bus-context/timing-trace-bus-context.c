/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and
 * use for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "core/wz_machine.h"
#include "core/wz_machine_profile.h"

typedef struct {
    wz_trace_event_t bus_event;
    unsigned bus_event_count;
} capture_t;

static void capture_trace(const wz_trace_event_t* event, void* context)
{
    capture_t* capture = (capture_t*)context;
    if (event->kind == WZ_TRACE_CPU_BUS) {
        capture->bus_event = *event;
        ++capture->bus_event_count;
    }
}

int main(void)
{
    wz_machine_t machine = {0};
    wz_trace_sink_t trace = {0};
    wz_bus_request_t request = {0};
    capture_t capture = {0};
    const wz_qword_t expected_registers =
        (UINT64_C(0x88) << 56u) | (UINT64_C(0x77) << 48u) |
        (UINT64_C(0x66) << 40u) | (UINT64_C(0x55) << 32u) |
        (UINT64_C(0x44) << 24u) | (UINT64_C(0x33) << 16u) |
        (UINT64_C(0x22) << 8u) | UINT64_C(0x11);
    bool initialized = false;
    int result = 1;

    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) !=
        WZ_RESULT_OK) {
        goto cleanup;
    }
    initialized = true;
    machine.cpu.program_counter = 0x2345u;
    machine.cpu.stack_pointer = 0xcafeu;
    machine.cpu.main.a = 0x11u;
    machine.cpu.main.f = 0x22u;
    machine.cpu.main.b = 0x33u;
    machine.cpu.main.c = 0x44u;
    machine.cpu.main.d = 0x55u;
    machine.cpu.main.e = 0x66u;
    machine.cpu.main.h = 0x77u;
    machine.cpu.main.l = 0x88u;

    wz_trace_sink_init(&trace, capture_trace, &capture);
    wz_machine_set_timing_trace(&machine, &trace);
    wz_bus_request_init(&request, WZ_BUS_MEMORY_READ, 0u, 0x4000u, 0u, 3u);
    machine.memory[0x4000u] = 0xa5u;
    if (wz_machine_bus_request(&machine, &request) != WZ_RESULT_OK ||
        capture.bus_event_count != 1u ||
        capture.bus_event.program_counter != 0x2345u ||
        capture.bus_event.stack_pointer != 0xcafeu ||
        capture.bus_event.register_snapshot != expected_registers ||
        capture.bus_event.address != 0x4000u ||
        capture.bus_event.value != 0xa5u) {
        goto cleanup;
    }

    (void)puts("PASS trace bus records CPU context");
    result = 0;

cleanup:
    if (initialized) {
        wz_machine_set_timing_trace(&machine, NULL);
        wz_machine_destroy(&machine);
    }
    return result;
}
