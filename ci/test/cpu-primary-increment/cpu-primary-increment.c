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

enum { C = 1, N = 2, PV = 4, X = 8, H = 16, Y = 32, Z = 64, S = 128 };

typedef struct {
    wz_bus_request_t bus[4];
    wz_trace_event_t trace[16];
    size_t bus_count, trace_count;
    bool overflow;
} capture_t;

static void on_bus(const wz_bus_request_t* event, void* context)
{
    capture_t* capture = (capture_t*)context;
    if (capture->bus_count == 4u) capture->overflow = true;
    else capture->bus[capture->bus_count++] = *event;
}

static void on_trace(const wz_trace_event_t* event, void* context)
{
    capture_t* capture = (capture_t*)context;
    if (capture->trace_count == 16u) capture->overflow = true;
    else capture->trace[capture->trace_count++] = *event;
}

static bool check_capture(const capture_t* capture, wz_master_tick_t base,
                          wz_byte_t opcode)
{
    size_t buses = 0u, opcodes = 0u, instructions = 0u;
    if (capture->overflow || capture->bus_count != 1u) return false;
    if (capture->bus[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        capture->bus[0].master_tick != base ||
        capture->bus[0].address != 0x8000u ||
        capture->bus[0].value != opcode ||
        capture->bus[0].t_states != 4u ||
        capture->bus[0].contention_delay != 0u) return false;
    for (size_t index = 0u; index < capture->trace_count; ++index) {
        const wz_trace_event_t* event = &capture->trace[index];
        if (index != 0u && event->sequence <= capture->trace[index - 1u].sequence) return false;
        if (event->kind == WZ_TRACE_CPU_BUS) {
            if (buses != 0u || event->master_tick != base ||
                event->address != 0x8000u || event->value != opcode ||
                event->cycle != (wz_byte_t)WZ_BUS_M1_OPCODE_FETCH ||
                event->t_states != 4u) return false;
            ++buses;
        } else if (event->kind == WZ_TRACE_CPU_OPCODE_BYTE) {
            if (opcodes != 0u || event->address != 0x8000u ||
                event->value != opcode) return false;
            ++opcodes;
        } else if (event->kind == WZ_TRACE_CPU_INSTRUCTION) {
            if (event->program_counter != 0x8000u || event->value != opcode) return false;
            ++instructions;
        }
    }
    return buses == 1u && opcodes == 1u && instructions == 1u;
}

static wz_byte_t* register8(wz_z80_state_t* state, unsigned target)
{
    switch (target) {
    case 0u: return &state->main.b;
    case 1u: return &state->main.c;
    case 2u: return &state->main.d;
    case 3u: return &state->main.e;
    case 4u: return &state->main.h;
    case 5u: return &state->main.l;
    case 7u: return &state->main.a;
    default: return 0;
    }
}

static void set_pair(wz_z80_state_t* state, unsigned pair, wz_word_t value)
{
    switch (pair) {
    case 0u: state->main.b = (wz_byte_t)(value >> 8u);
             state->main.c = (wz_byte_t)value; break;
    case 1u: state->main.d = (wz_byte_t)(value >> 8u);
             state->main.e = (wz_byte_t)value; break;
    case 2u: state->main.h = (wz_byte_t)(value >> 8u);
             state->main.l = (wz_byte_t)value; break;
    default: state->stack_pointer = value; break;
    }
}

static unsigned inc_dec_flags(unsigned original, unsigned value,
                              bool decrement)
{
    const unsigned result = decrement ? (value - 1u) & 255u :
                                        (value + 1u) & 255u;
    unsigned flags = (original & C) | (result & (S | Y | X));
    if (result == 0u) flags |= Z;
    if (decrement) {
        flags |= N;
        if ((value & 15u) == 0u) flags |= H;
        if (value == 128u) flags |= PV;
    } else {
        if ((value & 15u) == 15u) flags |= H;
        if (value == 127u) flags |= PV;
    }
    return flags;
}

static bool run_8bit(wz_machine_t* machine, capture_t* capture,
                     unsigned target, unsigned value, unsigned carry,
                     bool decrement)
{
    const wz_byte_t opcode = (wz_byte_t)((target << 3u) +
                                         (decrement ? 5u : 4u));
    const wz_master_tick_t base = machine->master_tick;
    wz_z80_state_t expected;
    memset(capture, 0, sizeof(*capture));
    machine->cpu.program_counter = 0x8000u;
    machine->cpu.main.f = (wz_byte_t)(0xfeu | carry);
    machine->cpu.main.a = 0x81u;
    machine->cpu.main.b = 0x12u; machine->cpu.main.c = 0x34u;
    machine->cpu.main.d = 0x56u; machine->cpu.main.e = 0x78u;
    machine->cpu.main.h = 0x90u; machine->cpu.main.l = 0x00u;
    machine->cpu.r = 0u;
    machine->cpu.q = 0u;
    machine->cpu.memptr = 0xbeefu;
    *register8(&machine->cpu, target) = (wz_byte_t)value;
    wz_machine_memory_write(machine, 0x8000u, opcode);
    expected = machine->cpu;
    *register8(&expected, target) = (wz_byte_t)(decrement ? value - 1u : value + 1u);
    expected.main.f = (wz_byte_t)inc_dec_flags(machine->cpu.main.f, value, decrement);
    expected.program_counter = 0x8001u;
    expected.r = 1u;
    expected.q = 1u;
    if (wz_z80_step(machine) != WZ_RESULT_OK ||
        machine->master_tick != base + 8u ||
        memcmp(&machine->cpu, &expected, sizeof(expected)) != 0 ||
        !check_capture(capture, base, opcode)) {
        (void)fprintf(stderr,
            "primary 8-bit increment mismatch target=%u value=%02x carry=%u dec=%u "
            "F=%02x/%02x PC=%04x/%04x bus=%zu trace=%zu\n",
            target, value, carry, decrement,
            machine->cpu.main.f, expected.main.f,
            machine->cpu.program_counter, expected.program_counter,
            capture->bus_count, capture->trace_count);
        return false;
    }
    return true;
}

static bool run_16bit(wz_machine_t* machine, capture_t* capture,
                      unsigned pair, unsigned value, bool decrement)
{
    const wz_byte_t opcode = (wz_byte_t)((pair << 4u) +
                                         (decrement ? 0x0bu : 0x03u));
    const wz_master_tick_t base = machine->master_tick;
    wz_z80_state_t expected;
    memset(capture, 0, sizeof(*capture));
    machine->cpu.program_counter = 0x8000u;
    machine->cpu.main.f = 0xa5u;
    machine->cpu.main.b = 0x12u; machine->cpu.main.c = 0x34u;
    machine->cpu.main.d = 0x56u; machine->cpu.main.e = 0x78u;
    machine->cpu.main.h = 0x90u; machine->cpu.main.l = 0x00u;
    machine->cpu.stack_pointer = 0x9000u;
    machine->cpu.r = 0u;
    machine->cpu.q = 0u;
    machine->cpu.memptr = 0xbeefu;
    set_pair(&machine->cpu, pair, (wz_word_t)value);
    wz_machine_memory_write(machine, 0x8000u, opcode);
    expected = machine->cpu;
    set_pair(&expected, pair,
             (wz_word_t)(decrement ? value - 1u : value + 1u));
    expected.program_counter = 0x8001u;
    expected.r = 1u;
    if (wz_z80_step(machine) != WZ_RESULT_OK ||
        machine->master_tick != base + 12u ||
        memcmp(&machine->cpu, &expected, sizeof(expected)) != 0 ||
        !check_capture(capture, base, opcode)) {
        (void)fprintf(stderr,
            "primary 16-bit increment mismatch pair=%u value=%04x dec=%u "
            "F=%02x/%02x PC=%04x/%04x bus=%zu trace=%zu\n",
            pair, value, decrement,
            machine->cpu.main.f, expected.main.f,
            machine->cpu.program_counter, expected.program_counter,
            capture->bus_count, capture->trace_count);
        return false;
    }
    return true;
}

int main(void)
{
    static const unsigned values8[] = {0x00u, 0x0fu, 0x7fu, 0x80u, 0xffu};
    static const unsigned values16[] = {0x0000u, 0x7fffu, 0xffffu};
    wz_machine_t machine;
    wz_bus_observer_t observer;
    wz_trace_sink_t sink;
    capture_t capture = {0};
    unsigned cases8 = 0u, cases16 = 0u;
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) return 1;
    wz_bus_observer_init(&observer, on_bus, &capture);
    if (wz_machine_set_bus_observer(&machine, &observer) != WZ_RESULT_OK) goto failed;
    wz_trace_sink_init(&sink, on_trace, &capture);
    wz_machine_set_timing_trace(&machine, &sink);
    for (unsigned target = 0u; target < 8u; ++target) {
        if (target == 6u) continue;
        for (size_t value = 0u; value < 5u; ++value) {
            for (unsigned carry = 0u; carry < 2u; ++carry) {
                for (unsigned dec = 0u; dec < 2u; ++dec) {
                    if (!run_8bit(&machine, &capture, target, values8[value],
                                  carry, dec != 0u)) goto failed;
                    ++cases8;
                }
            }
        }
    }
    for (unsigned pair = 0u; pair < 4u; ++pair) {
        for (size_t value = 0u; value < 3u; ++value) {
            for (unsigned dec = 0u; dec < 2u; ++dec) {
                if (!run_16bit(&machine, &capture, pair, values16[value],
                               dec != 0u)) goto failed;
                ++cases16;
            }
        }
    }
    wz_machine_destroy(&machine);
    (void)printf("{\"testId\":\"cpu-primary-increment\",\"status\":\"pass\","
                 "\"cases\":%u,\"byteCases\":%u,\"wordCases\":%u}\n",
                 cases8 + cases16, cases8, cases16);
    return 0;
failed:
    wz_machine_destroy(&machine);
    return 1;
}
