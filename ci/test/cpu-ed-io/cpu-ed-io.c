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

enum { FLAG_C = 1, FLAG_PV = 4, FLAG_X = 8, FLAG_Y = 32,
       FLAG_Z = 64, FLAG_S = 128, MAX_EVENTS = 20 };

typedef struct {
    wz_bus_request_t bus[MAX_EVENTS];
    wz_trace_event_t trace[MAX_EVENTS];
    size_t bus_count;
    size_t trace_count;
    wz_byte_t input_value;
    bool overflow;
} capture_t;

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
    return cycle == WZ_BUS_IO_READ && address == 0x1235u ?
        capture->input_value : 0xffu;
}

static bool even_parity(unsigned value)
{
    unsigned bits = 0u;
    for (unsigned index = 0u; index < 8u; ++index) bits += (value >> index) & 1u;
    return (bits & 1u) == 0u;
}

static wz_byte_t* target_register(wz_z80_register_bank_t* bank, unsigned target)
{
    switch (target) {
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

static bool check_bus_trace(const capture_t* capture, wz_master_tick_t base,
                            wz_byte_t opcode, wz_byte_t port_value, bool input)
{
    const wz_bus_cycle_t expected_cycle = input ? WZ_BUS_IO_READ : WZ_BUS_IO_WRITE;
    size_t bus_trace = 0u;
    size_t opcode_bytes = 0u;
    size_t instructions = 0u;
    if (capture->overflow || capture->bus_count != 3u) return false;
    if (capture->bus[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        capture->bus[0].master_tick != base ||
        capture->bus[0].address != 0x8000u || capture->bus[0].value != 0xedu ||
        capture->bus[0].t_states != 4u ||
        capture->bus[1].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        capture->bus[1].master_tick != base + 8u ||
        capture->bus[1].address != 0x8001u || capture->bus[1].value != opcode ||
        capture->bus[1].t_states != 4u ||
        capture->bus[2].cycle != expected_cycle ||
        capture->bus[2].master_tick != base + 16u ||
        capture->bus[2].address != 0x1235u ||
        capture->bus[2].value != port_value ||
        capture->bus[2].t_states != 4u) return false;
    for (size_t index = 0u; index < 3u; ++index) {
        if (capture->bus[index].contention_delay != 0u) return false;
    }
    for (size_t index = 0u; index < capture->trace_count; ++index) {
        const wz_trace_event_t* event = &capture->trace[index];
        if (index != 0u && event->sequence <= capture->trace[index - 1u].sequence) {
            return false;
        }
        if (event->kind == WZ_TRACE_CPU_BUS) {
            if (bus_trace >= 3u ||
                event->master_tick != capture->bus[bus_trace].master_tick ||
                event->address != capture->bus[bus_trace].address ||
                event->value != capture->bus[bus_trace].value ||
                event->cycle != (wz_byte_t)capture->bus[bus_trace].cycle ||
                event->t_states != capture->bus[bus_trace].t_states) return false;
            ++bus_trace;
        } else if (event->kind == WZ_TRACE_CPU_OPCODE_BYTE) {
            if (opcode_bytes >= 2u ||
                event->address != (wz_word_t)(0x8000u + opcode_bytes) ||
                event->value != (opcode_bytes == 0u ? 0xedu : opcode)) return false;
            ++opcode_bytes;
        } else if (event->kind == WZ_TRACE_CPU_INSTRUCTION) {
            if (event->program_counter != 0x8000u || event->value != 0xedu) return false;
            ++instructions;
        }
    }
    return bus_trace == 3u && opcode_bytes == 2u && instructions == 1u;
}

static bool run_case(wz_machine_t* machine, capture_t* capture,
                     bool input, unsigned target, unsigned value, unsigned carry)
{
    const wz_byte_t opcode = (wz_byte_t)((input ? 0x40u : 0x41u) |
                                        (target << 3u));
    const wz_byte_t old_flags = (wz_byte_t)(0xc4u | carry);
    const wz_master_tick_t base = machine->master_tick;
    wz_z80_register_bank_t expected;
    wz_byte_t port_value;
    memset(capture, 0, sizeof(*capture));
    capture->input_value = (wz_byte_t)value;
    machine->cpu.program_counter = 0x8000u;
    machine->cpu.main.a = 0xdeu;
    machine->cpu.main.b = 0x12u;
    machine->cpu.main.c = 0x35u;
    machine->cpu.main.d = 0x56u;
    machine->cpu.main.e = 0x78u;
    machine->cpu.main.h = 0x9au;
    machine->cpu.main.l = 0xbcu;
    machine->cpu.main.f = old_flags;
    machine->cpu.memptr = 0xbeefu;
    machine->cpu.r = 0u;
    machine->cpu.q = 0u;
    wz_machine_memory_write(machine, 0x8000u, 0xedu);
    wz_machine_memory_write(machine, 0x8001u, opcode);
    expected = machine->cpu.main;
    if (input) {
        expected.f = (wz_byte_t)(carry | (value & (FLAG_S | FLAG_X | FLAG_Y)));
        if (value == 0u) expected.f |= FLAG_Z;
        if (even_parity(value)) expected.f |= FLAG_PV;
        if (target != 6u) *target_register(&expected, target) = (wz_byte_t)value;
        port_value = (wz_byte_t)value;
    } else {
        port_value = target == 6u ? 0u : *target_register(&expected, target);
    }
    if (wz_z80_step(machine) != WZ_RESULT_OK ||
        machine->cpu.program_counter != 0x8002u ||
        machine->master_tick != base + 24u ||
        machine->cpu.r != 2u ||
        machine->cpu.q != (input ? 1u : 0u) ||
        machine->cpu.memptr != 0x1236u ||
        machine->cpu.main.a != expected.a ||
        machine->cpu.main.b != expected.b ||
        machine->cpu.main.c != expected.c ||
        machine->cpu.main.d != expected.d ||
        machine->cpu.main.e != expected.e ||
        machine->cpu.main.h != expected.h ||
        machine->cpu.main.l != expected.l ||
        machine->cpu.main.f != expected.f ||
        !check_bus_trace(capture, base, opcode, port_value, input)) {
        (void)fprintf(stderr,
            "ED IO mismatch input=%u target=%u value=%02x carry=%u "
            "A=%02x/%02x F=%02x/%02x PC=%04x ticks=%llu bus=%zu trace=%zu\n",
            input, target, value, carry, machine->cpu.main.a, expected.a,
            machine->cpu.main.f, expected.f, machine->cpu.program_counter,
            (unsigned long long)(machine->master_tick - base),
            capture->bus_count, capture->trace_count);
        return false;
    }
    return true;
}

int main(void)
{
    static const unsigned input_values[] = {0x00u, 0x01u, 0x28u, 0x80u, 0xffu};
    wz_machine_t machine;
    wz_bus_observer_t observer;
    wz_bus_input_t bus_input;
    wz_trace_sink_t sink;
    capture_t capture = {0};
    unsigned cases = 0u;
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        (void)fputs("machine init failed\n", stderr);
        return 1;
    }
    wz_bus_observer_init(&observer, record_bus, &capture);
    wz_bus_input_init(&bus_input, read_port, &capture);
    if (wz_machine_set_bus_observer(&machine, &observer) != WZ_RESULT_OK ||
        wz_machine_set_bus_input(&machine, &bus_input) != WZ_RESULT_OK) {
        wz_machine_destroy(&machine);
        return 1;
    }
    wz_trace_sink_init(&sink, record_trace, &capture);
    wz_machine_set_timing_trace(&machine, &sink);
    for (unsigned target = 0u; target < 8u; ++target) {
        for (unsigned carry = 0u; carry < 2u; ++carry) {
            for (unsigned index = 0u; index < sizeof(input_values) / sizeof(input_values[0]); ++index) {
                if (!run_case(&machine, &capture, true, target,
                              input_values[index], carry)) goto failed;
                ++cases;
            }
            if (!run_case(&machine, &capture, false, target, 0u, carry)) goto failed;
            ++cases;
        }
    }
    wz_machine_destroy(&machine);
    (void)printf("{\"testId\":\"cpu-ed-io\",\"status\":\"pass\","
                 "\"cases\":%u,\"inCases\":80,\"outCases\":16}\n", cases);
    return 0;
failed:
    wz_machine_destroy(&machine);
    return 1;
}
