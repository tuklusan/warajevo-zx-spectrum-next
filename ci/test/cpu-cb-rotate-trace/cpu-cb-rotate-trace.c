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

enum { C = 1, PV = 4, X = 8, Y = 32, Z = 64, S = 128, MAX_EVENTS = 20 };

typedef struct {
    wz_bus_request_t bus[MAX_EVENTS];
    wz_trace_event_t trace[MAX_EVENTS];
    size_t bus_count, trace_count;
    bool overflow;
} capture_t;

typedef struct {
    wz_bus_cycle_t cycle;
    wz_master_tick_t tick;
    wz_word_t address;
    wz_byte_t value, duration;
} expected_bus_t;

typedef struct { wz_byte_t value, flags; } answer_t;

static void on_bus(const wz_bus_request_t* event, void* context)
{
    capture_t* capture = (capture_t*)context;
    if (capture->bus_count >= MAX_EVENTS) capture->overflow = true;
    else capture->bus[capture->bus_count++] = *event;
}

static void on_trace(const wz_trace_event_t* event, void* context)
{
    capture_t* capture = (capture_t*)context;
    if (capture->trace_count >= MAX_EVENTS) capture->overflow = true;
    else capture->trace[capture->trace_count++] = *event;
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

static bool parity(unsigned value)
{
    unsigned count = 0u;
    for (unsigned bit = 0u; bit < 8u; ++bit) count += (value >> bit) & 1u;
    return (count & 1u) == 0u;
}

static answer_t reference(unsigned operation, unsigned value, unsigned old_carry)
{
    unsigned shifted = 0u, carry = 0u;
    unsigned flags;
    switch (operation) {
    case 0u: carry = value >> 7u; shifted = (value << 1u) | carry; break;
    case 1u: carry = value & 1u; shifted = (value >> 1u) | (carry << 7u); break;
    case 2u: carry = value >> 7u; shifted = (value << 1u) | old_carry; break;
    case 3u: carry = value & 1u; shifted = (value >> 1u) | (old_carry << 7u); break;
    case 4u: carry = value >> 7u; shifted = value << 1u; break;
    case 5u: carry = value & 1u; shifted = (value >> 1u) | (value & S); break;
    case 6u: carry = value >> 7u; shifted = (value << 1u) | 1u; break;
    default: carry = value & 1u; shifted = value >> 1u; break;
    }
    shifted &= 255u;
    flags = (shifted & (S | Y | X)) | carry;
    if (shifted == 0u) flags |= Z;
    if (parity(shifted)) flags |= PV;
    return (answer_t){(wz_byte_t)shifted, (wz_byte_t)flags};
}

static bool check_capture(const capture_t* capture,
                          const expected_bus_t* bus, size_t count,
                          wz_byte_t prefix, wz_byte_t second)
{
    size_t bus_events = 0u, opcode_events = 0u, instruction_events = 0u;
    if (capture->overflow || capture->bus_count != count) return false;
    for (size_t index = 0u; index < count; ++index) {
        const wz_bus_request_t* event = &capture->bus[index];
        if (event->cycle != bus[index].cycle ||
            event->master_tick != bus[index].tick ||
            event->address != bus[index].address ||
            event->value != bus[index].value ||
            event->t_states != bus[index].duration ||
            event->contention_delay != 0u) return false;
    }
    for (size_t index = 0u; index < capture->trace_count; ++index) {
        const wz_trace_event_t* event = &capture->trace[index];
        if (index != 0u && event->sequence <= capture->trace[index - 1u].sequence) return false;
        if (event->kind == WZ_TRACE_CPU_BUS) {
            if (bus_events >= count ||
                event->master_tick != bus[bus_events].tick ||
                event->address != bus[bus_events].address ||
                event->value != bus[bus_events].value ||
                event->cycle != (wz_byte_t)bus[bus_events].cycle ||
                event->t_states != bus[bus_events].duration) return false;
            ++bus_events;
        } else if (event->kind == WZ_TRACE_CPU_OPCODE_BYTE) {
            if (opcode_events >= 2u ||
                event->address != 0x8000u + opcode_events ||
                event->value != (opcode_events == 0u ? prefix : second)) return false;
            ++opcode_events;
        } else if (event->kind == WZ_TRACE_CPU_INSTRUCTION) {
            if (event->program_counter != 0x8000u || event->value != prefix) return false;
            ++instruction_events;
        }
    }
    return bus_events == count && opcode_events == 2u && instruction_events == 1u;
}

static bool run_case(wz_machine_t* machine, capture_t* capture,
                     unsigned mode, unsigned target, unsigned operation,
                     unsigned value, unsigned carry)
{
    const bool indexed = mode != 0u;
    const bool memory = indexed || target == 6u;
    const wz_byte_t prefix = indexed ? (mode == 1u ? 0xddu : 0xfdu) : 0xcbu;
    const wz_byte_t opcode = (wz_byte_t)((operation << 3u) | target);
    const wz_byte_t second = indexed ? 0xcbu : opcode;
    const wz_byte_t displacement = mode == 2u ? 0xffu : 0x01u;
    const wz_word_t address = indexed ? 0x9001u : 0x9000u;
    const answer_t answer = reference(operation, value, carry);
    const wz_master_tick_t base = machine->master_tick;
    const wz_master_tick_t duration = indexed ? 46u : memory ? 30u : 16u;
    expected_bus_t bus[7];
    size_t count = 0u;
    wz_z80_state_t expected;
    wz_byte_t expected_memory = (wz_byte_t)value;
    memset(capture, 0, sizeof(*capture));
    machine->cpu.program_counter = 0x8000u;
    machine->cpu.main.a = 0x81u;
    machine->cpu.main.f = (wz_byte_t)(0xc4u | carry);
    machine->cpu.main.b = 0x12u; machine->cpu.main.c = 0x34u;
    machine->cpu.main.d = 0x56u; machine->cpu.main.e = 0x78u;
    machine->cpu.main.h = 0x90u; machine->cpu.main.l = 0x00u;
    machine->cpu.ix = 0x9000u;
    machine->cpu.iy = 0x9002u;
    machine->cpu.memptr = 0xbeefu;
    machine->cpu.r = 0u;
    machine->cpu.q = 0u;
    if (!indexed && target != 6u) *register8(&machine->cpu, target) = (wz_byte_t)value;
    wz_machine_memory_write(machine, address, (wz_byte_t)value);
    wz_machine_memory_write(machine, 0x8000u, prefix);
    wz_machine_memory_write(machine, 0x8001u, second);
    if (indexed) {
        wz_machine_memory_write(machine, 0x8002u, displacement);
        wz_machine_memory_write(machine, 0x8003u, opcode);
    }
    expected = machine->cpu;
    expected.program_counter = indexed ? 0x8004u : 0x8002u;
    expected.r = 2u;
    expected.q = 1u;
    expected.main.f = answer.flags;
    if (indexed) expected.memptr = address;
    if (memory) expected_memory = answer.value;
    if (target != 6u) *register8(&expected, target) = answer.value;
    bus[count++] = (expected_bus_t){WZ_BUS_M1_OPCODE_FETCH, base,
                                     0x8000u, prefix, 4u};
    bus[count++] = (expected_bus_t){WZ_BUS_M1_OPCODE_FETCH, base + 8u,
                                     0x8001u, second, 4u};
    if (indexed) {
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ, base + 16u,
                                         0x8002u, displacement, 3u};
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ, base + 22u,
                                         0x8003u, opcode, 3u};
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ, base + 32u,
                                         address, (wz_byte_t)value, 3u};
        bus[count++] = (expected_bus_t){WZ_BUS_INTERNAL, base + 34u,
                                         address, 0u, 3u};
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_WRITE, base + 40u,
                                         address, answer.value, 3u};
    } else if (memory) {
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ, base + 16u,
                                         address, (wz_byte_t)value, 3u};
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_WRITE, base + 24u,
                                         address, answer.value, 3u};
    }
    if (wz_z80_step(machine) != WZ_RESULT_OK ||
        machine->master_tick != base + duration ||
        memcmp(&machine->cpu, &expected, sizeof(expected)) != 0 ||
        wz_machine_memory_read(machine, address) != expected_memory ||
        !check_capture(capture, bus, count, prefix, second)) {
        (void)fprintf(stderr,
            "CB rotate trace mismatch mode=%u target=%u op=%u value=%02x carry=%u "
            "F=%02x/%02x PC=%04x/%04x ticks=%llu/%llu bus=%zu/%zu trace=%zu\n",
            mode, target, operation, value, carry,
            machine->cpu.main.f, expected.main.f,
            machine->cpu.program_counter, expected.program_counter,
            (unsigned long long)(machine->master_tick - base),
            (unsigned long long)duration, capture->bus_count, count,
            capture->trace_count);
        return false;
    }
    return true;
}

int main(void)
{
    static const unsigned values[] = {0x00u, 0x01u, 0x7fu, 0x80u, 0xffu};
    wz_machine_t machine;
    wz_bus_observer_t observer;
    wz_trace_sink_t sink;
    capture_t capture = {0};
    unsigned cases = 0u;
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) return 1;
    wz_bus_observer_init(&observer, on_bus, &capture);
    if (wz_machine_set_bus_observer(&machine, &observer) != WZ_RESULT_OK) goto failed;
    wz_trace_sink_init(&sink, on_trace, &capture);
    wz_machine_set_timing_trace(&machine, &sink);
    for (unsigned mode = 0u; mode < 3u; ++mode) {
        for (unsigned target = 0u; target < 8u; ++target) {
            for (unsigned operation = 0u; operation < 8u; ++operation) {
                for (size_t value = 0u; value < 5u; ++value) {
                    for (unsigned carry = 0u; carry < 2u; ++carry) {
                        if (!run_case(&machine, &capture, mode, target,
                                      operation, values[value], carry)) goto failed;
                        ++cases;
                    }
                }
            }
        }
    }
    wz_machine_destroy(&machine);
    (void)printf("{\"testId\":\"cpu-cb-rotate-trace\",\"status\":\"pass\","
                 "\"cases\":%u,\"operations\":8,\"targets\":8,\"modes\":3}\n", cases);
    return 0;
failed:
    wz_machine_destroy(&machine);
    return 1;
}
