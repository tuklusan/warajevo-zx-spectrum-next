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
    wz_bus_cycle_t cycle;
    wz_master_tick_t tick;
    wz_word_t address;
    wz_byte_t value;
    wz_byte_t states;
} expected_bus_t;

typedef struct {
    wz_bus_request_t bus[5];
    wz_trace_event_t trace[32];
    size_t bus_count;
    size_t trace_count;
    bool overflow;
} capture_t;

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "failed at line %d: %s\n", \
                      __LINE__, #condition); \
        goto cleanup; \
    } \
} while (0)

static void record_bus(const wz_bus_request_t* request, void* context)
{
    capture_t* capture = (capture_t*)context;
    if (capture->bus_count >= 5u) {
        capture->overflow = true;
        return;
    }
    capture->bus[capture->bus_count++] = *request;
}

static void record_trace(const wz_trace_event_t* event, void* context)
{
    capture_t* capture = (capture_t*)context;
    if (capture->trace_count >= 32u) {
        capture->overflow = true;
        return;
    }
    capture->trace[capture->trace_count++] = *event;
}

static bool check_capture(const capture_t* capture)
{
    static const expected_bus_t expected[] = {
        {WZ_BUS_M1_OPCODE_FETCH, 0u, 0x8000u, 0x00u, 4u},
        {WZ_BUS_M1_OPCODE_FETCH, 8u, 0x8001u, 0x32u, 4u},
        {WZ_BUS_MEMORY_READ, 16u, 0x8002u, 0x01u, 3u},
        {WZ_BUS_MEMORY_READ, 22u, 0x8003u, 0x90u, 3u},
        {WZ_BUS_MEMORY_WRITE, 28u, 0x9001u, 0x5au, 3u}
    };
    size_t traced_bus = 0u;
    size_t traced_instructions = 0u;
    size_t traced_opcodes = 0u;
    if (capture->overflow || capture->bus_count != 5u) return false;
    for (size_t index = 0u; index < 5u; ++index) {
        const wz_bus_request_t* bus = &capture->bus[index];
        if (bus->cycle != expected[index].cycle ||
            bus->master_tick != expected[index].tick ||
            bus->address != expected[index].address ||
            bus->value != expected[index].value ||
            bus->t_states != expected[index].states ||
            bus->contention_delay != 0u) return false;
    }
    for (size_t index = 0u; index < capture->trace_count; ++index) {
        const wz_trace_event_t* event = &capture->trace[index];
        if (index != 0u &&
            event->sequence <= capture->trace[index - 1u].sequence) return false;
        if (event->kind == WZ_TRACE_CPU_BUS) {
            if (traced_bus >= 5u ||
                event->master_tick != expected[traced_bus].tick ||
                event->address != expected[traced_bus].address ||
                event->value != expected[traced_bus].value ||
                event->cycle != (wz_byte_t)expected[traced_bus].cycle ||
                event->t_states != expected[traced_bus].states) return false;
            ++traced_bus;
        } else if (event->kind == WZ_TRACE_CPU_INSTRUCTION) {
            ++traced_instructions;
        } else if (event->kind == WZ_TRACE_CPU_OPCODE_BYTE) {
            ++traced_opcodes;
        }
    }
    return traced_bus == 5u && traced_instructions == 2u &&
           traced_opcodes == 2u;
}

int main(void)
{
    static const wz_byte_t program[] = {0x00u, 0x32u, 0x01u, 0x90u};
    wz_machine_t machine;
    wz_bus_observer_t observer;
    wz_trace_sink_t sink;
    capture_t capture = {0};
    bool initialized = false;
    bool passed = false;
    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    initialized = true;
    for (size_t index = 0u; index < sizeof(program); ++index) {
        wz_machine_memory_write(&machine, (wz_word_t)(0x8000u + index),
                                program[index]);
    }
    machine.cpu.program_counter = 0x8000u;
    machine.cpu.main.a = 0x5au;
    wz_bus_observer_init(&observer, record_bus, &capture);
    REQUIRE(wz_machine_set_bus_observer(&machine, &observer) == WZ_RESULT_OK);
    wz_trace_sink_init(&sink, record_trace, &capture);
    wz_machine_set_timing_trace(&machine, &sink);
    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_OK);
    REQUIRE(machine.cpu.program_counter == 0x8001u &&
            machine.cpu.r == 1u && machine.master_tick == 8u);
    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_OK);
    REQUIRE(machine.cpu.program_counter == 0x8004u &&
            machine.cpu.r == 2u && machine.master_tick == 34u);
    REQUIRE(machine.cpu.memptr == 0x5a02u);
    REQUIRE(wz_machine_memory_read(&machine, 0x9001u) == 0x5au);
    REQUIRE(check_capture(&capture));
    passed = true;
cleanup:
    if (initialized) wz_machine_destroy(&machine);
    if (!passed) return 1;
    (void)puts("{\"testId\":\"cpu-fetch-write\",\"status\":\"pass\","
               "\"cases\":2,\"exactBusCycles\":5}");
    return 0;
}
