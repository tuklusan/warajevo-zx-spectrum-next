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

typedef struct {
    wz_bus_cycle_t cycle;
    wz_master_tick_t tick;
    wz_word_t address;
    wz_byte_t value;
    wz_byte_t states;
} expected_bus_t;

typedef struct {
    wz_bus_request_t bus[8];
    wz_trace_event_t trace[40];
    size_t bus_count;
    size_t trace_count;
    bool overflow;
    wz_byte_t vector;
} capture_t;

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "%s failed at line %d: %s\n", \
                      name, __LINE__, #condition); \
        goto cleanup; \
    } \
} while (0)

static void record_bus(const wz_bus_request_t* request, void* context)
{
    capture_t* capture = (capture_t*)context;
    if (capture->bus_count >= 8u) {
        capture->overflow = true;
        return;
    }
    capture->bus[capture->bus_count++] = *request;
}

static void record_trace(const wz_trace_event_t* event, void* context)
{
    capture_t* capture = (capture_t*)context;
    if (capture->trace_count >= 40u) {
        capture->overflow = true;
        return;
    }
    capture->trace[capture->trace_count++] = *event;
}

static wz_byte_t read_vector(wz_bus_cycle_t cycle, wz_word_t address,
                             void* context)
{
    capture_t* capture = (capture_t*)context;
    (void)cycle;
    (void)address;
    return capture->vector;
}

static bool check_capture(const char* name, const capture_t* capture,
                          const expected_bus_t* expected, size_t count,
                          size_t instruction_count)
{
    size_t traced_bus = 0u;
    size_t traced_instruction = 0u;
    size_t traced_opcode = 0u;
    size_t traced_sample = 0u;
    size_t traced_accept = 0u;
    if (capture->overflow || capture->bus_count != count) {
        (void)fprintf(stderr, "%s: bus count mismatch\n", name);
        return false;
    }
    for (size_t index = 0u; index < count; ++index) {
        const wz_bus_request_t* bus = &capture->bus[index];
        if (bus->cycle != expected[index].cycle ||
            bus->master_tick != expected[index].tick ||
            bus->address != expected[index].address ||
            bus->value != expected[index].value ||
            bus->t_states != expected[index].states ||
            bus->contention_delay != 0u) {
            (void)fprintf(stderr, "%s: bus event %zu mismatch\n", name, index);
            return false;
        }
    }
    for (size_t index = 0u; index < capture->trace_count; ++index) {
        const wz_trace_event_t* event = &capture->trace[index];
        if (index != 0u &&
            event->sequence <= capture->trace[index - 1u].sequence) {
            (void)fprintf(stderr, "%s: trace order mismatch\n", name);
            return false;
        }
        if (event->kind == WZ_TRACE_CPU_BUS) {
            if (traced_bus >= count ||
                event->master_tick != expected[traced_bus].tick ||
                event->address != expected[traced_bus].address ||
                event->value != expected[traced_bus].value ||
                event->cycle != (wz_byte_t)expected[traced_bus].cycle ||
                event->t_states != expected[traced_bus].states) {
                (void)fprintf(stderr, "%s: trace bus mismatch\n", name);
                return false;
            }
            ++traced_bus;
        } else if (event->kind == WZ_TRACE_CPU_INSTRUCTION) {
            ++traced_instruction;
        } else if (event->kind == WZ_TRACE_CPU_OPCODE_BYTE) {
            ++traced_opcode;
        } else if (event->kind == WZ_TRACE_INTERRUPT) {
            if (event->value == WZ_TRACE_INTERRUPT_MASKABLE_SAMPLE) {
                ++traced_sample;
            } else if (event->value == WZ_TRACE_INTERRUPT_MASKABLE_ACCEPT) {
                ++traced_accept;
            }
        }
    }
    if (traced_bus != count || traced_instruction != instruction_count ||
        traced_opcode != 0u || traced_sample != 1u || traced_accept != 1u) {
        (void)fprintf(stderr, "%s: trace event counts mismatch\n", name);
        return false;
    }
    return true;
}

static bool interrupt_mode_zero(void)
{
    const char* name = "IM 0 injected RST 38h";
    static const expected_bus_t expected[] = {
        {WZ_BUS_INTERRUPT_ACKNOWLEDGE, 0u, 0x8000u, 0xffu, 7u},
        {WZ_BUS_INTERNAL, 14u, 0x8000u, 0x00u, 1u},
        {WZ_BUS_MEMORY_WRITE, 16u, 0xfffeu, 0x80u, 3u},
        {WZ_BUS_MEMORY_WRITE, 22u, 0xfffdu, 0x00u, 3u}
    };
    wz_machine_t machine;
    wz_bus_observer_t observer;
    wz_bus_input_t input;
    wz_trace_sink_t sink;
    capture_t capture = {0};
    bool initialized = false;
    bool passed = false;
    capture.vector = 0xffu;
    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    initialized = true;
    machine.cpu.program_counter = 0x8000u;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.interrupt_mode = WZ_Z80_INTERRUPT_MODE_0;
    wz_bus_observer_init(&observer, record_bus, &capture);
    REQUIRE(wz_machine_set_bus_observer(&machine, &observer) == WZ_RESULT_OK);
    wz_bus_input_init(&input, read_vector, &capture);
    REQUIRE(wz_machine_set_bus_input(&machine, &input) == WZ_RESULT_OK);
    wz_trace_sink_init(&sink, record_trace, &capture);
    wz_machine_set_timing_trace(&machine, &sink);
    REQUIRE(wz_z80_accept_maskable_interrupt(&machine) == WZ_RESULT_OK);
    REQUIRE(machine.cpu.program_counter == 0x0038u);
    REQUIRE(machine.cpu.stack_pointer == 0xfffdu);
    REQUIRE(machine.cpu.r == 1u && machine.master_tick == 28u);
    REQUIRE(machine.cpu.iff1 == 0u && machine.cpu.iff2 == 0u);
    REQUIRE(wz_machine_memory_read(&machine, 0xfffeu) == 0x80u);
    REQUIRE(wz_machine_memory_read(&machine, 0xfffdu) == 0x00u);
    REQUIRE(check_capture(name, &capture, expected,
        sizeof(expected) / sizeof(expected[0]), 1u));
    passed = true;
cleanup:
    if (initialized) wz_machine_destroy(&machine);
    return passed;
}

static bool interrupt_mode_two(void)
{
    const char* name = "IM 2 vector table";
    static const expected_bus_t expected[] = {
        {WZ_BUS_INTERRUPT_ACKNOWLEDGE, 0u, 0x8000u, 0x10u, 7u},
        {WZ_BUS_MEMORY_READ, 14u, 0x9010u, 0x34u, 3u},
        {WZ_BUS_MEMORY_READ, 20u, 0x9011u, 0x12u, 3u},
        {WZ_BUS_MEMORY_WRITE, 26u, 0xfffeu, 0x80u, 3u},
        {WZ_BUS_MEMORY_WRITE, 32u, 0xfffdu, 0x00u, 3u}
    };
    wz_machine_t machine;
    wz_bus_observer_t observer;
    wz_bus_input_t input;
    wz_trace_sink_t sink;
    capture_t capture = {0};
    bool initialized = false;
    bool passed = false;
    capture.vector = 0x10u;
    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    initialized = true;
    wz_machine_memory_write(&machine, 0x9010u, 0x34u);
    wz_machine_memory_write(&machine, 0x9011u, 0x12u);
    machine.cpu.program_counter = 0x8000u;
    machine.cpu.i = 0x90u;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.interrupt_mode = WZ_Z80_INTERRUPT_MODE_2;
    wz_bus_observer_init(&observer, record_bus, &capture);
    REQUIRE(wz_machine_set_bus_observer(&machine, &observer) == WZ_RESULT_OK);
    wz_bus_input_init(&input, read_vector, &capture);
    REQUIRE(wz_machine_set_bus_input(&machine, &input) == WZ_RESULT_OK);
    wz_trace_sink_init(&sink, record_trace, &capture);
    wz_machine_set_timing_trace(&machine, &sink);
    REQUIRE(wz_z80_accept_maskable_interrupt(&machine) == WZ_RESULT_OK);
    REQUIRE(machine.cpu.program_counter == 0x1234u);
    REQUIRE(machine.cpu.stack_pointer == 0xfffdu);
    REQUIRE(machine.cpu.r == 1u && machine.master_tick == 38u);
    REQUIRE(machine.cpu.iff1 == 0u && machine.cpu.iff2 == 0u);
    REQUIRE(wz_machine_memory_read(&machine, 0xfffeu) == 0x80u);
    REQUIRE(wz_machine_memory_read(&machine, 0xfffdu) == 0x00u);
    REQUIRE(check_capture(name, &capture, expected,
        sizeof(expected) / sizeof(expected[0]), 0u));
    passed = true;
cleanup:
    if (initialized) wz_machine_destroy(&machine);
    return passed;
}

int main(void)
{
    if (!interrupt_mode_zero() || !interrupt_mode_two()) return 1;
    (void)puts("{\"testId\":\"cpu-interrupt-modes\",\"status\":\"pass\","
               "\"cases\":2,\"im0InjectedOpcode\":true,\"im2Vector\":true}");
    return 0;
}
