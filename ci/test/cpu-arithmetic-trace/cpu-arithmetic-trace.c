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

typedef struct { wz_byte_t a, f; } alu_result_t;

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

static int signed_byte(unsigned value)
{
    return value < 128u ? (int)value : (int)value - 256;
}

static bool even_parity(unsigned value)
{
    unsigned bits = 0u;
    for (unsigned index = 0u; index < 8u; ++index) bits += (value >> index) & 1u;
    return (bits & 1u) == 0u;
}

static unsigned result_flags(unsigned result)
{
    unsigned flags = result & (FLAG_S | FLAG_Y | FLAG_X);
    if (result == 0u) flags |= FLAG_Z;
    return flags;
}

static alu_result_t alu_reference(unsigned operation, unsigned left,
                                  unsigned operand, unsigned carry_in)
{
    alu_result_t expected = {(wz_byte_t)left, 0u};
    unsigned flags;
    unsigned result;
    const unsigned carry = operation == 1u || operation == 3u ? carry_in : 0u;
    if (operation <= 1u) {
        const unsigned wide = left + operand + carry;
        const int signed_sum = signed_byte(left) + signed_byte(operand) + (int)carry;
        result = wide & 255u;
        flags = result_flags(result);
        if ((left & 15u) + (operand & 15u) + carry > 15u) flags |= FLAG_H;
        if (signed_sum < -128 || signed_sum > 127) flags |= FLAG_PV;
        if (wide > 255u) flags |= FLAG_C;
        expected.a = (wz_byte_t)result;
    } else if (operation <= 3u || operation == 7u) {
        const int difference = (int)left - (int)operand - (int)carry;
        const int signed_difference = signed_byte(left) - signed_byte(operand) - (int)carry;
        result = (unsigned)difference & 255u;
        flags = result_flags(result) | FLAG_N;
        if (operation == 7u) flags = (flags & ~(FLAG_Y | FLAG_X)) |
                                     (operand & (FLAG_Y | FLAG_X));
        else expected.a = (wz_byte_t)result;
        if ((left & 15u) < (operand & 15u) + carry) flags |= FLAG_H;
        if (signed_difference < -128 || signed_difference > 127) flags |= FLAG_PV;
        if (difference < 0) flags |= FLAG_C;
    } else {
        result = operation == 4u ? left & operand :
                 operation == 5u ? left ^ operand : left | operand;
        flags = result_flags(result);
        if (even_parity(result)) flags |= FLAG_PV;
        if (operation == 4u) flags |= FLAG_H;
        expected.a = (wz_byte_t)result;
    }
    expected.f = (wz_byte_t)flags;
    return expected;
}

static alu_result_t inc_dec_reference(bool decrement, unsigned value,
                                      unsigned carry)
{
    const unsigned result = decrement ? (value - 1u) & 255u :
                                        (value + 1u) & 255u;
    unsigned flags = result_flags(result) | carry;
    if (decrement) {
        flags |= FLAG_N;
        if ((value & 15u) == 0u) flags |= FLAG_H;
        if (value == 128u) flags |= FLAG_PV;
    } else {
        if ((value & 15u) == 15u) flags |= FLAG_H;
        if (value == 127u) flags |= FLAG_PV;
    }
    return (alu_result_t){(wz_byte_t)result, (wz_byte_t)flags};
}

static bool check_bus_trace(const capture_t* capture,
                            const expected_bus_t* expected,
                            size_t count, wz_byte_t prefix,
                            wz_byte_t opcode, bool indexed)
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
            if (trace_opcode >= (indexed ? 2u : 1u) ||
                event->address != (wz_word_t)(0x8000u + trace_opcode) ||
                event->value != (trace_opcode == 0u ? prefix : opcode)) return false;
            ++trace_opcode;
        } else if (event->kind == WZ_TRACE_CPU_INSTRUCTION) {
            if (event->program_counter != 0x8000u || event->value != prefix) return false;
            ++trace_instruction;
        }
    }
    return trace_bus == count && trace_opcode == (indexed ? 2u : 1u) &&
           trace_instruction == 1u;
}

/* mode: (HL), DD indexed, FD indexed, immediate. */
static bool run_case(wz_machine_t* machine, capture_t* capture,
                     unsigned mode, unsigned operation, unsigned carry,
                     bool inc_dec, bool decrement)
{
    static const wz_byte_t immediate_opcodes[8] =
        {0xc6u, 0xceu, 0xd6u, 0xdeu, 0xe6u, 0xeeu, 0xf6u, 0xfeu};
    const bool indexed = mode == 1u || mode == 2u;
    const bool immediate = mode == 3u;
    const wz_byte_t prefix = indexed ? (mode == 1u ? 0xddu : 0xfdu) :
                             (wz_byte_t)(inc_dec ? (decrement ? 0x35u : 0x34u) :
                             immediate ? immediate_opcodes[operation] :
                             0x86u | (operation << 3u));
    const wz_byte_t opcode = (wz_byte_t)(inc_dec ? (decrement ? 0x35u : 0x34u) :
        0x86u | (operation << 3u));
    const wz_word_t address = indexed ? 0x9001u : 0x9000u;
    const wz_byte_t displacement = mode == 2u ? 0xffu : 0x01u;
    const wz_master_tick_t base = machine->master_tick;
    const alu_result_t result = inc_dec ?
        inc_dec_reference(decrement, 0x7fu, carry) :
        alu_reference(operation, 0x81u, 0x7fu, carry);
    wz_z80_state_t expected;
    expected_bus_t bus[7] = {{WZ_BUS_M1_OPCODE_FETCH,
                               base, 0x8000u, prefix, 4u}};
    size_t count = 1u;
    wz_master_tick_t duration;
    wz_byte_t expected_memory = 0x7fu;
    memset(capture, 0, sizeof(*capture));
    machine->cpu.program_counter = 0x8000u;
    machine->cpu.main.a = 0x81u;
    machine->cpu.main.f = (wz_byte_t)carry;
    machine->cpu.main.b = 0x12u; machine->cpu.main.c = 0x34u;
    machine->cpu.main.d = 0x56u; machine->cpu.main.e = 0x78u;
    machine->cpu.main.h = 0x90u; machine->cpu.main.l = 0x00u;
    machine->cpu.ix = 0x9000u;
    machine->cpu.iy = 0x9002u;
    machine->cpu.memptr = 0xbeefu;
    machine->cpu.r = 0u;
    machine->cpu.q = 0u;
    wz_machine_memory_write(machine, 0x8000u, prefix);
    wz_machine_memory_write(machine, 0x8001u,
        indexed ? opcode : 0x7fu);
    wz_machine_memory_write(machine, 0x8002u, displacement);
    wz_machine_memory_write(machine, address, 0x7fu);
    expected = machine->cpu;
    expected.program_counter = indexed ? 0x8003u :
                               immediate ? 0x8002u : 0x8001u;
    expected.r = indexed ? 2u : 1u;
    expected.main.f = result.f;
    expected.q = 1u;
    if (!inc_dec) expected.main.a = result.a;
    else expected_memory = result.a;
    if (indexed) {
        expected.memptr = address;
        bus[count++] = (expected_bus_t){WZ_BUS_M1_OPCODE_FETCH,
            base + 8u, 0x8001u, opcode, 4u};
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
            base + 16u, 0x8002u, displacement, 3u};
        bus[count++] = (expected_bus_t){WZ_BUS_INTERNAL,
            base + 22u, address, 0u, 5u};
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
            base + 32u, address, 0x7fu, 3u};
        duration = inc_dec ? 46u : 38u;
        if (inc_dec) {
            bus[count++] = (expected_bus_t){WZ_BUS_INTERNAL,
                base + 38u, address, 0u, 1u};
            bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_WRITE,
                base + 40u, address, expected_memory, 3u};
        }
    } else if (inc_dec) {
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
            base + 8u, address, 0x7fu, 3u};
        bus[count++] = (expected_bus_t){WZ_BUS_INTERNAL,
            base + 14u, address, 0u, 1u};
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_WRITE,
            base + 16u, address, expected_memory, 3u};
        duration = 22u;
    } else {
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
            base + 8u, immediate ? 0x8001u : address, 0x7fu, 3u};
        duration = 14u;
    }
    if (wz_z80_step(machine) != WZ_RESULT_OK ||
        machine->master_tick != base + duration ||
        memcmp(&machine->cpu, &expected, sizeof(expected)) != 0 ||
        wz_machine_memory_read(machine, address) != expected_memory ||
        !check_bus_trace(capture, bus, count, prefix, opcode, indexed)) {
        (void)fprintf(stderr,
            "arithmetic trace mismatch mode=%u operation=%u carry=%u incdec=%u dec=%u "
            "A=%02x/%02x F=%02x/%02x PC=%04x/%04x ticks=%llu/%llu "
            "bus=%zu/%zu trace=%zu\n",
            mode, operation, carry, inc_dec, decrement,
            machine->cpu.main.a, expected.main.a,
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
    for (unsigned mode = 0u; mode < 4u; ++mode) {
        for (unsigned operation = 0u; operation < 8u; ++operation) {
            for (unsigned carry = 0u; carry < 2u; ++carry) {
                if (!run_case(&machine, &capture, mode, operation,
                              carry, false, false)) goto failed;
                ++cases;
            }
        }
        if (mode == 3u) continue;
        for (unsigned decrement = 0u; decrement < 2u; ++decrement) {
            for (unsigned carry = 0u; carry < 2u; ++carry) {
                if (!run_case(&machine, &capture, mode, 0u, carry,
                              true, decrement != 0u)) goto failed;
                ++cases;
            }
        }
    }
    wz_machine_destroy(&machine);
    (void)printf("{\"testId\":\"cpu-arithmetic-trace\",\"status\":\"pass\","
                 "\"cases\":%u,\"aluCases\":64,\"incDecCases\":12}\n", cases);
    return 0;
failed:
    wz_machine_destroy(&machine);
    return 1;
}
