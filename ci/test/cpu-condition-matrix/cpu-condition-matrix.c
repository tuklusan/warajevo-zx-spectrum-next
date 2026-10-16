/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "core/wz_machine.h"

#define MAX_EVENTS 16u
#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "condition matrix opcode %02x path %u line %d: %s\n", \
                      (unsigned)opcode, (unsigned)taken, __LINE__, #condition); \
        goto cleanup; \
    } \
} while (0)

typedef struct {
    wz_bus_request_t bus[MAX_EVENTS];
    wz_trace_event_t trace[MAX_EVENTS];
    size_t bus_count;
    size_t trace_count;
    bool overflow;
} capture_t;

static void record_bus(const wz_bus_request_t* request, void* context)
{
    capture_t* capture = (capture_t*)context;
    if (capture->bus_count >= MAX_EVENTS) {
        capture->overflow = true;
        return;
    }
    capture->bus[capture->bus_count++] = *request;
}

static void record_trace(const wz_trace_event_t* event, void* context)
{
    capture_t* capture = (capture_t*)context;
    if (capture->trace_count >= MAX_EVENTS) {
        capture->overflow = true;
        return;
    }
    capture->trace[capture->trace_count++] = *event;
}

static bool run_case(wz_byte_t opcode, wz_byte_t flags, bool taken,
                     wz_byte_t displacement, bool indirect)
{
    wz_machine_t machine;
    wz_bus_observer_t observer;
    wz_trace_sink_t sink;
    capture_t capture = {0};
    wz_bus_cycle_t cycles[3] = {WZ_BUS_M1_OPCODE_FETCH, WZ_BUS_MEMORY_READ,
                                WZ_BUS_MEMORY_READ};
    wz_master_tick_t ticks[3] = {0u, 8u, 14u};
    wz_word_t addresses[3] = {0x8000u, 0x8001u, 0x8002u};
    wz_byte_t values[3] = {opcode, 0x04u, 0x80u};
    wz_byte_t t_states[3] = {4u, 3u, 3u};
    wz_word_t final_pc;
    wz_word_t final_memptr;
    wz_master_tick_t final_tick;
    size_t expected_count;
    size_t trace_bus_count = 0u;
    size_t instruction_count = 0u;
    size_t opcode_count = 0u;
    size_t sync_count = 0u;
    bool initialized = false;
    bool passed = false;
    bool relative = opcode < 0xc2u;

    if (indirect) {
        expected_count = 1u;
        final_pc = 0x9000u;
        final_memptr = 0u;
        final_tick = 8u;
    } else if (relative) {
        expected_count = taken ? 3u : 2u;
        values[1] = displacement;
        addresses[2] = 0x8001u;
        if (taken) {
            cycles[2] = WZ_BUS_INTERNAL;
            values[2] = 0u;
            t_states[2] = 5u;
        } else {
            cycles[1] = WZ_BUS_INTERNAL;
            values[1] = 0u;
        }
        final_pc = taken ? (wz_word_t)(0x8002u +
            (wz_word_t)(int16_t)(int8_t)displacement) : 0x8002u;
        final_memptr = taken ? final_pc : 0u;
        final_tick = taken ? 24u : 14u;
    } else {
        expected_count = 3u;
        final_pc = taken ? 0x8004u : 0x8003u;
        final_memptr = 0x8004u;
        final_tick = 20u;
    }

    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    initialized = true;
    wz_machine_memory_write(&machine, 0x8000u, opcode);
    wz_machine_memory_write(&machine, 0x8001u,
                            relative ? displacement : 0x04u);
    wz_machine_memory_write(&machine, 0x8002u, 0x80u);
    machine.cpu.program_counter = 0x8000u;
    machine.cpu.main.f = flags;
    machine.cpu.main.h = 0x90u;
    machine.cpu.main.l = 0x00u;
    wz_bus_observer_init(&observer, record_bus, &capture);
    REQUIRE(wz_machine_set_bus_observer(&machine, &observer) == WZ_RESULT_OK);
    wz_trace_sink_init(&sink, record_trace, &capture);
    wz_machine_set_timing_trace(&machine, &sink);

    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_OK);
    REQUIRE(!capture.overflow && capture.bus_count == expected_count);
    REQUIRE(machine.cpu.program_counter == final_pc);
    REQUIRE(machine.cpu.memptr == final_memptr);
    REQUIRE(machine.cpu.main.f == flags);
    REQUIRE(machine.cpu.r == 1u);
    REQUIRE(machine.master_tick == final_tick);
    for (size_t index = 0u; index < expected_count; ++index) {
        const wz_bus_request_t* bus = &capture.bus[index];
        REQUIRE(bus->cycle == cycles[index]);
        REQUIRE(bus->master_tick == ticks[index]);
        REQUIRE(bus->address == addresses[index]);
        REQUIRE(bus->value == values[index]);
        REQUIRE(bus->t_states == t_states[index]);
        REQUIRE(bus->contention_delay == 0u);
    }
    for (size_t index = 0u; index < capture.trace_count; ++index) {
        const wz_trace_event_t* event = &capture.trace[index];
        if (index != 0u) REQUIRE(event->sequence > capture.trace[index - 1u].sequence);
        if (event->kind == WZ_TRACE_CPU_BUS) {
            REQUIRE(trace_bus_count < expected_count);
            REQUIRE(event->master_tick == ticks[trace_bus_count]);
            REQUIRE(event->address == addresses[trace_bus_count]);
            REQUIRE(event->value == values[trace_bus_count]);
            REQUIRE(event->cycle == (wz_byte_t)cycles[trace_bus_count]);
            REQUIRE(event->t_states == t_states[trace_bus_count]);
            ++trace_bus_count;
        } else if (event->kind == WZ_TRACE_CPU_INSTRUCTION) {
            REQUIRE(event->program_counter == 0x8000u && event->value == opcode);
            ++instruction_count;
        } else if (event->kind == WZ_TRACE_CPU_OPCODE_BYTE) {
            REQUIRE(event->address == 0x8000u && event->value == opcode);
            ++opcode_count;
        } else if (event->kind == WZ_TRACE_CPU_STATE_SYNC) {
            ++sync_count;
        }
    }
    REQUIRE(trace_bus_count == expected_count);
    REQUIRE(instruction_count == 1u && opcode_count == 1u && sync_count == 5u);
    passed = true;

cleanup:
    if (initialized) wz_machine_destroy(&machine);
    return passed;
}

int main(void)
{
    static const wz_byte_t jr_opcodes[4] = {0x20u, 0x28u, 0x30u, 0x38u};
    static const wz_byte_t jp_opcodes[8] = {
        0xc2u, 0xcau, 0xd2u, 0xdau, 0xe2u, 0xeau, 0xf2u, 0xfau
    };
    static const wz_byte_t masks[4] = {0x40u, 0x01u, 0x04u, 0x80u};
    size_t cases = 0u;
    for (size_t index = 0u; index < 4u; ++index) {
        bool wants_set = (index & 1u) != 0u;
        for (unsigned path = 0u; path < 2u; ++path) {
            bool taken = path != 0u;
            wz_byte_t flags = wants_set == taken ? masks[index / 2u] : 0u;
            if (!run_case(jr_opcodes[index], flags, taken, 0x02u, false)) return 1;
            ++cases;
        }
    }
    for (size_t index = 0u; index < 8u; ++index) {
        bool wants_set = (index & 1u) != 0u;
        for (unsigned path = 0u; path < 2u; ++path) {
            bool taken = path != 0u;
            wz_byte_t flags = wants_set == taken ? masks[index / 2u] : 0u;
            if (!run_case(jp_opcodes[index], flags, taken, 0u, false)) return 1;
            ++cases;
        }
    }
    if (!run_case(0x18u, 0u, true, 0x02u, false) ||
        !run_case(0x18u, 0u, true, 0xfeu, false) ||
        !run_case(0xc3u, 0u, true, 0u, false) ||
        !run_case(0xe9u, 0u, true, 0u, true)) {
        return 1;
    }
    cases += 4u;
    (void)printf("{\"testId\":\"cpu-condition-matrix\",\"status\":\"pass\","
                 "\"cases\":%zu,\"jrConditionalPaths\":8,"
                 "\"jpConditionalPaths\":16,\"unconditionalPaths\":4}\n", cases);
    return 0;
}
