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

static wz_word_t pair_value(const wz_machine_t* machine, unsigned pair)
{
    switch (pair) {
    case 0u: return (wz_word_t)(((wz_word_t)machine->cpu.main.b << 8u) |
                                 machine->cpu.main.c);
    case 1u: return (wz_word_t)(((wz_word_t)machine->cpu.main.d << 8u) |
                                 machine->cpu.main.e);
    case 2u: return (wz_word_t)(((wz_word_t)machine->cpu.main.h << 8u) |
                                 machine->cpu.main.l);
    default: return machine->cpu.stack_pointer;
    }
}

static bool check_bus_trace(const capture_t* capture,
                            const expected_bus_t* expected,
                            size_t count, wz_byte_t opcode)
{
    size_t trace_bus = 0u;
    size_t trace_opcode = 0u;
    size_t trace_instruction = 0u;
    if (capture->overflow || capture->bus_count != count) return false;
    for (size_t index = 0u; index < count; ++index) {
        const wz_bus_request_t* bus = &capture->bus[index];
        if (bus->cycle != expected[index].cycle ||
            bus->master_tick != expected[index].tick ||
            bus->address != expected[index].address ||
            bus->value != expected[index].value ||
            bus->t_states != expected[index].duration ||
            bus->contention_delay != 0u) return false;
    }
    for (size_t index = 0u; index < capture->trace_count; ++index) {
        const wz_trace_event_t* event = &capture->trace[index];
        if (index != 0u && event->sequence <= capture->trace[index - 1u].sequence) {
            return false;
        }
        if (event->kind == WZ_TRACE_CPU_BUS) {
            if (trace_bus >= count ||
                event->master_tick != expected[trace_bus].tick ||
                event->address != expected[trace_bus].address ||
                event->value != expected[trace_bus].value ||
                event->cycle != (wz_byte_t)expected[trace_bus].cycle ||
                event->t_states != expected[trace_bus].duration) return false;
            ++trace_bus;
        } else if (event->kind == WZ_TRACE_CPU_OPCODE_BYTE) {
            if (trace_opcode >= 2u ||
                event->address != (wz_word_t)(0x8000u + trace_opcode) ||
                event->value != (trace_opcode == 0u ? 0xedu : opcode)) return false;
            ++trace_opcode;
        } else if (event->kind == WZ_TRACE_CPU_INSTRUCTION) {
            if (event->program_counter != 0x8000u || event->value != 0xedu) return false;
            ++trace_instruction;
        }
    }
    return trace_bus == count && trace_opcode == 2u && trace_instruction == 1u;
}

static bool run_case(wz_machine_t* machine, capture_t* capture,
                     bool load, unsigned pair, wz_word_t address,
                     wz_byte_t flags)
{
    const wz_byte_t opcode = (wz_byte_t)((load ? 0x4bu : 0x43u) | (pair << 4u));
    const wz_master_tick_t base = machine->master_tick;
    const wz_byte_t low_address = (wz_byte_t)address;
    const wz_byte_t high_address = (wz_byte_t)(address >> 8u);
    const wz_word_t next_address = (wz_word_t)(address + 1u);
    const wz_word_t initial_pairs[4] = {0x1234u, 0x5678u, 0x9abcu, 0xdef0u};
    const wz_word_t expected_pair = load ? 0xbeefu : initial_pairs[pair];
    const wz_byte_t expected_low = (wz_byte_t)expected_pair;
    const wz_byte_t expected_high = (wz_byte_t)(expected_pair >> 8u);
    expected_bus_t expected[6] = {
        {WZ_BUS_M1_OPCODE_FETCH, base, 0x8000u, 0xedu, 4u},
        {WZ_BUS_M1_OPCODE_FETCH, base + 8u, 0x8001u, opcode, 4u},
        {WZ_BUS_MEMORY_READ, base + 16u, 0x8002u, low_address, 3u},
        {WZ_BUS_MEMORY_READ, base + 22u, 0x8003u, high_address, 3u},
        {load ? WZ_BUS_MEMORY_READ : WZ_BUS_MEMORY_WRITE,
         base + 28u, address, expected_low, 3u},
        {load ? WZ_BUS_MEMORY_READ : WZ_BUS_MEMORY_WRITE,
         base + 34u, next_address, expected_high, 3u},
    };
    memset(capture, 0, sizeof(*capture));
    machine->cpu.program_counter = 0x8000u;
    machine->cpu.main.a = 0xa5u;
    machine->cpu.main.f = flags;
    machine->cpu.main.b = 0x12u; machine->cpu.main.c = 0x34u;
    machine->cpu.main.d = 0x56u; machine->cpu.main.e = 0x78u;
    machine->cpu.main.h = 0x9au; machine->cpu.main.l = 0xbcu;
    machine->cpu.stack_pointer = 0xdef0u;
    machine->cpu.memptr = 0xbeefu;
    machine->cpu.r = 0u;
    machine->cpu.q = 0u;
    wz_machine_memory_write(machine, 0x8000u, 0xedu);
    wz_machine_memory_write(machine, 0x8001u, opcode);
    wz_machine_memory_write(machine, 0x8002u, low_address);
    wz_machine_memory_write(machine, 0x8003u, high_address);
    wz_machine_memory_write(machine, address, load ? 0xefu : 0xaau);
    wz_machine_memory_write(machine, next_address, load ? 0xbeu : 0xbbu);
    if (wz_z80_step(machine) != WZ_RESULT_OK ||
        machine->cpu.program_counter != 0x8004u ||
        machine->master_tick != base + 40u ||
        machine->cpu.r != 2u || machine->cpu.q != 0u ||
        machine->cpu.memptr != next_address ||
        machine->cpu.main.f != flags ||
        machine->cpu.main.a != 0xa5u ||
        pair_value(machine, pair) != expected_pair ||
        wz_machine_memory_read(machine, address) !=
            (load ? 0xefu : expected_low) ||
        wz_machine_memory_read(machine, next_address) !=
            (load ? 0xbeu : expected_high) ||
        !check_bus_trace(capture, expected, 6u, opcode)) {
        (void)fprintf(stderr,
            "ED transfer mismatch load=%u pair=%u address=%04x flags=%02x "
            "pair=%04x/%04x PC=%04x ticks=%llu bus=%zu trace=%zu\n",
            load, pair, address, flags, pair_value(machine, pair), expected_pair,
            machine->cpu.program_counter,
            (unsigned long long)(machine->master_tick - base),
            capture->bus_count, capture->trace_count);
        return false;
    }
    return true;
}

int main(void)
{
    static const wz_word_t addresses[2] = {0x9000u, 0xfffeu};
    static const wz_byte_t flags[2] = {0x00u, 0xd7u};
    wz_machine_t machine;
    wz_bus_observer_t observer;
    wz_trace_sink_t sink;
    capture_t capture = {0};
    unsigned cases = 0u;
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        (void)fputs("machine init failed\n", stderr);
        return 1;
    }
    wz_bus_observer_init(&observer, record_bus, &capture);
    if (wz_machine_set_bus_observer(&machine, &observer) != WZ_RESULT_OK) {
        wz_machine_destroy(&machine);
        return 1;
    }
    wz_trace_sink_init(&sink, record_trace, &capture);
    wz_machine_set_timing_trace(&machine, &sink);
    for (unsigned load = 0u; load < 2u; ++load) {
        for (unsigned pair = 0u; pair < 4u; ++pair) {
            for (unsigned position = 0u; position < 2u; ++position) {
                for (unsigned flag = 0u; flag < 2u; ++flag) {
                    if (!run_case(&machine, &capture, load != 0u, pair,
                                  addresses[position], flags[flag])) goto failed;
                    ++cases;
                }
            }
        }
    }
    wz_machine_destroy(&machine);
    (void)printf("{\"testId\":\"cpu-ed-transfer\",\"status\":\"pass\","
                 "\"cases\":%u,\"loadCases\":16,\"storeCases\":16}\n", cases);
    return 0;
failed:
    wz_machine_destroy(&machine);
    return 1;
}
