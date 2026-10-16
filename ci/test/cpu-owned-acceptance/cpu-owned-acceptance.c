/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include "core/wz_machine.h"

#define CAPACITY 32u
#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "CPU acceptance failed at line %d: %s\n", \
                      __LINE__, #condition); \
        goto cleanup; \
    } \
} while (0)

typedef struct {
    wz_bus_request_t bus[CAPACITY];
    size_t bus_count;
    wz_trace_event_t trace[CAPACITY];
    size_t trace_count;
    bool overflow;
} capture_t;

static void record_bus(const wz_bus_request_t* request, void* context)
{
    capture_t* capture = (capture_t*)context;
    if (capture->bus_count >= CAPACITY) {
        capture->overflow = true;
        return;
    }
    capture->bus[capture->bus_count++] = *request;
}

static void record_trace(const wz_trace_event_t* event, void* context)
{
    capture_t* capture = (capture_t*)context;
    if (capture->trace_count >= CAPACITY) {
        capture->overflow = true;
        return;
    }
    capture->trace[capture->trace_count++] = *event;
}

int main(void)
{
    static const wz_bus_cycle_t cycles[4] = {
        WZ_BUS_M1_OPCODE_FETCH, WZ_BUS_MEMORY_READ,
        WZ_BUS_MEMORY_READ, WZ_BUS_MEMORY_READ
    };
    static const wz_master_tick_t ticks[4] = {0u, 8u, 14u, 20u};
    static const wz_word_t addresses[4] = {0x8000u, 0x8001u, 0x8002u, 0x9000u};
    static const wz_byte_t values[4] = {0x3au, 0x00u, 0x90u, 0x5au};
    static const wz_byte_t lengths[4] = {4u, 3u, 3u, 3u};
    wz_machine_t machine;
    wz_bus_observer_t observer;
    wz_trace_sink_t sink;
    capture_t capture = {0};
    size_t trace_bus_count = 0u;
    size_t instruction_count = 0u;
    size_t opcode_count = 0u;
    size_t sync_count = 0u;
    bool initialized = false;
    int result = 1;

    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) == WZ_RESULT_OK);
    initialized = true;
    wz_machine_memory_write(&machine, 0x8000u, 0x3au);
    wz_machine_memory_write(&machine, 0x8001u, 0x00u);
    wz_machine_memory_write(&machine, 0x8002u, 0x90u);
    wz_machine_memory_write(&machine, 0x9000u, 0x5au);
    machine.cpu.program_counter = 0x8000u;
    wz_bus_observer_init(&observer, record_bus, &capture);
    REQUIRE(wz_machine_set_bus_observer(&machine, &observer) == WZ_RESULT_OK);
    wz_trace_sink_init(&sink, record_trace, &capture);
    wz_machine_set_timing_trace(&machine, &sink);

    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_OK);
    REQUIRE(!capture.overflow && capture.bus_count == 4u);
    REQUIRE(machine.cpu.main.a == 0x5au);
    REQUIRE(machine.cpu.program_counter == 0x8003u);
    REQUIRE(machine.cpu.memptr == 0x9001u);
    REQUIRE(machine.master_tick == 26u);
    for (size_t index = 0u; index < 4u; ++index) {
        const wz_bus_request_t* bus = &capture.bus[index];
        REQUIRE(bus->cycle == cycles[index]);
        REQUIRE(bus->master_tick == ticks[index]);
        REQUIRE(bus->address == addresses[index]);
        REQUIRE(bus->value == values[index]);
        REQUIRE(bus->t_states == lengths[index]);
        REQUIRE(bus->contention_delay == 0u);
    }
    for (size_t index = 0u; index < capture.trace_count; ++index) {
        const wz_trace_event_t* event = &capture.trace[index];
        if (index != 0u) REQUIRE(event->sequence > capture.trace[index - 1u].sequence);
        if (event->kind == WZ_TRACE_CPU_BUS) {
            REQUIRE(trace_bus_count < 4u);
            REQUIRE(event->master_tick == ticks[trace_bus_count]);
            REQUIRE(event->address == addresses[trace_bus_count]);
            REQUIRE(event->value == values[trace_bus_count]);
            REQUIRE(event->cycle == (wz_byte_t)cycles[trace_bus_count]);
            REQUIRE(event->t_states == lengths[trace_bus_count]);
            ++trace_bus_count;
        } else if (event->kind == WZ_TRACE_CPU_INSTRUCTION) {
            REQUIRE(event->program_counter == 0x8000u && event->value == 0x3au);
            ++instruction_count;
        } else if (event->kind == WZ_TRACE_CPU_OPCODE_BYTE) {
            REQUIRE(event->address == 0x8000u && event->value == 0x3au);
            ++opcode_count;
        } else if (event->kind == WZ_TRACE_CPU_STATE_SYNC) {
            ++sync_count;
        }
    }
    REQUIRE(trace_bus_count == 4u && instruction_count == 1u);
    REQUIRE(opcode_count == 1u && sync_count == 5u);
    (void)printf("{\"testId\":\"cpu-owned-acceptance\",\"status\":\"pass\","
                 "\"instruction\":\"LD A,(9000h)\",\"busCycles\":%zu,"
                 "\"traceEvents\":%zu,\"masterTicks\":%" PRIu64 "}\n",
                 capture.bus_count, capture.trace_count,
                 (uint64_t)machine.master_tick);
    result = 0;

cleanup:
    if (initialized) wz_machine_destroy(&machine);
    return result;
}
