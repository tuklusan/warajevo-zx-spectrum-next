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

#define MAX_BUS 16u
#define MAX_TRACE 64u
#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "%s failed at line %d: %s\n", \
                      name, __LINE__, #condition); \
        goto cleanup; \
    } \
} while (0)

typedef struct {
    wz_bus_request_t bus[MAX_BUS];
    wz_trace_event_t trace[MAX_TRACE];
    size_t bus_count;
    size_t trace_count;
    bool overflow;
} capture_t;

typedef struct {
    wz_bus_cycle_t cycle;
    wz_master_tick_t tick;
    wz_word_t address;
    wz_byte_t value;
    wz_byte_t states;
} expected_bus_t;

static void record_bus(const wz_bus_request_t* request, void* context)
{
    capture_t* capture = (capture_t*)context;
    if (capture->bus_count >= MAX_BUS) {
        capture->overflow = true;
        return;
    }
    capture->bus[capture->bus_count++] = *request;
}

static void record_trace(const wz_trace_event_t* event, void* context)
{
    capture_t* capture = (capture_t*)context;
    if (capture->trace_count >= MAX_TRACE) {
        capture->overflow = true;
        return;
    }
    capture->trace[capture->trace_count++] = *event;
}

static bool check_bus_and_trace(const char* name, const capture_t* capture,
                                const expected_bus_t* expected, size_t count,
                                size_t instruction_count,
                                size_t opcode_count,
                                size_t sample_count,
                                size_t accept_count)
{
    size_t traced_bus = 0u;
    size_t traced_instructions = 0u;
    size_t traced_opcodes = 0u;
    size_t traced_samples = 0u;
    size_t traced_accepts = 0u;
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
            ++traced_instructions;
        } else if (event->kind == WZ_TRACE_CPU_OPCODE_BYTE) {
            ++traced_opcodes;
        } else if (event->kind == WZ_TRACE_INTERRUPT) {
            if (event->value == WZ_TRACE_INTERRUPT_MASKABLE_SAMPLE) {
                ++traced_samples;
            } else if (event->value == WZ_TRACE_INTERRUPT_MASKABLE_ACCEPT ||
                       event->value == WZ_TRACE_INTERRUPT_NMI_ACCEPT) {
                ++traced_accepts;
            }
        }
    }
    if (traced_bus != count ||
        traced_instructions != instruction_count ||
        traced_opcodes != opcode_count ||
        traced_samples != sample_count ||
        traced_accepts != accept_count) {
        (void)fprintf(stderr, "%s: trace event counts mismatch\n", name);
        return false;
    }
    return true;
}

static bool halt_then_im1(void)
{
    const char* name = "HALT then IM1";
    static const expected_bus_t expected[] = {
        {WZ_BUS_M1_OPCODE_FETCH, 0u, 0x8000u, 0x76u, 4u},
        {WZ_BUS_M1_OPCODE_FETCH, 8u, 0x8000u, 0x76u, 4u},
        {WZ_BUS_M1_OPCODE_FETCH, 16u, 0x8000u, 0x76u, 4u},
        {WZ_BUS_INTERRUPT_ACKNOWLEDGE, 24u, 0x8000u, 0xffu, 7u},
        {WZ_BUS_MEMORY_WRITE, 38u, 0xfffeu, 0x80u, 3u},
        {WZ_BUS_MEMORY_WRITE, 44u, 0xfffdu, 0x01u, 3u}
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
    wz_machine_memory_write(&machine, 0x8000u, 0x76u);
    machine.cpu.program_counter = 0x8000u;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.interrupt_mode = WZ_Z80_INTERRUPT_MODE_1;
    wz_bus_observer_init(&observer, record_bus, &capture);
    REQUIRE(wz_machine_set_bus_observer(&machine, &observer) == WZ_RESULT_OK);
    wz_trace_sink_init(&sink, record_trace, &capture);
    wz_machine_set_timing_trace(&machine, &sink);
    for (unsigned index = 0u; index < 3u; ++index) {
        REQUIRE(wz_z80_step(&machine) == WZ_RESULT_OK);
        REQUIRE(machine.cpu.halted == 1u && machine.cpu.program_counter == 0x8000u);
        REQUIRE(machine.cpu.r == (wz_byte_t)(index + 1u));
        REQUIRE(machine.master_tick == (wz_master_tick_t)(8u * (index + 1u)));
    }
    REQUIRE(wz_z80_accept_maskable_interrupt(&machine) == WZ_RESULT_OK);
    REQUIRE(machine.cpu.halted == 0u && machine.cpu.program_counter == 0x0038u);
    REQUIRE(machine.cpu.stack_pointer == 0xfffdu && machine.cpu.r == 4u);
    REQUIRE(machine.cpu.iff1 == 0u && machine.cpu.iff2 == 0u);
    REQUIRE(machine.master_tick == 50u);
    REQUIRE(wz_machine_memory_read(&machine, 0xfffeu) == 0x80u);
    REQUIRE(wz_machine_memory_read(&machine, 0xfffdu) == 0x01u);
    REQUIRE(check_bus_and_trace(name, &capture, expected,
        sizeof(expected) / sizeof(expected[0]), 1u, 3u, 1u, 1u));
    passed = true;
cleanup:
    if (initialized) wz_machine_destroy(&machine);
    return passed;
}

static bool halt_then_nmi(void)
{
    const char* name = "HALT then NMI";
    static const expected_bus_t expected[] = {
        {WZ_BUS_M1_OPCODE_FETCH, 0u, 0x8000u, 0x76u, 4u},
        {WZ_BUS_INTERNAL, 8u, 0x8001u, 0x00u, 5u},
        {WZ_BUS_MEMORY_WRITE, 18u, 0xfffeu, 0x80u, 3u},
        {WZ_BUS_MEMORY_WRITE, 24u, 0xfffdu, 0x01u, 3u}
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
    wz_machine_memory_write(&machine, 0x8000u, 0x76u);
    machine.cpu.program_counter = 0x8000u;
    machine.cpu.iff1 = 1u;
    wz_bus_observer_init(&observer, record_bus, &capture);
    REQUIRE(wz_machine_set_bus_observer(&machine, &observer) == WZ_RESULT_OK);
    wz_trace_sink_init(&sink, record_trace, &capture);
    wz_machine_set_timing_trace(&machine, &sink);
    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_OK);
    REQUIRE(machine.cpu.halted == 1u && machine.master_tick == 8u);
    REQUIRE(wz_z80_accept_nmi(&machine) == WZ_RESULT_OK);
    REQUIRE(machine.cpu.halted == 0u && machine.cpu.program_counter == 0x0066u);
    REQUIRE(machine.cpu.stack_pointer == 0xfffdu && machine.cpu.r == 2u);
    REQUIRE(machine.cpu.iff1 == 0u && machine.cpu.iff2 == 1u);
    REQUIRE(machine.master_tick == 30u);
    REQUIRE(wz_machine_memory_read(&machine, 0xfffeu) == 0x80u);
    REQUIRE(wz_machine_memory_read(&machine, 0xfffdu) == 0x01u);
    REQUIRE(check_bus_and_trace(name, &capture, expected,
        sizeof(expected) / sizeof(expected[0]), 1u, 1u, 0u, 1u));
    passed = true;
cleanup:
    if (initialized) wz_machine_destroy(&machine);
    return passed;
}

static bool ei_delay_and_di(void)
{
    const char* name = "EI delay and DI";
    static const wz_byte_t program[] = {0xfbu, 0x00u, 0xf3u, 0xfbu, 0x00u};
    static const expected_bus_t expected[] = {
        {WZ_BUS_M1_OPCODE_FETCH, 0u, 0x8000u, 0xfbu, 4u},
        {WZ_BUS_M1_OPCODE_FETCH, 8u, 0x8001u, 0x00u, 4u},
        {WZ_BUS_M1_OPCODE_FETCH, 16u, 0x8002u, 0xf3u, 4u},
        {WZ_BUS_M1_OPCODE_FETCH, 24u, 0x8003u, 0xfbu, 4u},
        {WZ_BUS_M1_OPCODE_FETCH, 32u, 0x8004u, 0x00u, 4u},
        {WZ_BUS_INTERRUPT_ACKNOWLEDGE, 40u, 0x8005u, 0xffu, 7u},
        {WZ_BUS_MEMORY_WRITE, 54u, 0xfffeu, 0x80u, 3u},
        {WZ_BUS_MEMORY_WRITE, 60u, 0xfffdu, 0x05u, 3u}
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
    for (size_t index = 0u; index < sizeof(program); ++index) {
        wz_machine_memory_write(&machine, (wz_word_t)(0x8000u + index),
                                program[index]);
    }
    machine.cpu.program_counter = 0x8000u;
    machine.cpu.interrupt_mode = WZ_Z80_INTERRUPT_MODE_1;
    wz_bus_observer_init(&observer, record_bus, &capture);
    REQUIRE(wz_machine_set_bus_observer(&machine, &observer) == WZ_RESULT_OK);
    wz_trace_sink_init(&sink, record_trace, &capture);
    wz_machine_set_timing_trace(&machine, &sink);
    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_OK);
    REQUIRE(machine.cpu.iff1 == 1u && machine.cpu.iff2 == 1u);
    REQUIRE(machine.cpu.interrupt_enable_delay == 1u);
    REQUIRE(!wz_z80_maskable_interrupts_acceptable(&machine.cpu));
    REQUIRE(wz_z80_accept_maskable_interrupt(&machine) ==
            WZ_RESULT_UNSUPPORTED_OPERATION);
    REQUIRE(machine.master_tick == 8u && capture.bus_count == 1u);
    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_OK);
    REQUIRE(wz_z80_maskable_interrupts_acceptable(&machine.cpu));
    REQUIRE(machine.cpu.interrupt_enable_delay == 0u);
    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_OK);
    REQUIRE(machine.cpu.iff1 == 0u && machine.cpu.iff2 == 0u);
    REQUIRE(!wz_z80_maskable_interrupts_acceptable(&machine.cpu));
    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_OK);
    REQUIRE(machine.cpu.interrupt_enable_delay == 1u);
    REQUIRE(!wz_z80_maskable_interrupts_acceptable(&machine.cpu));
    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_OK);
    REQUIRE(wz_z80_maskable_interrupts_acceptable(&machine.cpu));
    REQUIRE(machine.cpu.program_counter == 0x8005u && machine.master_tick == 40u);
    REQUIRE(wz_z80_accept_maskable_interrupt(&machine) == WZ_RESULT_OK);
    REQUIRE(machine.cpu.program_counter == 0x0038u);
    REQUIRE(machine.cpu.stack_pointer == 0xfffdu && machine.cpu.r == 6u);
    REQUIRE(machine.cpu.iff1 == 0u && machine.cpu.iff2 == 0u);
    REQUIRE(machine.master_tick == 66u);
    REQUIRE(wz_machine_memory_read(&machine, 0xfffeu) == 0x80u);
    REQUIRE(wz_machine_memory_read(&machine, 0xfffdu) == 0x05u);
    REQUIRE(check_bus_and_trace(name, &capture, expected,
        sizeof(expected) / sizeof(expected[0]), 5u, 5u, 2u, 1u));
    passed = true;
cleanup:
    if (initialized) wz_machine_destroy(&machine);
    return passed;
}

int main(void)
{
    if (!halt_then_im1() || !halt_then_nmi() || !ei_delay_and_di()) return 1;
    (void)puts("{\"testId\":\"cpu-halt-interrupt\",\"status\":\"pass\","
               "\"cases\":3,\"haltFetches\":3,\"eiDelayRejected\":true,"
               "\"im1Wake\":true,\"nmiWake\":true}");
    return 0;
}
