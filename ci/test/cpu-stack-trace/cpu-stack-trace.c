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

#define MAX_EVENTS 16u

typedef enum { CALL, RET, RST, PUSH, POP } kind_t;

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
                   wz_word_t address, wz_byte_t value, wz_byte_t t_states)
{
    expected[*count].cycle = cycle;
    expected[*count].tick = tick;
    expected[*count].address = address;
    expected[*count].value = value;
    expected[*count].t_states = t_states;
    ++*count;
}

static bool check_bus_trace(const capture_t* capture, const expected_bus_t* expected,
                            size_t count, unsigned opcode)
{
    size_t trace_bus = 0u;
    size_t instruction = 0u;
    size_t opcode_bytes = 0u;
    if (capture->overflow || capture->bus_count != count) goto failed;
    for (size_t index = 0u; index < count; ++index) {
        const wz_bus_request_t* actual = &capture->bus[index];
        if (actual->cycle != expected[index].cycle ||
            actual->master_tick != expected[index].tick ||
            actual->address != expected[index].address ||
            actual->value != expected[index].value ||
            actual->t_states != expected[index].t_states ||
            actual->contention_delay != 0u) goto failed;
    }
    for (size_t index = 0u; index < capture->trace_count; ++index) {
        const wz_trace_event_t* event = &capture->trace[index];
        if (index != 0u && event->sequence <= capture->trace[index - 1u].sequence) {
            goto failed;
        }
        if (event->kind == WZ_TRACE_CPU_BUS) {
            if (trace_bus >= count ||
                event->master_tick != expected[trace_bus].tick ||
                event->address != expected[trace_bus].address ||
                event->value != expected[trace_bus].value ||
                event->cycle != (wz_byte_t)expected[trace_bus].cycle ||
                event->t_states != expected[trace_bus].t_states) goto failed;
            ++trace_bus;
        } else if (event->kind == WZ_TRACE_CPU_INSTRUCTION) {
            if (event->program_counter != 0x8000u || event->value != opcode) goto failed;
            ++instruction;
        } else if (event->kind == WZ_TRACE_CPU_OPCODE_BYTE) {
            if (event->address != 0x8000u || event->value != opcode) goto failed;
            ++opcode_bytes;
        }
    }
    if (trace_bus == count && instruction == 1u && opcode_bytes == 1u) return true;
failed:
    (void)fprintf(stderr, "stack trace mismatch opcode=%02x bus=%zu/%zu trace=%zu\n",
                  opcode, capture->bus_count, count, capture->trace_count);
    for (size_t index = 0u; index < capture->bus_count; ++index) {
        (void)fprintf(stderr, "  bus[%zu]=%u tick=%llu addr=%04x value=%02x t=%u\n",
            index, (unsigned)capture->bus[index].cycle,
            (unsigned long long)capture->bus[index].master_tick,
            capture->bus[index].address, capture->bus[index].value,
            capture->bus[index].t_states);
    }
    return false;
}

static bool run_case(kind_t kind, wz_byte_t opcode, wz_byte_t flags,
                     bool taken, bool wrap)
{
    const bool writes_stack = kind == CALL || kind == RST || kind == PUSH;
    const wz_word_t initial_sp = writes_stack ? (wrap ? 0x0000u : 0x9002u) :
                               (wrap ? 0xfffeu : 0x9000u);
    const wz_word_t high_address = (wz_word_t)(initial_sp - 1u);
    const wz_word_t low_address = (wz_word_t)(initial_sp - 2u);
    const wz_word_t pop_high_address = (wz_word_t)(initial_sp + 1u);
    const wz_word_t pair_values[4] = {
        0x1234u, 0x5678u, 0x9abcu, (wz_word_t)(0xde00u | flags)
    };
    const unsigned pair = (opcode >> 4u) & 3u;
    const wz_word_t pair_value = pair_values[pair];
    const wz_byte_t push_high = (wz_byte_t)((kind == PUSH ? pair_value :
                                  (kind == CALL ? 0x8003u : 0x8001u)) >> 8u);
    const wz_byte_t push_low = (wz_byte_t)(kind == PUSH ? pair_value :
                                 (kind == CALL ? 0x8003u : 0x8001u));
    const bool conditional = (kind == CALL && opcode != 0xcdu) ||
                             (kind == RET && opcode != 0xc9u);
    wz_machine_t machine;
    wz_bus_observer_t observer;
    wz_trace_sink_t sink;
    capture_t capture = {0};
    expected_bus_t expected[6];
    size_t count = 0u;
    wz_word_t expected_pc = 0x8001u;
    wz_word_t expected_sp = initial_sp;
    wz_word_t expected_memptr = 0xbeefu;
    wz_master_tick_t expected_tick = 0u;
    bool result = false;
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        (void)fputs("machine init failed\n", stderr);
        return false;
    }
    wz_machine_memory_write(&machine, 0x8000u, opcode);
    wz_machine_memory_write(&machine, 0x8001u, 0x23u);
    wz_machine_memory_write(&machine, 0x8002u, 0xa1u);
    if (!writes_stack) {
        wz_machine_memory_write(&machine, initial_sp, 0xadu);
        wz_machine_memory_write(&machine, pop_high_address, 0xa1u);
    }
    machine.cpu.program_counter = 0x8000u;
    machine.cpu.stack_pointer = initial_sp;
    machine.cpu.memptr = 0xbeefu;
    machine.cpu.main.a = 0xdeu;
    machine.cpu.main.f = flags;
    machine.cpu.main.b = 0x12u; machine.cpu.main.c = 0x34u;
    machine.cpu.main.d = 0x56u; machine.cpu.main.e = 0x78u;
    machine.cpu.main.h = 0x9au; machine.cpu.main.l = 0xbcu;
    wz_bus_observer_init(&observer, record_bus, &capture);
    if (wz_machine_set_bus_observer(&machine, &observer) != WZ_RESULT_OK) goto cleanup;
    wz_trace_sink_init(&sink, record_trace, &capture);
    wz_machine_set_timing_trace(&machine, &sink);
    append(expected, &count, WZ_BUS_M1_OPCODE_FETCH, 0u, 0x8000u, opcode, 4u);
    switch (kind) {
    case CALL:
        append(expected, &count, WZ_BUS_MEMORY_READ, 8u, 0x8001u, 0x23u, 3u);
        append(expected, &count, WZ_BUS_MEMORY_READ, 14u, 0x8002u, 0xa1u, 3u);
        expected_pc = taken ? 0xa123u : 0x8003u;
        expected_memptr = 0xa123u;
        expected_tick = taken ? 34u : 20u;
        if (!taken) break;
        append(expected, &count, WZ_BUS_INTERNAL, 20u, 0x8003u, 0u, 1u);
        append(expected, &count, WZ_BUS_MEMORY_WRITE, 22u,
               high_address, push_high, 3u);
        append(expected, &count, WZ_BUS_MEMORY_WRITE, 28u,
               low_address, push_low, 3u);
        expected_sp = low_address;
        break;
    case RET:
        if (conditional) append(expected, &count, WZ_BUS_INTERNAL,
                                8u, 0x8001u, 0u, 1u);
        expected_tick = conditional ? (taken ? 22u : 10u) : 20u;
        if (!taken) break;
        append(expected, &count, WZ_BUS_MEMORY_READ, conditional ? 10u : 8u,
               initial_sp, 0xadu, 3u);
        append(expected, &count, WZ_BUS_MEMORY_READ, conditional ? 16u : 14u,
               pop_high_address, 0xa1u, 3u);
        expected_sp = (wz_word_t)(initial_sp + 2u);
        expected_pc = 0xa1adu;
        expected_memptr = expected_pc;
        break;
    case RST:
        append(expected, &count, WZ_BUS_INTERNAL, 8u, 0x8001u, 0u, 1u);
        append(expected, &count, WZ_BUS_MEMORY_WRITE, 10u,
               high_address, push_high, 3u);
        append(expected, &count, WZ_BUS_MEMORY_WRITE, 16u,
               low_address, push_low, 3u);
        expected_tick = 22u;
        expected_sp = low_address;
        expected_pc = (wz_word_t)(opcode & 0x38u);
        expected_memptr = expected_pc;
        break;
    case PUSH:
        append(expected, &count, WZ_BUS_INTERNAL, 8u, initial_sp, 0u, 1u);
        append(expected, &count, WZ_BUS_MEMORY_WRITE, 10u,
               high_address, push_high, 3u);
        append(expected, &count, WZ_BUS_MEMORY_WRITE, 16u,
               low_address, push_low, 3u);
        expected_tick = 22u;
        expected_sp = low_address;
        break;
    case POP:
        append(expected, &count, WZ_BUS_MEMORY_READ, 8u,
               initial_sp, 0xadu, 3u);
        append(expected, &count, WZ_BUS_MEMORY_READ, 14u,
               pop_high_address, 0xa1u, 3u);
        expected_tick = 20u;
        expected_sp = (wz_word_t)(initial_sp + 2u);
        break;
    }
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        !check_bus_trace(&capture, expected, count, opcode) ||
        machine.cpu.program_counter != expected_pc ||
        machine.cpu.stack_pointer != expected_sp ||
        machine.cpu.memptr != expected_memptr ||
        machine.master_tick != expected_tick || machine.cpu.r != 1u) {
        (void)fprintf(stderr, "stack state mismatch kind=%u opcode=%02x "
                              "taken=%u wrap=%u PC=%04x/%04x SP=%04x/%04x "
                              "tick=%llu/%llu memptr=%04x/%04x\n",
            (unsigned)kind, opcode, (unsigned)taken, (unsigned)wrap,
            machine.cpu.program_counter, expected_pc,
            machine.cpu.stack_pointer, expected_sp,
            (unsigned long long)machine.master_tick,
            (unsigned long long)expected_tick,
            machine.cpu.memptr, expected_memptr);
        goto cleanup;
    }
    if (writes_stack && (kind == PUSH || taken) &&
        (wz_machine_memory_read(&machine, high_address) != push_high ||
         wz_machine_memory_read(&machine, low_address) != push_low)) goto cleanup;
    if (kind == POP) {
        const wz_word_t actual = pair == 0u ?
            (wz_word_t)(((wz_word_t)machine.cpu.main.b << 8u) | machine.cpu.main.c) :
            pair == 1u ?
            (wz_word_t)(((wz_word_t)machine.cpu.main.d << 8u) | machine.cpu.main.e) :
            pair == 2u ?
            (wz_word_t)(((wz_word_t)machine.cpu.main.h << 8u) | machine.cpu.main.l) :
            (wz_word_t)(((wz_word_t)machine.cpu.main.a << 8u) | machine.cpu.main.f);
        if (actual != 0xa1adu) goto cleanup;
    } else if (machine.cpu.main.f != flags) goto cleanup;
    result = true;
cleanup:
    wz_machine_destroy(&machine);
    return result;
}

static wz_byte_t condition_flags(unsigned condition, bool taken)
{
    static const wz_byte_t masks[4] = {0x40u, 0x01u, 0x04u, 0x80u};
    const wz_byte_t mask = masks[condition / 2u];
    const bool set = (condition & 1u) != 0u;
    return (wz_byte_t)((taken == set) ? mask : 0u);
}

int main(void)
{
    unsigned cases = 0u;
    for (unsigned condition = 0u; condition < 8u; ++condition) {
        for (unsigned taken = 0u; taken < 2u; ++taken) {
            const wz_byte_t flags = condition_flags(condition, taken != 0u);
            if (!run_case(CALL, (wz_byte_t)(0xc4u + condition * 8u),
                          flags, taken != 0u, false) ||
                !run_case(RET, (wz_byte_t)(0xc0u + condition * 8u),
                          flags, taken != 0u, false)) return 1;
            cases += 2u;
        }
    }
    if (!run_case(CALL, 0xcdu, 0xf0u, true, false) ||
        !run_case(RET, 0xc9u, 0xf0u, true, false)) return 1;
    cases += 2u;
    for (unsigned vector = 0u; vector < 8u; ++vector) {
        if (!run_case(RST, (wz_byte_t)(0xc7u + vector * 8u),
                      0xf0u, true, false)) return 1;
        ++cases;
    }
    for (unsigned pair = 0u; pair < 4u; ++pair) {
        if (!run_case(PUSH, (wz_byte_t)(0xc5u + pair * 16u),
                      0xf0u, true, false) ||
            !run_case(POP, (wz_byte_t)(0xc1u + pair * 16u),
                      0xf0u, true, false)) return 1;
        cases += 2u;
    }
    if (!run_case(CALL, 0xcdu, 0xf0u, true, true) ||
        !run_case(RET, 0xc9u, 0xf0u, true, true) ||
        !run_case(RST, 0xffu, 0xf0u, true, true) ||
        !run_case(PUSH, 0xf5u, 0xf0u, true, true) ||
        !run_case(POP, 0xf1u, 0xf0u, true, true)) return 1;
    cases += 5u;
    (void)printf("{\"testId\":\"cpu-stack-trace\",\"status\":\"pass\","
                 "\"cases\":%u,\"conditionalPaths\":32,\"wrapCases\":5}\n", cases);
    return 0;
}
