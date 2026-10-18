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

typedef struct { wz_byte_t value, flags; } answer_t;

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

static int signed_byte(unsigned value)
{
    return value < 128u ? (int)value : (int)value - 256;
}

static unsigned common_flags(unsigned result)
{
    return (result & (S | Y | X)) | (result == 0u ? Z : 0u);
}

static bool parity(unsigned value)
{
    unsigned count = 0u;
    for (unsigned bit = 0u; bit < 8u; ++bit) count += (value >> bit) & 1u;
    return (count & 1u) == 0u;
}

static answer_t alu_answer(unsigned operation, unsigned left,
                           unsigned operand, unsigned carry)
{
    answer_t answer = {(wz_byte_t)left, 0u};
    unsigned flags, value;
    const unsigned input_carry = operation == 1u || operation == 3u ? carry : 0u;
    if (operation <= 1u) {
        const unsigned wide = left + operand + input_carry;
        const int signed_sum = signed_byte(left) + signed_byte(operand) + (int)input_carry;
        value = wide & 255u;
        flags = common_flags(value);
        if ((left & 15u) + (operand & 15u) + input_carry > 15u) flags |= H;
        if (signed_sum < -128 || signed_sum > 127) flags |= PV;
        if (wide > 255u) flags |= C;
        answer.value = (wz_byte_t)value;
    } else if (operation <= 3u || operation == 7u) {
        const int difference = (int)left - (int)operand - (int)input_carry;
        const int signed_difference = signed_byte(left) - signed_byte(operand) - (int)input_carry;
        value = (unsigned)difference & 255u;
        flags = common_flags(value) | N;
        if (operation == 7u) flags = (flags & ~(Y | X)) | (operand & (Y | X));
        else answer.value = (wz_byte_t)value;
        if ((left & 15u) < (operand & 15u) + input_carry) flags |= H;
        if (signed_difference < -128 || signed_difference > 127) flags |= PV;
        if (difference < 0) flags |= C;
    } else {
        value = operation == 4u ? left & operand :
                operation == 5u ? left ^ operand : left | operand;
        flags = common_flags(value) | (parity(value) ? PV : 0u) |
                (operation == 4u ? H : 0u);
        answer.value = (wz_byte_t)value;
    }
    answer.flags = (wz_byte_t)flags;
    return answer;
}

static answer_t inc_dec_answer(bool decrement, unsigned value, unsigned carry)
{
    const unsigned result = decrement ? (value - 1u) & 255u :
                                        (value + 1u) & 255u;
    unsigned flags = common_flags(result) | carry;
    if (decrement) {
        flags |= N;
        if ((value & 15u) == 0u) flags |= H;
        if (value == 128u) flags |= PV;
    } else {
        if ((value & 15u) == 15u) flags |= H;
        if (value == 127u) flags |= PV;
    }
    return (answer_t){(wz_byte_t)result, (wz_byte_t)flags};
}

static bool check_capture(const capture_t* capture, wz_master_tick_t base,
                          wz_byte_t prefix, wz_byte_t opcode)
{
    size_t bus_events = 0u, opcode_events = 0u, instruction_events = 0u;
    if (capture->overflow || capture->bus_count != 2u) return false;
    for (size_t index = 0u; index < 2u; ++index) {
        const wz_bus_request_t* event = &capture->bus[index];
        if (event->cycle != WZ_BUS_M1_OPCODE_FETCH ||
            event->master_tick != base + 8u * index ||
            event->address != 0x8000u + index ||
            event->value != (index == 0u ? prefix : opcode) ||
            event->t_states != 4u || event->contention_delay != 0u) return false;
    }
    for (size_t index = 0u; index < capture->trace_count; ++index) {
        const wz_trace_event_t* event = &capture->trace[index];
        if (index != 0u && event->sequence <= capture->trace[index - 1u].sequence) return false;
        if (event->kind == WZ_TRACE_CPU_BUS) {
            if (bus_events >= 2u ||
                event->master_tick != base + 8u * bus_events ||
                event->address != 0x8000u + bus_events ||
                event->value != (bus_events == 0u ? prefix : opcode) ||
                event->cycle != (wz_byte_t)WZ_BUS_M1_OPCODE_FETCH ||
                event->t_states != 4u) return false;
            ++bus_events;
        } else if (event->kind == WZ_TRACE_CPU_OPCODE_BYTE) {
            if (opcode_events >= 2u ||
                event->address != 0x8000u + opcode_events ||
                event->value != (opcode_events == 0u ? prefix : opcode)) return false;
            ++opcode_events;
        } else if (event->kind == WZ_TRACE_CPU_INSTRUCTION) {
            if (event->program_counter != 0x8000u || event->value != prefix) return false;
            ++instruction_events;
        }
    }
    return bus_events == 2u && opcode_events == 2u && instruction_events == 1u;
}

static bool run_case(wz_machine_t* machine, capture_t* capture,
                     unsigned index_mode, unsigned half, unsigned operation,
                     unsigned carry, bool inc_dec, bool decrement)
{
    const wz_byte_t prefix = index_mode == 0u ? 0xddu : 0xfdu;
    const wz_byte_t opcode = inc_dec ?
        (wz_byte_t)((half == 0u ? 0x24u : 0x2cu) + (decrement ? 1u : 0u)) :
        (wz_byte_t)(0x84u + (operation << 3u) + half);
    const unsigned operand = half == 0u ? 0x7fu : 0x80u;
    const answer_t answer = inc_dec ?
        inc_dec_answer(decrement, operand, carry) :
        alu_answer(operation, 0x81u, operand, carry);
    const wz_master_tick_t base = machine->master_tick;
    wz_z80_state_t expected;
    memset(capture, 0, sizeof(*capture));
    machine->cpu.program_counter = 0x8000u;
    machine->cpu.main.a = 0x81u;
    machine->cpu.main.f = (wz_byte_t)carry;
    machine->cpu.main.h = 0x91u;
    machine->cpu.main.l = 0x23u;
    machine->cpu.ix = 0x7f80u;
    machine->cpu.iy = 0x7f80u;
    machine->cpu.memptr = 0xbeefu;
    machine->cpu.r = 0u;
    machine->cpu.q = 0u;
    wz_machine_memory_write(machine, 0x8000u, prefix);
    wz_machine_memory_write(machine, 0x8001u, opcode);
    expected = machine->cpu;
    expected.program_counter = 0x8002u;
    expected.r = 2u;
    expected.q = 1u;
    expected.main.f = answer.flags;
    if (inc_dec) {
        wz_word_t* index = index_mode == 0u ? &expected.ix : &expected.iy;
        *index = half == 0u ? (wz_word_t)((*index & 0x00ffu) | ((wz_word_t)answer.value << 8u)) :
                              (wz_word_t)((*index & 0xff00u) | answer.value);
    } else expected.main.a = answer.value;
    if (wz_z80_step(machine) != WZ_RESULT_OK ||
        machine->master_tick != base + 16u ||
        memcmp(&machine->cpu, &expected, sizeof(expected)) != 0 ||
        !check_capture(capture, base, prefix, opcode)) {
        (void)fprintf(stderr,
            "index half arithmetic mismatch mode=%u half=%u op=%u carry=%u "
            "incdec=%u dec=%u A=%02x/%02x F=%02x/%02x IX=%04x/%04x "
            "IY=%04x/%04x PC=%04x/%04x bus=%zu trace=%zu\n",
            index_mode, half, operation, carry, inc_dec, decrement,
            machine->cpu.main.a, expected.main.a, machine->cpu.main.f, expected.main.f,
            machine->cpu.ix, expected.ix, machine->cpu.iy, expected.iy,
            machine->cpu.program_counter, expected.program_counter,
            capture->bus_count, capture->trace_count);
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
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) return 1;
    wz_bus_observer_init(&observer, on_bus, &capture);
    if (wz_machine_set_bus_observer(&machine, &observer) != WZ_RESULT_OK) goto failed;
    wz_trace_sink_init(&sink, on_trace, &capture);
    wz_machine_set_timing_trace(&machine, &sink);
    for (unsigned index_mode = 0u; index_mode < 2u; ++index_mode) {
        for (unsigned half = 0u; half < 2u; ++half) {
            for (unsigned operation = 0u; operation < 8u; ++operation) {
                for (unsigned carry = 0u; carry < 2u; ++carry) {
                    if (!run_case(&machine, &capture, index_mode, half,
                                  operation, carry, false, false)) goto failed;
                    ++cases;
                }
            }
            for (unsigned decrement = 0u; decrement < 2u; ++decrement) {
                for (unsigned carry = 0u; carry < 2u; ++carry) {
                    if (!run_case(&machine, &capture, index_mode, half,
                                  0u, carry, true, decrement != 0u)) goto failed;
                    ++cases;
                }
            }
        }
    }
    wz_machine_destroy(&machine);
    (void)printf("{\"testId\":\"cpu-index-half-arithmetic\",\"status\":\"pass\","
                 "\"cases\":%u,\"aluCases\":64,\"incDecCases\":16}\n", cases);
    return 0;
failed:
    wz_machine_destroy(&machine);
    return 1;
}
