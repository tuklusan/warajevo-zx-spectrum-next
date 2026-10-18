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

static wz_byte_t* reg(wz_z80_register_bank_t* bank, unsigned index)
{
    switch (index) {
    case 0u: return &bank->b;
    case 1u: return &bank->c;
    case 2u: return &bank->d;
    case 3u: return &bank->e;
    case 4u: return &bank->h;
    case 5u: return &bank->l;
    case 7u: return &bank->a;
    default: return 0;
    }
}

static bool check_bus_trace(const capture_t* capture,
                            const expected_bus_t* expected,
                            size_t count, wz_byte_t prefix,
                            wz_byte_t opcode)
{
    size_t trace_bus = 0u;
    size_t trace_opcode = 0u;
    size_t trace_instruction = 0u;
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
                event->value != (trace_opcode == 0u ? prefix : opcode)) return false;
            ++trace_opcode;
        } else if (event->kind == WZ_TRACE_CPU_INSTRUCTION) {
            if (event->program_counter != 0x8000u || event->value != prefix) return false;
            ++trace_instruction;
        }
    }
    return trace_bus == count && trace_opcode == 2u && trace_instruction == 1u;
}

static bool run_case(wz_machine_t* machine, capture_t* capture,
                     wz_byte_t prefix, wz_byte_t opcode,
                     wz_word_t index_value, wz_byte_t displacement)
{
    const wz_master_tick_t base = machine->master_tick;
    const wz_word_t address = (wz_word_t)(index_value +
        (wz_word_t)(int16_t)(int8_t)displacement);
    const bool immediate = opcode == 0x36u;
    const bool load = !immediate && (opcode & 7u) == 6u;
    const unsigned reg_index = load ? (opcode >> 3u) & 7u : opcode & 7u;
    wz_z80_state_t expected;
    expected_bus_t bus[5] = {
        {WZ_BUS_M1_OPCODE_FETCH, base, 0x8000u, prefix, 4u},
        {WZ_BUS_M1_OPCODE_FETCH, base + 8u, 0x8001u, opcode, 4u},
        {WZ_BUS_MEMORY_READ, base + 16u, 0x8002u, displacement, 3u},
    };
    wz_byte_t expected_memory = 0x3cu;
    wz_byte_t value;
    memset(capture, 0, sizeof(*capture));
    machine->cpu.program_counter = 0x8000u;
    machine->cpu.main.a = 0xdeu;
    machine->cpu.main.f = 0xd7u;
    machine->cpu.main.b = 0x12u; machine->cpu.main.c = 0x34u;
    machine->cpu.main.d = 0x56u; machine->cpu.main.e = 0x78u;
    machine->cpu.main.h = 0x9au; machine->cpu.main.l = 0xbcu;
    machine->cpu.ix = prefix == 0xddu ? index_value : 0x4377u;
    machine->cpu.iy = prefix == 0xfdu ? index_value : 0x6588u;
    machine->cpu.memptr = 0xbeefu;
    machine->cpu.r = 0u;
    machine->cpu.q = 0u;
    wz_machine_memory_write(machine, 0x8000u, prefix);
    wz_machine_memory_write(machine, 0x8001u, opcode);
    wz_machine_memory_write(machine, 0x8002u, displacement);
    wz_machine_memory_write(machine, 0x8003u, 0xa5u);
    wz_machine_memory_write(machine, address, 0x3cu);
    expected = machine->cpu;
    expected.program_counter = immediate ? 0x8004u : 0x8003u;
    expected.r = 2u;
    expected.memptr = address;
    if (immediate) {
        expected_memory = 0xa5u;
        bus[3] = (expected_bus_t){WZ_BUS_MEMORY_READ,
            base + 22u, 0x8003u, 0xa5u, 3u};
    } else {
        bus[3] = (expected_bus_t){WZ_BUS_INTERNAL,
            base + 22u, address, 0u, 5u};
    }
    if (load) {
        *reg(&expected.main, reg_index) = 0x3cu;
        bus[4] = (expected_bus_t){WZ_BUS_MEMORY_READ,
            base + 32u, address, 0x3cu, 3u};
    } else {
        value = immediate ? 0xa5u : *reg(&expected.main, reg_index);
        expected_memory = value;
        bus[4] = (expected_bus_t){WZ_BUS_MEMORY_WRITE,
            base + 32u, address, value, 3u};
    }
    if (wz_z80_step(machine) != WZ_RESULT_OK ||
        machine->master_tick != base + 38u ||
        memcmp(&machine->cpu, &expected, sizeof(expected)) != 0 ||
        wz_machine_memory_read(machine, address) != expected_memory ||
        !check_bus_trace(capture, bus, 5u, prefix, opcode)) {
        (void)fprintf(stderr,
            "indexed load mismatch prefix=%02x opcode=%02x IX/IY=%04x d=%02x "
            "addr=%04x PC=%04x/%04x mem=%02x/%02x ticks=%llu bus=%zu trace=%zu\n",
            prefix, opcode, index_value, displacement, address,
            machine->cpu.program_counter, expected.program_counter,
            wz_machine_memory_read(machine, address), expected_memory,
            (unsigned long long)(machine->master_tick - base),
            capture->bus_count, capture->trace_count);
        return false;
    }
    return true;
}

int main(void)
{
    static const wz_byte_t opcodes[] = {
        0x46u, 0x4eu, 0x56u, 0x5eu, 0x66u, 0x6eu, 0x7eu,
        0x70u, 0x71u, 0x72u, 0x73u, 0x74u, 0x75u, 0x77u,
        0x36u,
    };
    static const wz_word_t indexes[] = {0xa800u, 0x0000u, 0x9080u};
    static const wz_byte_t displacements[] = {0x01u, 0xffu, 0x80u};
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
    if (wz_machine_set_bus_observer(&machine, &observer) != WZ_RESULT_OK) goto failed;
    wz_trace_sink_init(&sink, record_trace, &capture);
    wz_machine_set_timing_trace(&machine, &sink);
    for (unsigned prefix = 0u; prefix < 2u; ++prefix) {
        for (size_t opcode = 0u; opcode < sizeof(opcodes) / sizeof(opcodes[0]); ++opcode) {
            for (size_t location = 0u; location < 3u; ++location) {
                if (!run_case(&machine, &capture,
                              prefix == 0u ? 0xddu : 0xfdu,
                              opcodes[opcode], indexes[location],
                              displacements[location])) goto failed;
                ++cases;
            }
        }
    }
    wz_machine_destroy(&machine);
    (void)printf("{\"testId\":\"cpu-indexed-load\",\"status\":\"pass\","
                 "\"cases\":%u,\"registerReadCases\":42,"
                 "\"registerWriteCases\":42,\"immediateCases\":6,"
                 "\"wraparoundCases\":30}\n", cases);
    return 0;
failed:
    wz_machine_destroy(&machine);
    return 1;
}
