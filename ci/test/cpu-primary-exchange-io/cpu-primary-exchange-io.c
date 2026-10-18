/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "core/wz_machine.h"

enum { MAX_EVENTS = 24 };

typedef struct {
    wz_bus_request_t bus[MAX_EVENTS];
    wz_trace_event_t trace[MAX_EVENTS];
    size_t bus_count;
    size_t trace_count;
    wz_byte_t input_value;
    wz_word_t input_port;
    bool overflow;
} capture_t;

typedef struct {
    wz_bus_cycle_t cycle;
    wz_master_tick_t tick;
    wz_word_t address;
    wz_byte_t value;
    wz_byte_t duration;
} expected_bus_t;

static void record_bus(const wz_bus_request_t* request, void* context)
{
    capture_t* capture = (capture_t*)context;
    if (capture->bus_count >= MAX_EVENTS) capture->overflow = true;
    else capture->bus[capture->bus_count++] = *request;
}

static void record_trace(const wz_trace_event_t* event, void* context)
{
    capture_t* capture = (capture_t*)context;
    if (capture->trace_count >= MAX_EVENTS) capture->overflow = true;
    else capture->trace[capture->trace_count++] = *event;
}

static wz_byte_t read_port(wz_bus_cycle_t cycle, wz_word_t address, void* context)
{
    const capture_t* capture = (const capture_t*)context;
    return cycle == WZ_BUS_IO_READ && address == capture->input_port ?
        capture->input_value : 0xffu;
}

static bool check_bus_trace(const capture_t* capture,
                            const expected_bus_t* expected,
                            size_t count, wz_byte_t opcode)
{
    size_t bus_events = 0u;
    size_t opcode_events = 0u;
    size_t instruction_events = 0u;
    if (capture->overflow || capture->bus_count != count) return false;
    for (size_t index = 0u; index < count; ++index) {
        const wz_bus_request_t* actual = &capture->bus[index];
        if (actual->cycle != expected[index].cycle ||
            actual->master_tick != expected[index].tick ||
            actual->address != expected[index].address ||
            actual->value != expected[index].value ||
            actual->t_states != expected[index].duration ||
            actual->contention_delay != 0u) return false;
    }
    for (size_t index = 0u; index < capture->trace_count; ++index) {
        const wz_trace_event_t* event = &capture->trace[index];
        if (index != 0u && event->sequence <= capture->trace[index - 1u].sequence) {
            return false;
        }
        if (event->kind == WZ_TRACE_CPU_BUS) {
            if (bus_events >= count ||
                event->master_tick != expected[bus_events].tick ||
                event->address != expected[bus_events].address ||
                event->value != expected[bus_events].value ||
                event->cycle != (wz_byte_t)expected[bus_events].cycle ||
                event->t_states != expected[bus_events].duration) return false;
            ++bus_events;
        } else if (event->kind == WZ_TRACE_CPU_OPCODE_BYTE) {
            if (opcode_events != 0u ||
                event->address != 0x8000u || event->value != opcode) return false;
            ++opcode_events;
        } else if (event->kind == WZ_TRACE_CPU_INSTRUCTION) {
            if (event->program_counter != 0x8000u || event->value != opcode) return false;
            ++instruction_events;
        }
    }
    return bus_events == count && opcode_events == 1u && instruction_events == 1u;
}

static bool run_case(wz_machine_t* machine, capture_t* capture,
                     wz_byte_t opcode, wz_byte_t a, wz_byte_t input,
                     wz_byte_t flags)
{
    const wz_master_tick_t base = machine->master_tick;
    const bool io = opcode == 0xd3u || opcode == 0xdbu;
    const wz_word_t port = (wz_word_t)(((wz_word_t)a << 8u) | 0x35u);
    wz_z80_state_t expected;
    expected_bus_t bus[6] = {{WZ_BUS_M1_OPCODE_FETCH,
                               base, 0x8000u, opcode, 4u}};
    size_t count = 1u;
    wz_master_tick_t duration = 8u;
    wz_byte_t stack_low = 0x34u;
    wz_byte_t stack_high = 0x12u;
    memset(capture, 0, sizeof(*capture));
    capture->input_value = input;
    capture->input_port = port;
    machine->cpu.program_counter = 0x8000u;
    machine->cpu.stack_pointer = 0x9000u;
    machine->cpu.main.a = a;
    machine->cpu.main.f = flags;
    machine->cpu.main.b = 0x90u; machine->cpu.main.c = 0x00u;
    machine->cpu.main.d = 0xa0u; machine->cpu.main.e = 0x00u;
    machine->cpu.main.h = 0xb0u; machine->cpu.main.l = 0x00u;
    machine->cpu.alternate.a = 0x9cu;
    machine->cpu.alternate.f = 0x46u;
    machine->cpu.alternate.b = 0x12u; machine->cpu.alternate.c = 0x34u;
    machine->cpu.alternate.d = 0x56u; machine->cpu.alternate.e = 0x78u;
    machine->cpu.alternate.h = 0x9au; machine->cpu.alternate.l = 0xbcu;
    machine->cpu.memptr = 0xbeefu;
    machine->cpu.r = 0u;
    machine->cpu.q = 0u;
    wz_machine_memory_write(machine, 0x8000u, opcode);
    wz_machine_memory_write(machine, 0x8001u, 0x35u);
    wz_machine_memory_write(machine, 0x9000u, stack_low);
    wz_machine_memory_write(machine, 0x9001u, stack_high);
    expected = machine->cpu;
    expected.program_counter = 0x8001u;
    expected.r = 1u;
    if (opcode == 0x08u) {
        expected.main.a = 0x9cu; expected.main.f = 0x46u;
        expected.alternate.a = a; expected.alternate.f = flags;
    } else if (opcode == 0xd9u) {
        expected.main.b = 0x12u; expected.main.c = 0x34u;
        expected.main.d = 0x56u; expected.main.e = 0x78u;
        expected.main.h = 0x9au; expected.main.l = 0xbcu;
        expected.alternate.b = 0x90u; expected.alternate.c = 0x00u;
        expected.alternate.d = 0xa0u; expected.alternate.e = 0x00u;
        expected.alternate.h = 0xb0u; expected.alternate.l = 0x00u;
    } else if (opcode == 0xebu) {
        expected.main.d = 0xb0u; expected.main.e = 0x00u;
        expected.main.h = 0xa0u; expected.main.l = 0x00u;
    } else if (opcode == 0xe3u) {
        expected.main.h = 0x12u; expected.main.l = 0x34u;
        expected.memptr = 0x1234u;
        stack_low = 0x00u; stack_high = 0xb0u;
        duration = 38u;
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
            base + 8u, 0x9000u, 0x34u, 3u};
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
            base + 14u, 0x9001u, 0x12u, 3u};
        bus[count++] = (expected_bus_t){WZ_BUS_INTERNAL,
            base + 20u, 0x9000u, 0x00u, 3u};
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_WRITE,
            base + 22u, 0x9001u, 0xb0u, 3u};
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_WRITE,
            base + 28u, 0x9000u, 0x00u, 3u};
    } else if (io) {
        expected.program_counter = 0x8002u;
        expected.memptr = (wz_word_t)(((wz_word_t)a << 8u) | 0x36u);
        duration = 22u;
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
            base + 8u, 0x8001u, 0x35u, 3u};
        bus[count++] = (expected_bus_t){opcode == 0xdbu ?
            WZ_BUS_IO_READ : WZ_BUS_IO_WRITE,
            base + 14u, port, opcode == 0xdbu ? input : a, 4u};
        if (opcode == 0xdbu) expected.main.a = input;
    }
    if (wz_z80_step(machine) != WZ_RESULT_OK ||
        machine->master_tick != base + duration ||
        memcmp(&machine->cpu, &expected, sizeof(expected)) != 0 ||
        wz_machine_memory_read(machine, 0x9000u) != stack_low ||
        wz_machine_memory_read(machine, 0x9001u) != stack_high ||
        !check_bus_trace(capture, bus, count, opcode)) {
        (void)fprintf(stderr,
            "primary exchange/IO mismatch opcode=%02x A=%02x input=%02x F=%02x "
            "PC=%04x/%04x memptr=%04x/%04x ticks=%llu/%llu bus=%zu/%zu trace=%zu\n",
            opcode, a, input, flags, machine->cpu.program_counter,
            expected.program_counter, machine->cpu.memptr, expected.memptr,
            (unsigned long long)(machine->master_tick - base),
            (unsigned long long)duration, capture->bus_count, count,
            capture->trace_count);
        return false;
    }
    return true;
}

int main(void)
{
    static const wz_byte_t exchanges[] = {0x08u, 0xd9u, 0xebu, 0xe3u};
    static const wz_byte_t a_values[] = {0x12u, 0x28u, 0xffu};
    static const wz_byte_t input_values[] = {0x00u, 0x80u, 0xffu};
    static const wz_byte_t flags[] = {0x00u, 0xd7u};
    wz_machine_t machine;
    wz_bus_observer_t observer;
    wz_bus_input_t input;
    wz_trace_sink_t sink;
    capture_t capture = {0};
    unsigned cases = 0u;
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        (void)fputs("machine init failed\n", stderr);
        return 1;
    }
    wz_bus_observer_init(&observer, record_bus, &capture);
    wz_bus_input_init(&input, read_port, &capture);
    if (wz_machine_set_bus_observer(&machine, &observer) != WZ_RESULT_OK ||
        wz_machine_set_bus_input(&machine, &input) != WZ_RESULT_OK) goto failed;
    wz_trace_sink_init(&sink, record_trace, &capture);
    wz_machine_set_timing_trace(&machine, &sink);
    for (size_t index = 0u; index < sizeof(exchanges); ++index) {
        if (!run_case(&machine, &capture, exchanges[index], 0x12u,
                      0x34u, 0xd7u)) goto failed;
        ++cases;
    }
    for (unsigned direction = 0u; direction < 2u; ++direction) {
        for (size_t value = 0u; value < sizeof(a_values); ++value) {
            for (size_t flag = 0u; flag < sizeof(flags); ++flag) {
                if (!run_case(&machine, &capture,
                              direction == 0u ? 0xd3u : 0xdbu,
                              a_values[value], input_values[value],
                              flags[flag])) goto failed;
                ++cases;
            }
        }
    }
    wz_machine_destroy(&machine);
    (void)printf("{\"testId\":\"cpu-primary-exchange-io\","
                 "\"status\":\"pass\",\"cases\":%u,"
                 "\"exchangeCases\":4,\"ioCases\":12}\n", cases);
    return 0;
failed:
    wz_machine_destroy(&machine);
    return 1;
}
