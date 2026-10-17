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

#define MAX_EVENTS 32u

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

static bool check_trace(const capture_t* capture, const expected_bus_t* expected,
                        size_t count, unsigned opcode, unsigned step)
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
            if (event->program_counter != 0x8000u || event->value != 0xedu) goto failed;
            ++instruction;
        } else if (event->kind == WZ_TRACE_CPU_OPCODE_BYTE) {
            if (opcode_bytes == 0u &&
                (event->address != 0x8000u || event->value != 0xedu)) goto failed;
            if (opcode_bytes == 1u &&
                (event->address != 0x8001u || event->value != opcode)) goto failed;
            ++opcode_bytes;
        }
    }
    if (trace_bus == count && instruction == 1u && opcode_bytes == 2u) return true;
failed:
    (void)fprintf(stderr,
        "block trace mismatch opcode=%02x step=%u bus=%zu/%zu trace=%zu\n",
        opcode, step, capture->bus_count, count, capture->trace_count);
    for (size_t index = 0u; index < capture->bus_count; ++index) {
        (void)fprintf(stderr, "  bus[%zu]=%u tick=%llu addr=%04x value=%02x t=%u\n",
            index, (unsigned)capture->bus[index].cycle,
            (unsigned long long)capture->bus[index].master_tick,
            capture->bus[index].address, capture->bus[index].value,
            capture->bus[index].t_states);
    }
    return false;
}

static bool run_case(unsigned opcode, bool early_match)
{
    const unsigned family = opcode & 7u;
    const bool decrement = (opcode & 8u) != 0u;
    const bool repeat = (opcode & 16u) != 0u;
    const unsigned steps = repeat && !early_match ? 2u : 1u;
    const wz_word_t first = decrement ? 0x9001u : 0x9000u;
    const wz_word_t destination = decrement ? 0xa001u : 0xa000u;
    const int direction = decrement ? -1 : 1;
    wz_machine_t machine;
    wz_bus_observer_t observer;
    wz_trace_sink_t sink;
    capture_t capture;
    bool initialized = false;
    bool result = false;
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        (void)fputs("machine init failed\n", stderr);
        return false;
    }
    initialized = true;
    wz_machine_memory_write(&machine, 0x8000u, 0xedu);
    wz_machine_memory_write(&machine, 0x8001u, (wz_byte_t)opcode);
    wz_machine_memory_write(&machine, first, early_match ? 0x34u : 0x12u);
    wz_machine_memory_write(&machine, (wz_word_t)(first + direction), 0x34u);
    machine.cpu.program_counter = 0x8000u;
    machine.cpu.main.a = 0x34u;
    machine.cpu.main.h = (wz_byte_t)(first >> 8u);
    machine.cpu.main.l = (wz_byte_t)first;
    machine.cpu.main.d = (wz_byte_t)(destination >> 8u);
    machine.cpu.main.e = (wz_byte_t)destination;
    machine.cpu.main.b = family >= 2u ? 2u : 0u;
    machine.cpu.main.c = family >= 2u ? 0x35u : 2u;
    machine.cpu.r = 0u;
    memset(&capture, 0, sizeof(capture));
    wz_bus_observer_init(&observer, record_bus, &capture);
    if (wz_machine_set_bus_observer(&machine, &observer) != WZ_RESULT_OK) goto cleanup;
    wz_trace_sink_init(&sink, record_trace, &capture);
    wz_machine_set_timing_trace(&machine, &sink);

    for (unsigned step = 0u; step < steps; ++step) {
        expected_bus_t expected[4];
        size_t count = 0u;
        const wz_master_tick_t base = machine.master_tick;
        const wz_word_t address = (wz_word_t)(first + (int)step * direction);
        const wz_word_t target = (wz_word_t)(destination + (int)step * direction);
        const wz_byte_t value = (wz_byte_t)(step == 0u ?
                                  (early_match ? 0x34u : 0x12u) : 0x34u);
        const wz_byte_t b_before = (wz_byte_t)(2u - step);
        const bool repeats_now = repeat && !early_match && step == 0u;
        const wz_master_tick_t duration = repeats_now ? 42u : 32u;
        memset(&capture, 0, sizeof(capture));
        append(expected, &count, WZ_BUS_M1_OPCODE_FETCH, base,
               0x8000u, 0xedu, 4u);
        append(expected, &count, WZ_BUS_M1_OPCODE_FETCH, base + 8u,
               0x8001u, (wz_byte_t)opcode, 4u);
        switch (family) {
        case 0u:
            append(expected, &count, WZ_BUS_MEMORY_READ, base + 16u,
                   address, value, 3u);
            append(expected, &count, WZ_BUS_MEMORY_WRITE, base + 22u,
                   target, value, 3u);
            break;
        case 1u:
            append(expected, &count, WZ_BUS_MEMORY_READ, base + 16u,
                   address, value, 3u);
            break;
        case 2u:
            append(expected, &count, WZ_BUS_IO_READ, base + 18u,
                   (wz_word_t)(((wz_word_t)b_before << 8u) | 0x35u), 0xffu, 4u);
            append(expected, &count, WZ_BUS_MEMORY_WRITE, base + 26u,
                   address, 0xffu, 3u);
            break;
        default:
            append(expected, &count, WZ_BUS_MEMORY_READ, base + 18u,
                   address, value, 3u);
            append(expected, &count, WZ_BUS_IO_WRITE, base + 24u,
                   (wz_word_t)(((wz_word_t)(b_before - 1u) << 8u) | 0x35u),
                   value, 4u);
            break;
        }
        if (wz_z80_step(&machine) != WZ_RESULT_OK ||
            !check_trace(&capture, expected, count, opcode, step) ||
            machine.master_tick != base + duration ||
            machine.cpu.program_counter != (repeats_now ? 0x8000u : 0x8002u) ||
            machine.cpu.r != (wz_byte_t)(2u * (step + 1u)) ||
            machine.cpu.q != 1u) {
            (void)fprintf(stderr, "block state mismatch opcode=%02x step=%u "
                                  "PC=%04x ticks=%llu expected=%llu\n",
                opcode, step, machine.cpu.program_counter,
                (unsigned long long)machine.master_tick,
                (unsigned long long)(base + duration));
            goto cleanup;
        }
        if (family == 0u && wz_machine_memory_read(&machine, target) != value) {
            goto cleanup;
        }
        if (family == 2u && wz_machine_memory_read(&machine, address) != 0xffu) {
            goto cleanup;
        }
    }
    result = true;
cleanup:
    if (initialized) wz_machine_destroy(&machine);
    return result;
}

int main(void)
{
    static const wz_byte_t opcodes[] = {
        0xa0u, 0xa1u, 0xa2u, 0xa3u, 0xa8u, 0xa9u, 0xaau, 0xabu,
        0xb0u, 0xb1u, 0xb2u, 0xb3u, 0xb8u, 0xb9u, 0xbau, 0xbbu
    };
    unsigned steps = 0u;
    for (size_t index = 0u; index < sizeof(opcodes) / sizeof(opcodes[0]); ++index) {
        if (!run_case(opcodes[index], false)) return 1;
        steps += (opcodes[index] & 16u) != 0u ? 2u : 1u;
    }
    if (!run_case(0xb1u, true) || !run_case(0xb9u, true)) return 1;
    steps += 2u;
    (void)printf("{\"testId\":\"cpu-block-repeat\",\"status\":\"pass\","
                 "\"opcodes\":16,\"earlyMatchCases\":2,\"steps\":%u}\n", steps);
    return 0;
}
