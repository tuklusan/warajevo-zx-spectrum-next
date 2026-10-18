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

enum { FLAG_C = 1, FLAG_PV = 4, FLAG_X = 8, FLAG_H = 16,
       FLAG_Y = 32, FLAG_Z = 64, FLAG_S = 128, MAX_EVENTS = 20 };

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
    if (capture->bus_count == MAX_EVENTS) capture->overflow = true;
    else capture->bus[capture->bus_count++] = *request;
}

static void record_trace(const wz_trace_event_t* event, void* context)
{
    capture_t* capture = (capture_t*)context;
    if (capture->trace_count == MAX_EVENTS) capture->overflow = true;
    else capture->trace[capture->trace_count++] = *event;
}

static void append(expected_bus_t* expected, size_t* count,
                   wz_bus_cycle_t cycle, wz_master_tick_t tick,
                   wz_word_t address, wz_byte_t value, wz_byte_t duration)
{
    expected[*count].cycle = cycle;
    expected[*count].tick = tick;
    expected[*count].address = address;
    expected[*count].value = value;
    expected[*count].duration = duration;
    ++*count;
}

static bool check_bus_trace(const capture_t* capture,
                            const expected_bus_t* expected, size_t count,
                            wz_byte_t prefix, wz_byte_t second)
{
    size_t bus_trace = 0u;
    size_t instruction = 0u;
    size_t opcode_bytes = 0u;
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
            if (bus_trace >= count ||
                event->master_tick != expected[bus_trace].tick ||
                event->address != expected[bus_trace].address ||
                event->value != expected[bus_trace].value ||
                event->cycle != (wz_byte_t)expected[bus_trace].cycle ||
                event->t_states != expected[bus_trace].duration) return false;
            ++bus_trace;
        } else if (event->kind == WZ_TRACE_CPU_INSTRUCTION) {
            if (event->program_counter != 0x8000u || event->value != prefix) return false;
            ++instruction;
        } else if (event->kind == WZ_TRACE_CPU_OPCODE_BYTE) {
            if (opcode_bytes >= 2u ||
                event->address != (wz_word_t)(0x8000u + opcode_bytes) ||
                event->value != (opcode_bytes == 0u ? prefix : second)) return false;
            ++opcode_bytes;
        }
    }
    return bus_trace == count && instruction == 1u && opcode_bytes == 2u;
}

/* Modes: CB B, CB (HL), DD CB +1 to B, FD CB -1 to B, DD CB +1 memory. */
static bool run_case(wz_machine_t* machine, capture_t* capture,
                     unsigned mode, unsigned operation, unsigned bit,
                     unsigned value, unsigned carry)
{
    const bool indexed = mode >= 2u;
    const bool memory = mode == 1u || indexed;
    const bool copy_to_b = mode == 2u || mode == 3u;
    const wz_word_t address = indexed ? 0xa801u : 0x9000u;
    const wz_byte_t prefix = (wz_byte_t)(indexed ? (mode == 3u ? 0xfdu : 0xddu) : 0xcbu);
    const wz_byte_t second = (wz_byte_t)(indexed ? 0xcbu :
        ((operation == 0u ? 0x40u : operation == 1u ? 0x80u : 0xc0u) |
         (bit << 3u) | (mode == 1u ? 6u : 0u)));
    const wz_byte_t final_opcode = (wz_byte_t)(
        (operation == 0u ? 0x40u : operation == 1u ? 0x80u : 0xc0u) |
        (bit << 3u) | (mode == 1u || mode == 4u ? 6u : 0u));
    const wz_byte_t mask = (wz_byte_t)(1u << bit);
    const wz_byte_t changed = (wz_byte_t)(operation == 1u ?
        value & (unsigned)~mask : value | mask);
    const wz_byte_t old_flags = (wz_byte_t)(0xc4u | carry);
    const unsigned xy_source = indexed ? address >> 8u :
        (memory ? 0x28u : value);
    wz_byte_t flags = old_flags;
    const wz_master_tick_t before = machine->master_tick;
    const wz_master_tick_t duration = indexed ? (operation == 0u ? 40u : 46u) :
        (memory ? (operation == 0u ? 24u : 30u) : 16u);
    expected_bus_t expected[8];
    size_t count = 0u;
    if (operation == 0u) {
        flags = (wz_byte_t)(carry | FLAG_H | (xy_source & (FLAG_X | FLAG_Y)));
        if ((value & mask) == 0u) flags |= FLAG_Z | FLAG_PV;
        if (bit == 7u && (value & mask) != 0u) flags |= FLAG_S;
    }
    memset(capture, 0, sizeof(*capture));
    machine->cpu.program_counter = 0x8000u;
    machine->cpu.main.b = (wz_byte_t)(mode == 0u ? value : 0x5au);
    machine->cpu.main.h = 0x90u;
    machine->cpu.main.l = 0x00u;
    machine->cpu.main.f = old_flags;
    machine->cpu.ix = 0xa800u;
    machine->cpu.iy = 0xa802u;
    machine->cpu.memptr = 0x2800u;
    machine->cpu.r = 0u;
    machine->cpu.q = 0u;
    wz_machine_memory_write(machine, address, (wz_byte_t)value);
    wz_machine_memory_write(machine, 0x8000u, prefix);
    wz_machine_memory_write(machine, 0x8001u, second);
    if (indexed) {
        wz_machine_memory_write(machine, 0x8002u,
            (wz_byte_t)(mode == 3u ? 0xffu : 0x01u));
        wz_machine_memory_write(machine, 0x8003u, final_opcode);
    }
    append(expected, &count, WZ_BUS_M1_OPCODE_FETCH, before,
           0x8000u, prefix, 4u);
    append(expected, &count, WZ_BUS_M1_OPCODE_FETCH, before + 8u,
           0x8001u, second, 4u);
    if (indexed) {
        append(expected, &count, WZ_BUS_MEMORY_READ, before + 16u,
               0x8002u, (wz_byte_t)(mode == 3u ? 0xffu : 0x01u), 3u);
        append(expected, &count, WZ_BUS_MEMORY_READ, before + 22u,
               0x8003u, final_opcode, 3u);
        append(expected, &count, WZ_BUS_MEMORY_READ, before + 32u,
               address, (wz_byte_t)value, 3u);
        append(expected, &count, WZ_BUS_INTERNAL, before + 34u,
               address, 0u, 3u);
        if (operation != 0u) {
            append(expected, &count, WZ_BUS_MEMORY_WRITE, before + 40u,
                   address, changed, 3u);
        }
    } else if (memory) {
        append(expected, &count, WZ_BUS_MEMORY_READ, before + 16u,
               address, (wz_byte_t)value, 3u);
        if (operation != 0u) {
            append(expected, &count, WZ_BUS_MEMORY_WRITE, before + 24u,
                   address, changed, 3u);
        }
    }
    if (wz_z80_step(machine) != WZ_RESULT_OK ||
        machine->cpu.program_counter != (indexed ? 0x8004u : 0x8002u) ||
        machine->master_tick != before + duration ||
        machine->cpu.r != 2u ||
        machine->cpu.q != (operation == 0u ? 1u : 0u) ||
        machine->cpu.main.f != flags ||
        machine->cpu.main.b != (mode == 0u || (copy_to_b && operation != 0u) ?
            (operation == 0u ? (wz_byte_t)value : changed) : 0x5au) ||
        wz_machine_memory_read(machine, address) !=
            (memory && operation != 0u ? changed : (wz_byte_t)value) ||
        (indexed && machine->cpu.memptr != address) ||
        !check_bus_trace(capture, expected, count, prefix, second)) {
        (void)fprintf(stderr,
            "BIT/RES/SET mismatch mode=%u op=%u bit=%u value=%02x carry=%u "
            "F=%02x/%02x B=%02x PC=%04x ticks=%llu/%llu bus=%zu/%zu trace=%zu\n",
            mode, operation, bit, value, carry, machine->cpu.main.f,
            flags, machine->cpu.main.b, machine->cpu.program_counter,
            (unsigned long long)(machine->master_tick - before),
            (unsigned long long)duration, capture->bus_count, count,
            capture->trace_count);
        return false;
    }
    return true;
}

int main(void)
{
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
    for (unsigned mode = 0u; mode < 5u; ++mode) {
        for (unsigned operation = 0u; operation < 3u; ++operation) {
            for (unsigned bit = 0u; bit < 8u; ++bit) {
                for (unsigned value = 0u; value < 256u; ++value) {
                    for (unsigned carry = 0u; carry < 2u; ++carry) {
                        if (!run_case(&machine, &capture, mode, operation,
                                      bit, value, carry)) {
                            wz_machine_destroy(&machine);
                            return 1;
                        }
                        ++cases;
                    }
                }
            }
        }
    }
    wz_machine_destroy(&machine);
    (void)printf("{\"testId\":\"cpu-bit-set-reset\",\"status\":\"pass\","
                 "\"cases\":%u,\"bitCases\":20480,\"resSetCases\":40960}"
                 "\n", cases);
    return 0;
}
