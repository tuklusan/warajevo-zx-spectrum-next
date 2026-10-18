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

enum { FLAG_C = 1, FLAG_N = 2, FLAG_PV = 4, FLAG_X = 8,
       FLAG_H = 16, FLAG_Y = 32, FLAG_Z = 64, FLAG_S = 128,
       MAX_EVENTS = 24 };

typedef enum { NEG, IM, LD_I_A, LD_R_A, LD_A_I, LD_A_R,
               RRD, RLD, RETN_RETI } kind_t;

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

static bool even_parity(unsigned value)
{
    unsigned count = 0u;
    for (unsigned index = 0u; index < 8u; ++index) count += (value >> index) & 1u;
    return (count & 1u) == 0u;
}

static wz_byte_t sz53p(unsigned value)
{
    wz_byte_t flags = (wz_byte_t)(value & (FLAG_S | FLAG_Y | FLAG_X));
    if (value == 0u) flags |= FLAG_Z;
    if (even_parity(value)) flags |= FLAG_PV;
    return flags;
}

static wz_byte_t neg_flags(unsigned original, unsigned result)
{
    wz_byte_t flags = (wz_byte_t)(FLAG_N |
        (result & (FLAG_S | FLAG_Y | FLAG_X)));
    if (result == 0u) flags |= FLAG_Z;
    if ((original & 15u) != 0u) flags |= FLAG_H;
    if (original == 128u) flags |= FLAG_PV;
    if (original != 0u) flags |= FLAG_C;
    return flags;
}

static wz_byte_t ld_ir_flags(unsigned value, unsigned carry, unsigned iff2)
{
    wz_byte_t flags = (wz_byte_t)(carry | (value & (FLAG_S | FLAG_Y | FLAG_X)));
    if (value == 0u) flags |= FLAG_Z;
    if (iff2 != 0u) flags |= FLAG_PV;
    return flags;
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
                     kind_t kind, wz_byte_t opcode,
                     wz_byte_t accumulator, wz_byte_t memory_value,
                     wz_byte_t iff2)
{
    const wz_master_tick_t base = machine->master_tick;
    wz_z80_state_t expected;
    expected_bus_t bus[4] = {
        {WZ_BUS_M1_OPCODE_FETCH, base, 0x8000u, 0xedu, 4u},
        {WZ_BUS_M1_OPCODE_FETCH, base + 8u, 0x8001u, opcode, 4u},
    };
    size_t bus_count = 2u;
    wz_byte_t expected_memory = memory_value;
    wz_master_tick_t duration = 16u;
    memset(capture, 0, sizeof(*capture));
    machine->cpu.program_counter = 0x8000u;
    machine->cpu.stack_pointer = 0x9000u;
    machine->cpu.main.a = accumulator;
    machine->cpu.main.f = 0xd7u;
    machine->cpu.main.b = 0x12u; machine->cpu.main.c = 0x34u;
    machine->cpu.main.d = 0x56u; machine->cpu.main.e = 0x78u;
    machine->cpu.main.h = 0x90u; machine->cpu.main.l = 0x00u;
    machine->cpu.i = 0x80u;
    machine->cpu.r = 0xfeu;
    machine->cpu.iff1 = 1u;
    machine->cpu.iff2 = iff2;
    machine->cpu.interrupt_mode = 1u;
    machine->cpu.memptr = 0xbeefu;
    machine->cpu.q = 0u;
    wz_machine_memory_write(machine, 0x8000u, 0xedu);
    wz_machine_memory_write(machine, 0x8001u, opcode);
    wz_machine_memory_write(machine, 0x9000u, memory_value);
    wz_machine_memory_write(machine, 0x9001u, 0x12u);
    expected = machine->cpu;
    expected.program_counter = 0x8002u;
    expected.r = 0x80u;
    switch (kind) {
    case NEG:
        expected.main.a = (wz_byte_t)(0u - accumulator);
        expected.main.f = neg_flags(accumulator, expected.main.a);
        expected.q = 1u;
        break;
    case IM:
        expected.interrupt_mode = opcode == 0x56u || opcode == 0x76u ? 1u :
                                  opcode == 0x5eu || opcode == 0x7eu ? 2u : 0u;
        break;
    case LD_I_A:
        expected.i = accumulator;
        duration = 18u;
        break;
    case LD_R_A:
        expected.r = accumulator;
        duration = 18u;
        break;
    case LD_A_I:
        expected.main.a = 0x80u;
        expected.main.f = ld_ir_flags(expected.main.a, FLAG_C, iff2);
        expected.q = 1u;
        duration = 18u;
        break;
    case LD_A_R:
        expected.main.a = 0x80u;
        expected.main.f = ld_ir_flags(expected.main.a, FLAG_C, iff2);
        expected.q = 1u;
        duration = 18u;
        break;
    case RRD:
    case RLD:
        expected.main.a = (wz_byte_t)((accumulator & 0xf0u) |
            (kind == RRD ? memory_value & 0x0fu : memory_value >> 4u));
        expected_memory = (wz_byte_t)(kind == RRD ?
            ((accumulator & 0x0fu) << 4u) | (memory_value >> 4u) :
            (memory_value << 4u) | (accumulator & 0x0fu));
        expected.main.f = (wz_byte_t)(FLAG_C | sz53p(expected.main.a));
        expected.memptr = 0x9001u;
        expected.q = 1u;
        duration = 36u;
        bus[bus_count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
            base + 16u, 0x9000u, memory_value, 3u};
        bus[bus_count++] = (expected_bus_t){WZ_BUS_MEMORY_WRITE,
            base + 30u, 0x9000u, expected_memory, 3u};
        break;
    case RETN_RETI:
        expected.program_counter = 0x1200u | memory_value;
        expected.stack_pointer = 0x9002u;
        expected.memptr = expected.program_counter;
        expected.iff1 = iff2;
        duration = 28u;
        bus[bus_count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
            base + 16u, 0x9000u, memory_value, 3u};
        bus[bus_count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
            base + 22u, 0x9001u, 0x12u, 3u};
        break;
    }
    if (wz_z80_step(machine) != WZ_RESULT_OK ||
        machine->master_tick != base + duration ||
        memcmp(&machine->cpu, &expected, sizeof(expected)) != 0 ||
        wz_machine_memory_read(machine, 0x9000u) != expected_memory ||
        !check_bus_trace(capture, bus, bus_count, opcode)) {
        (void)fprintf(stderr,
            "ED control mismatch kind=%u opcode=%02x A=%02x mem=%02x iff2=%u "
            "PC=%04x/%04x F=%02x/%02x R=%02x/%02x ticks=%llu/%llu "
            "bus=%zu/%zu trace=%zu\n",
            (unsigned)kind, opcode, accumulator, memory_value, iff2,
            machine->cpu.program_counter, expected.program_counter,
            machine->cpu.main.f, expected.main.f,
            machine->cpu.r, expected.r,
            (unsigned long long)(machine->master_tick - base),
            (unsigned long long)duration, capture->bus_count, bus_count,
            capture->trace_count);
        return false;
    }
    return true;
}

int main(void)
{
    static const wz_byte_t neg_opcodes[] =
        {0x44u, 0x4cu, 0x54u, 0x5cu, 0x64u, 0x6cu, 0x74u, 0x7cu};
    static const wz_byte_t edge_values[] =
        {0x00u, 0x01u, 0x0fu, 0x10u, 0x7fu, 0x80u, 0xffu};
    static const wz_byte_t im_opcodes[] =
        {0x46u, 0x4eu, 0x66u, 0x6eu, 0x56u, 0x76u, 0x5eu, 0x7eu};
    static const wz_byte_t return_opcodes[] =
        {0x45u, 0x55u, 0x5du, 0x65u, 0x6du, 0x75u, 0x7du, 0x4du};
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
    for (size_t op = 0u; op < sizeof(neg_opcodes); ++op) {
        for (size_t value = 0u; value < sizeof(edge_values); ++value) {
            if (!run_case(&machine, &capture, NEG, neg_opcodes[op],
                          edge_values[value], 0x34u, 0u)) goto failed;
            ++cases;
        }
    }
    for (size_t op = 0u; op < sizeof(im_opcodes); ++op) {
        if (!run_case(&machine, &capture, IM, im_opcodes[op],
                      0x7du, 0x34u, 1u)) goto failed;
        ++cases;
    }
    for (unsigned iff2 = 0u; iff2 < 2u; ++iff2) {
        const wz_byte_t opcodes[] = {0x47u, 0x4fu, 0x57u, 0x5fu};
        const kind_t kinds[] = {LD_I_A, LD_R_A, LD_A_I, LD_A_R};
        for (size_t index = 0u; index < 4u; ++index) {
            if (!run_case(&machine, &capture, kinds[index], opcodes[index],
                          0x7du, 0x34u, (wz_byte_t)iff2)) goto failed;
            ++cases;
        }
        for (size_t op = 0u; op < sizeof(return_opcodes); ++op) {
            if (!run_case(&machine, &capture, RETN_RETI, return_opcodes[op],
                          0x7du, 0x34u, (wz_byte_t)iff2)) goto failed;
            ++cases;
        }
    }
    for (unsigned value = 0u; value < 3u; ++value) {
        const wz_byte_t a = (wz_byte_t)(value == 0u ? 0x7du : value == 1u ? 0x80u : 0x00u);
        const wz_byte_t memory = (wz_byte_t)(value == 0u ? 0x46u : value == 1u ? 0x01u : 0xffu);
        if (!run_case(&machine, &capture, RRD, 0x67u, a, memory, 0u) ||
            !run_case(&machine, &capture, RLD, 0x6fu, a, memory, 1u)) goto failed;
        cases += 2u;
    }
    wz_machine_destroy(&machine);
    (void)printf("{\"testId\":\"cpu-ed-control\",\"status\":\"pass\","
                 "\"cases\":%u,\"negCases\":56,\"imCases\":8,"
                 "\"irCases\":8,\"returnCases\":16,\"nibbleCases\":6}\n", cases);
    return 0;
failed:
    wz_machine_destroy(&machine);
    return 1;
}
