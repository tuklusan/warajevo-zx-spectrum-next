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

enum { MAX_EVENTS = 24 };

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

static bool check_bus_trace(const capture_t* capture,
                            const expected_bus_t* expected,
                            size_t count, wz_byte_t prefix,
                            wz_byte_t opcode)
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
            if (trace_opcode >= 2u ||
                event->address != (wz_word_t)(0x8000u + trace_opcode) ||
                event->value != (trace_opcode == 0u ? prefix : opcode)) return false;
            ++trace_opcode;
        } else if (event->kind == WZ_TRACE_CPU_INSTRUCTION) {
            if (event->program_counter != 0x8000u || event->value != prefix) return false;
            ++trace_instruction;
        }
    }
    return trace_bus == count && trace_opcode == 2u && trace_instruction == 1u;
}

static bool run_case(wz_machine_t* machine, capture_t* capture,
                     wz_byte_t prefix, wz_byte_t opcode)
{
    const wz_master_tick_t base = machine->master_tick;
    const wz_word_t original_index = prefix == 0xddu ? 0xa800u : 0xb900u;
    wz_word_t* expected_index;
    wz_z80_state_t expected;
    expected_bus_t bus[7] = {
        {WZ_BUS_M1_OPCODE_FETCH, base, 0x8000u, prefix, 4u},
        {WZ_BUS_M1_OPCODE_FETCH, base + 8u, 0x8001u, opcode, 4u},
    };
    size_t count = 2u;
    wz_master_tick_t duration = 16u;
    wz_byte_t stack[4] = {0xaau, 0xbbu, 0x34u, 0x12u};
    wz_byte_t absolute[2] = {0x3cu, 0x47u};
    memset(capture, 0, sizeof(*capture));
    machine->cpu.program_counter = 0x8000u;
    machine->cpu.stack_pointer = 0x9002u;
    machine->cpu.main.a = 0xdeu;
    machine->cpu.main.f = 0xd7u;
    machine->cpu.main.b = 0x12u; machine->cpu.main.c = 0x34u;
    machine->cpu.main.d = 0x56u; machine->cpu.main.e = 0x78u;
    machine->cpu.main.h = 0x9au; machine->cpu.main.l = 0xbcu;
    machine->cpu.ix = 0xa800u;
    machine->cpu.iy = 0xb900u;
    machine->cpu.memptr = 0xbeefu;
    machine->cpu.r = 0u;
    machine->cpu.q = 0u;
    wz_machine_memory_write(machine, 0x8000u, prefix);
    wz_machine_memory_write(machine, 0x8001u, opcode);
    wz_machine_memory_write(machine, 0x8002u,
                            opcode == 0x22u || opcode == 0x2au ? 0x00u : 0x23u);
    wz_machine_memory_write(machine, 0x8003u,
                            opcode == 0x22u || opcode == 0x2au ? 0xa0u : 0x56u);
    for (unsigned index = 0u; index < 4u; ++index) {
        wz_machine_memory_write(machine, (wz_word_t)(0x9000u + index), stack[index]);
    }
    wz_machine_memory_write(machine, 0xa000u, absolute[0]);
    wz_machine_memory_write(machine, 0xa001u, absolute[1]);
    expected = machine->cpu;
    expected.program_counter = 0x8002u;
    expected.r = 2u;
    expected_index = prefix == 0xddu ? &expected.ix : &expected.iy;
    switch (opcode) {
    case 0x44u: expected.main.b = (wz_byte_t)(original_index >> 8u); break;
    case 0x45u: expected.main.b = (wz_byte_t)original_index; break;
    case 0x60u:
        *expected_index = (wz_word_t)((original_index & 0x00ffu) |
                                       ((wz_word_t)expected.main.b << 8u));
        break;
    case 0x68u:
        *expected_index = (wz_word_t)((original_index & 0xff00u) |
                                       expected.main.b);
        break;
    case 0x26u:
    case 0x2eu:
        expected.program_counter = 0x8003u;
        *expected_index = opcode == 0x26u ?
            (wz_word_t)((original_index & 0x00ffu) | 0x2300u) :
            (wz_word_t)((original_index & 0xff00u) | 0x23u);
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
            base + 16u, 0x8002u, 0x23u, 3u};
        duration = 22u;
        break;
    case 0x21u:
        expected.program_counter = 0x8004u;
        *expected_index = 0x5623u;
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
            base + 16u, 0x8002u, 0x23u, 3u};
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
            base + 22u, 0x8003u, 0x56u, 3u};
        duration = 28u;
        break;
    case 0x22u:
    case 0x2au:
        expected.program_counter = 0x8004u;
        expected.memptr = 0xa001u;
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
            base + 16u, 0x8002u, 0x00u, 3u};
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
            base + 22u, 0x8003u, 0xa0u, 3u};
        if (opcode == 0x22u) {
            absolute[0] = (wz_byte_t)original_index;
            absolute[1] = (wz_byte_t)(original_index >> 8u);
            bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_WRITE,
                base + 28u, 0xa000u, absolute[0], 3u};
            bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_WRITE,
                base + 34u, 0xa001u, absolute[1], 3u};
        } else {
            *expected_index = 0x473cu;
            bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
                base + 28u, 0xa000u, 0x3cu, 3u};
            bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
                base + 34u, 0xa001u, 0x47u, 3u};
        }
        duration = 40u;
        break;
    case 0x23u: *expected_index = (wz_word_t)(original_index + 1u);
                duration = 20u; break;
    case 0x2bu: *expected_index = (wz_word_t)(original_index - 1u);
                duration = 20u; break;
    case 0xe9u: expected.program_counter = original_index; break;
    case 0xf9u: expected.stack_pointer = original_index;
                duration = 20u; break;
    case 0xe1u:
        *expected_index = 0x1234u;
        expected.stack_pointer = 0x9004u;
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
            base + 16u, 0x9002u, 0x34u, 3u};
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
            base + 22u, 0x9003u, 0x12u, 3u};
        duration = 28u;
        break;
    case 0xe5u:
        expected.stack_pointer = 0x9000u;
        stack[1] = (wz_byte_t)(original_index >> 8u);
        stack[0] = (wz_byte_t)original_index;
        bus[count++] = (expected_bus_t){WZ_BUS_INTERNAL,
            base + 16u, 0x9002u, 0u, 1u};
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_WRITE,
            base + 18u, 0x9001u, stack[1], 3u};
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_WRITE,
            base + 24u, 0x9000u, stack[0], 3u};
        duration = 30u;
        break;
    case 0xe3u:
        *expected_index = 0x1234u;
        expected.memptr = 0x1234u;
        stack[3] = (wz_byte_t)(original_index >> 8u);
        stack[2] = (wz_byte_t)original_index;
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
            base + 16u, 0x9002u, 0x34u, 3u};
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
            base + 22u, 0x9003u, 0x12u, 3u};
        bus[count++] = (expected_bus_t){WZ_BUS_INTERNAL,
            base + 28u, 0x9002u, 0u, 3u};
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_WRITE,
            base + 30u, 0x9003u, stack[3], 3u};
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_WRITE,
            base + 36u, 0x9002u, stack[2], 3u};
        duration = 46u;
        break;
    default: return false;
    }
    if (wz_z80_step(machine) != WZ_RESULT_OK ||
        machine->master_tick != base + duration ||
        memcmp(&machine->cpu, &expected, sizeof(expected)) != 0 ||
        wz_machine_memory_read(machine, 0x9000u) != stack[0] ||
        wz_machine_memory_read(machine, 0x9001u) != stack[1] ||
        wz_machine_memory_read(machine, 0x9002u) != stack[2] ||
        wz_machine_memory_read(machine, 0x9003u) != stack[3] ||
        wz_machine_memory_read(machine, 0xa000u) != absolute[0] ||
        wz_machine_memory_read(machine, 0xa001u) != absolute[1] ||
        !check_bus_trace(capture, bus, count, prefix, opcode)) {
        (void)fprintf(stderr,
            "indexed state mismatch prefix=%02x opcode=%02x IX/IY=%04x "
            "PC=%04x/%04x SP=%04x/%04x memptr=%04x/%04x "
            "ticks=%llu/%llu bus=%zu/%zu trace=%zu\n",
            prefix, opcode, original_index,
            machine->cpu.program_counter, expected.program_counter,
            machine->cpu.stack_pointer, expected.stack_pointer,
            machine->cpu.memptr, expected.memptr,
            (unsigned long long)(machine->master_tick - base),
            (unsigned long long)duration, capture->bus_count, count,
            capture->trace_count);
        return false;
    }
    return true;
}

int main(void)
{
    static const wz_byte_t opcodes[] = {
        0x44u, 0x45u, 0x60u, 0x68u, 0x26u, 0x2eu,
        0x21u, 0x22u, 0x2au, 0x23u, 0x2bu,
        0xe9u, 0xf9u, 0xe1u, 0xe5u, 0xe3u,
    };
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
    for (unsigned prefix = 0u; prefix < 2u; ++prefix) {
        for (size_t op = 0u; op < sizeof(opcodes) / sizeof(opcodes[0]); ++op) {
            if (!run_case(&machine, &capture,
                          prefix == 0u ? 0xddu : 0xfdu,
                          opcodes[op])) goto failed;
            ++cases;
        }
    }
    wz_machine_destroy(&machine);
    (void)printf("{\"testId\":\"cpu-indexed-state\",\"status\":\"pass\","
                 "\"cases\":%u,\"ixCases\":16,\"iyCases\":16}\n", cases);
    return 0;
failed:
    wz_machine_destroy(&machine);
    return 1;
}
