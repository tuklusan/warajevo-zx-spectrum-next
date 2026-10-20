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

#define MAX_EVENTS 64u
#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "line %d: %s\n", __LINE__, #condition); \
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

static bool check_bus(const capture_t* capture,
                     const expected_bus_t* expected,
                     size_t expected_count)
{
    if (capture->overflow || capture->bus_count != expected_count) return false;
    for (size_t index = 0u; index < expected_count; ++index) {
        const wz_bus_request_t* bus = &capture->bus[index];
        if (bus->cycle != expected[index].cycle ||
            bus->master_tick != expected[index].tick ||
            bus->address != expected[index].address ||
            bus->value != expected[index].value ||
            bus->t_states != expected[index].t_states ||
            bus->contention_delay != 0u) {
            return false;
        }
    }
    return true;
}

static bool check_interrupt_order(const capture_t* capture)
{
    static const wz_byte_t expected[] = {
        WZ_TRACE_INTERRUPT_NMI_ACCEPT,
        WZ_TRACE_INTERRUPT_MASKABLE_SAMPLE,
        WZ_TRACE_INTERRUPT_MASKABLE_SAMPLE,
        WZ_TRACE_INTERRUPT_MASKABLE_ACCEPT
    };
    size_t found = 0u;
    size_t traced_bus = 0u;
    for (size_t index = 0u; index < capture->trace_count; ++index) {
        const wz_trace_event_t* event = &capture->trace[index];
        if (index != 0u &&
            event->sequence <= capture->trace[index - 1u].sequence) {
            return false;
        }
        if (event->kind == WZ_TRACE_CPU_BUS) {
            const wz_bus_request_t* bus;
            if (traced_bus >= capture->bus_count) return false;
            bus = &capture->bus[traced_bus];
            ++traced_bus;
            if (event->master_tick != bus->master_tick ||
                event->address != bus->address ||
                event->value != bus->value ||
                event->cycle != (wz_byte_t)bus->cycle ||
                event->t_states != bus->t_states) {
                return false;
            }
            continue;
        }
        if (event->kind != WZ_TRACE_INTERRUPT) continue;
        if (event->value == WZ_TRACE_INTERRUPT_NMI_ACCEPT ||
            event->value == WZ_TRACE_INTERRUPT_MASKABLE_SAMPLE ||
            event->value == WZ_TRACE_INTERRUPT_MASKABLE_ACCEPT) {
            if (found >= sizeof(expected) / sizeof(expected[0]) ||
                event->value != expected[found]) {
                return false;
            }
            ++found;
        }
    }
    return found == sizeof(expected) / sizeof(expected[0]) &&
           traced_bus == capture->bus_count;
}

int main(void)
{
    static const expected_bus_t expected_bus[] = {
        {WZ_BUS_INTERNAL, 0u, 0x8000u, 0u, 5u},
        {WZ_BUS_MEMORY_WRITE, 10u, 0xfffdu, 0x80u, 3u},
        {WZ_BUS_MEMORY_WRITE, 16u, 0xfffcu, 0x00u, 3u},
        {WZ_BUS_M1_OPCODE_FETCH, 22u, 0x0066u, 0xedu, 4u},
        {WZ_BUS_M1_OPCODE_FETCH, 30u, 0x0067u, 0x45u, 4u},
        {WZ_BUS_MEMORY_READ, 38u, 0xfffcu, 0x00u, 3u},
        {WZ_BUS_MEMORY_READ, 44u, 0xfffdu, 0x80u, 3u},
        {WZ_BUS_INTERRUPT_ACKNOWLEDGE, 50u, 0x8000u, 0xffu, 7u},
        {WZ_BUS_MEMORY_WRITE, 64u, 0xfffdu, 0x80u, 3u},
        {WZ_BUS_MEMORY_WRITE, 70u, 0xfffcu, 0x00u, 3u}
    };
    wz_machine_t machine;
    wz_bus_observer_t observer;
    wz_trace_sink_t sink;
    capture_t capture = {0};
    bool initialized = false;
    bool passed = false;

    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    initialized = true;
    wz_machine_memory_write(&machine, 0x0066u, 0xedu);
    wz_machine_memory_write(&machine, 0x0067u, 0x45u);
    machine.cpu.program_counter = 0x8000u;
    machine.cpu.stack_pointer = 0xfffeu;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.interrupt_mode = WZ_Z80_INTERRUPT_MODE_1;
    wz_bus_observer_init(&observer, record_bus, &capture);
    REQUIRE(wz_machine_set_bus_observer(&machine, &observer) == WZ_RESULT_OK);
    wz_trace_sink_init(&sink, record_trace, &capture);
    wz_machine_set_timing_trace(&machine, &sink);

    wz_machine_update_interrupt_line(&machine);
    REQUIRE(wz_machine_maskable_interrupt_line_low(&machine));
    REQUIRE(wz_z80_accept_nmi(&machine) == WZ_RESULT_OK);
    REQUIRE(machine.cpu.program_counter == 0x0066u &&
            machine.cpu.stack_pointer == 0xfffcu &&
            machine.cpu.iff1 == 0u && machine.cpu.iff2 == 1u &&
            machine.master_tick == 22u);

    REQUIRE(wz_z80_sample_maskable_interrupt(&machine) ==
            WZ_RESULT_UNSUPPORTED_OPERATION);
    REQUIRE(capture.bus_count == 3u && machine.cpu.program_counter == 0x0066u &&
            machine.master_tick == 22u);

    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_OK);
    REQUIRE(machine.cpu.program_counter == 0x8000u &&
            machine.cpu.stack_pointer == 0xfffeu &&
            machine.cpu.iff1 == 1u && machine.cpu.iff2 == 1u &&
            machine.master_tick == 50u);
    REQUIRE(wz_machine_maskable_interrupt_line_low(&machine));

    REQUIRE(wz_z80_sample_maskable_interrupt(&machine) == WZ_RESULT_OK);
    REQUIRE(machine.cpu.program_counter == 0x0038u &&
            machine.cpu.stack_pointer == 0xfffcu &&
            machine.cpu.iff1 == 0u && machine.cpu.iff2 == 0u &&
            machine.master_tick == 76u);
    REQUIRE(check_bus(&capture, expected_bus,
                      sizeof(expected_bus) / sizeof(expected_bus[0])));
    REQUIRE(check_interrupt_order(&capture));
    passed = true;

cleanup:
    if (initialized) wz_machine_destroy(&machine);
    if (!passed) return 1;
    (void)puts("PASS NMI priority, rejected maskable sample, RETN restore, and pending IRQ acceptance");
    return 0;
}
