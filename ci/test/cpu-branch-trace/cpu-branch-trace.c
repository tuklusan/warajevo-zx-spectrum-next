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
#define REQUIRE_CASE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "%s failed at line %d: %s\n", \
                      test->name, __LINE__, #condition); \
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

typedef struct {
    wz_bus_cycle_t cycle;
    wz_master_tick_t tick;
    wz_word_t address;
    wz_byte_t value;
    wz_byte_t t_states;
} expected_bus_t;

typedef struct {
    const char* name;
    wz_byte_t opcode;
    wz_byte_t flags;
    wz_byte_t b_initial;
    wz_word_t final_pc;
    wz_byte_t final_b;
    wz_master_tick_t final_tick;
    size_t bus_count;
    expected_bus_t bus[4];
} branch_case_t;

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

static bool run_case(const branch_case_t* test)
{
    wz_machine_t machine;
    wz_bus_observer_t observer;
    wz_trace_sink_t sink;
    capture_t capture = {0};
    size_t trace_bus_count = 0u;
    size_t instruction_count = 0u;
    size_t opcode_count = 0u;
    bool initialized = false;
    bool passed = false;

    REQUIRE_CASE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
                 WZ_RESULT_OK);
    initialized = true;
    wz_machine_memory_write(&machine, 0x8000u, test->opcode);
    wz_machine_memory_write(&machine, 0x8001u, 0x02u);
    machine.cpu.program_counter = 0x8000u;
    machine.cpu.main.f = test->flags;
    machine.cpu.main.b = test->b_initial;
    wz_bus_observer_init(&observer, record_bus, &capture);
    REQUIRE_CASE(wz_machine_set_bus_observer(&machine, &observer) == WZ_RESULT_OK);
    wz_trace_sink_init(&sink, record_trace, &capture);
    wz_machine_set_timing_trace(&machine, &sink);

    REQUIRE_CASE(wz_z80_step(&machine) == WZ_RESULT_OK);
    REQUIRE_CASE(!capture.overflow && capture.bus_count == test->bus_count);
    REQUIRE_CASE(machine.cpu.program_counter == test->final_pc);
    REQUIRE_CASE(machine.cpu.main.b == test->final_b);
    REQUIRE_CASE(machine.cpu.main.f == test->flags);
    REQUIRE_CASE(machine.master_tick == test->final_tick);
    REQUIRE_CASE(machine.cpu.r == 1u);
    for (size_t index = 0u; index < test->bus_count; ++index) {
        const wz_bus_request_t* actual = &capture.bus[index];
        const expected_bus_t* expected = &test->bus[index];
        REQUIRE_CASE(actual->cycle == expected->cycle);
        REQUIRE_CASE(actual->master_tick == expected->tick);
        REQUIRE_CASE(actual->address == expected->address);
        REQUIRE_CASE(actual->value == expected->value);
        REQUIRE_CASE(actual->t_states == expected->t_states);
        REQUIRE_CASE(actual->contention_delay == 0u);
    }
    for (size_t index = 0u; index < capture.trace_count; ++index) {
        const wz_trace_event_t* event = &capture.trace[index];
        if (index != 0u) {
            REQUIRE_CASE(event->sequence > capture.trace[index - 1u].sequence);
        }
        if (event->kind == WZ_TRACE_CPU_BUS) {
            const expected_bus_t* expected;
            REQUIRE_CASE(trace_bus_count < test->bus_count);
            expected = &test->bus[trace_bus_count++];
            REQUIRE_CASE(event->master_tick == expected->tick);
            REQUIRE_CASE(event->address == expected->address);
            REQUIRE_CASE(event->value == expected->value);
            REQUIRE_CASE(event->cycle == (wz_byte_t)expected->cycle);
            REQUIRE_CASE(event->t_states == expected->t_states);
        } else if (event->kind == WZ_TRACE_CPU_INSTRUCTION) {
            REQUIRE_CASE(event->program_counter == 0x8000u);
            REQUIRE_CASE(event->value == test->opcode);
            ++instruction_count;
        } else if (event->kind == WZ_TRACE_CPU_OPCODE_BYTE) {
            REQUIRE_CASE(event->address == 0x8000u);
            REQUIRE_CASE(event->value == test->opcode);
            ++opcode_count;
        }
    }
    REQUIRE_CASE(trace_bus_count == test->bus_count);
    REQUIRE_CASE(instruction_count == 1u && opcode_count == 1u);
    passed = true;

cleanup:
    if (initialized) wz_machine_destroy(&machine);
    return passed;
}

int main(void)
{
    static const branch_case_t cases[] = {
        {"JR NZ taken", 0x20u, 0x00u, 0u, 0x8004u, 0u, 24u, 3u,
         {{WZ_BUS_M1_OPCODE_FETCH, 0u, 0x8000u, 0x20u, 4u},
          {WZ_BUS_MEMORY_READ, 8u, 0x8001u, 0x02u, 3u},
          {WZ_BUS_INTERNAL, 14u, 0x8001u, 0x00u, 5u}}},
        {"JR NZ not taken", 0x20u, 0x40u, 0u, 0x8002u, 0u, 14u, 2u,
         {{WZ_BUS_M1_OPCODE_FETCH, 0u, 0x8000u, 0x20u, 4u},
          {WZ_BUS_INTERNAL, 8u, 0x8001u, 0x00u, 3u}}},
        {"DJNZ taken", 0x10u, 0x55u, 2u, 0x8004u, 1u, 26u, 4u,
         {{WZ_BUS_M1_OPCODE_FETCH, 0u, 0x8000u, 0x10u, 4u},
          {WZ_BUS_INTERNAL, 8u, 0x8001u, 0x00u, 1u},
          {WZ_BUS_MEMORY_READ, 10u, 0x8001u, 0x02u, 3u},
          {WZ_BUS_INTERNAL, 16u, 0x8001u, 0x00u, 5u}}},
        {"DJNZ not taken", 0x10u, 0x55u, 1u, 0x8002u, 0u, 16u, 3u,
         {{WZ_BUS_M1_OPCODE_FETCH, 0u, 0x8000u, 0x10u, 4u},
          {WZ_BUS_INTERNAL, 8u, 0x8001u, 0x00u, 1u},
          {WZ_BUS_INTERNAL, 10u, 0x8001u, 0x00u, 3u}}}
    };
    for (size_t index = 0u; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        if (!run_case(&cases[index])) return 1;
    }
    (void)puts("{\"testId\":\"cpu-branch-trace\",\"status\":\"pass\","
               "\"cases\":4,\"paths\":[\"JR NZ taken\",\"JR NZ not taken\","
               "\"DJNZ taken\",\"DJNZ not taken\"]}");
    return 0;
}
