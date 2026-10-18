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

typedef struct { wz_word_t value; wz_byte_t flags; } answer_t;

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

static int signed_word(unsigned value)
{
    return value < 32768u ? (int)value : (int)value - 65536;
}

/* op 0=ADD HL, 1=ADC HL, 2=SBC HL, 3=ADD IX, 4=ADD IY. */
static answer_t reference(unsigned op, unsigned left, unsigned right,
                          unsigned carry, unsigned original_flags)
{
    answer_t answer;
    unsigned result, flags;
    if (op == 2u) {
        const int difference = (int)left - (int)right - (int)carry;
        const int signed_difference = signed_word(left) - signed_word(right) - (int)carry;
        result = (unsigned)difference & 65535u;
        flags = N;
        if ((left & 4095u) < (right & 4095u) + carry) flags |= H;
        if (signed_difference < -32768 || signed_difference > 32767) flags |= PV;
        if (difference < 0) flags |= C;
    } else {
        const unsigned input_carry = op == 1u ? carry : 0u;
        const unsigned wide = left + right + input_carry;
        const int signed_sum = signed_word(left) + signed_word(right) + (int)input_carry;
        result = wide & 65535u;
        flags = 0u;
        if ((left & 4095u) + (right & 4095u) + input_carry > 4095u) flags |= H;
        if (wide > 65535u) flags |= C;
        if (op == 1u && (signed_sum < -32768 || signed_sum > 32767)) flags |= PV;
    }
    flags |= (result >> 8u) & (Y | X);
    if (op == 0u || op >= 3u) flags |= original_flags & (S | Z | PV);
    else {
        if ((result & 32768u) != 0u) flags |= S;
        if (result == 0u) flags |= Z;
    }
    answer.value = (wz_word_t)result;
    answer.flags = (wz_byte_t)flags;
    return answer;
}

static bool check_capture(const capture_t* capture, wz_master_tick_t base,
                          const wz_byte_t* program, size_t fetch_count)
{
    size_t bus_events = 0u, opcode_events = 0u, instruction_events = 0u;
    if (capture->overflow || capture->bus_count != fetch_count) return false;
    for (size_t index = 0u; index < fetch_count; ++index) {
        const wz_bus_request_t* event = &capture->bus[index];
        if (event->cycle != WZ_BUS_M1_OPCODE_FETCH ||
            event->master_tick != base + 8u * index ||
            event->address != 0x8000u + index ||
            event->value != program[index] || event->t_states != 4u ||
            event->contention_delay != 0u) return false;
    }
    for (size_t index = 0u; index < capture->trace_count; ++index) {
        const wz_trace_event_t* event = &capture->trace[index];
        if (index != 0u && event->sequence <= capture->trace[index - 1u].sequence) return false;
        if (event->kind == WZ_TRACE_CPU_BUS) {
            if (bus_events >= fetch_count ||
                event->master_tick != base + 8u * bus_events ||
                event->address != 0x8000u + bus_events ||
                event->value != program[bus_events] ||
                event->cycle != (wz_byte_t)WZ_BUS_M1_OPCODE_FETCH ||
                event->t_states != 4u) return false;
            ++bus_events;
        } else if (event->kind == WZ_TRACE_CPU_OPCODE_BYTE) {
            if (opcode_events >= fetch_count ||
                event->address != 0x8000u + opcode_events ||
                event->value != program[opcode_events]) return false;
            ++opcode_events;
        } else if (event->kind == WZ_TRACE_CPU_INSTRUCTION) {
            if (event->program_counter != 0x8000u || event->value != program[0]) return false;
            ++instruction_events;
        }
    }
    return bus_events == fetch_count && opcode_events == fetch_count &&
           instruction_events == 1u;
}

static bool run_case(wz_machine_t* machine, capture_t* capture,
                     unsigned op, unsigned left, unsigned right, unsigned carry)
{
    static const wz_byte_t programs[5][2] = {
        {0x09u, 0x00u}, {0xedu, 0x4au}, {0xedu, 0x42u},
        {0xddu, 0x09u}, {0xfdu, 0x09u}
    };
    const size_t fetch_count = op == 0u ? 1u : 2u;
    const wz_master_tick_t duration = op == 0u ? 22u : 30u;
    const wz_master_tick_t base = machine->master_tick;
    const wz_byte_t original_flags = (wz_byte_t)(0xfeu | carry);
    const answer_t answer = reference(op, left, right, carry, original_flags);
    wz_z80_state_t expected;
    memset(capture, 0, sizeof(*capture));
    machine->cpu.program_counter = 0x8000u;
    machine->cpu.main.f = original_flags;
    machine->cpu.main.b = (wz_byte_t)(right >> 8u);
    machine->cpu.main.c = (wz_byte_t)right;
    machine->cpu.main.h = (wz_byte_t)(left >> 8u);
    machine->cpu.main.l = (wz_byte_t)left;
    machine->cpu.ix = (wz_word_t)left;
    machine->cpu.iy = (wz_word_t)left;
    machine->cpu.memptr = 0xbeefu;
    machine->cpu.r = 0u;
    machine->cpu.q = 0u;
    wz_machine_memory_write(machine, 0x8000u, programs[op][0]);
    wz_machine_memory_write(machine, 0x8001u, programs[op][1]);
    expected = machine->cpu;
    expected.program_counter = (wz_word_t)(0x8000u + fetch_count);
    expected.r = (wz_byte_t)fetch_count;
    expected.q = 1u;
    expected.main.f = answer.flags;
    expected.memptr = (wz_word_t)(left + 1u);
    if (op <= 2u) {
        expected.main.h = (wz_byte_t)(answer.value >> 8u);
        expected.main.l = (wz_byte_t)answer.value;
    } else if (op == 3u) expected.ix = answer.value;
    else expected.iy = answer.value;
    if (wz_z80_step(machine) != WZ_RESULT_OK ||
        machine->master_tick != base + duration ||
        memcmp(&machine->cpu, &expected, sizeof(expected)) != 0 ||
        !check_capture(capture, base, programs[op], fetch_count)) {
        (void)fprintf(stderr,
            "wide arithmetic trace mismatch op=%u left=%04x right=%04x carry=%u "
            "F=%02x/%02x PC=%04x/%04x ticks=%llu/%llu bus=%zu trace=%zu\n",
            op, left, right, carry, machine->cpu.main.f, expected.main.f,
            machine->cpu.program_counter, expected.program_counter,
            (unsigned long long)(machine->master_tick - base),
            (unsigned long long)duration, capture->bus_count, capture->trace_count);
        return false;
    }
    return true;
}

int main(void)
{
    static const unsigned edges[] = {0x0000u, 0x0fffu, 0x1000u, 0x7fffu,
                                     0x8000u, 0xffffu};
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
    for (unsigned op = 0u; op < 5u; ++op) {
        for (size_t left = 0u; left < 6u; ++left) {
            for (size_t right = 0u; right < 6u; ++right) {
                for (unsigned carry = 0u; carry < 2u; ++carry) {
                    if (!run_case(&machine, &capture, op, edges[left], edges[right], carry)) goto failed;
                    ++cases;
                }
            }
        }
    }
    wz_machine_destroy(&machine);
    (void)printf("{\"testId\":\"cpu-wide-arithmetic-trace\",\"status\":\"pass\","
                 "\"cases\":%u,\"operations\":5,\"edgeValues\":6}\n", cases);
    return 0;
failed:
    wz_machine_destroy(&machine);
    return 1;
}
