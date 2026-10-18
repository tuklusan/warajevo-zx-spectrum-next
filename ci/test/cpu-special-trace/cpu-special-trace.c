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

typedef struct { wz_byte_t a, f; } answer_t;

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

static bool parity(unsigned value)
{
    unsigned bits = 0u;
    for (unsigned bit = 0u; bit < 8u; ++bit) bits += (value >> bit) & 1u;
    return (bits & 1u) == 0u;
}

static answer_t reference(wz_byte_t opcode, unsigned a, unsigned f,
                          unsigned prior_q)
{
    answer_t answer = {(wz_byte_t)a, (wz_byte_t)f};
    unsigned result = a, carry = 0u, flags = f;
    switch (opcode) {
    case 0x07u: result = ((a << 1u) | (a >> 7u)) & 255u;
                carry = a >> 7u; break;
    case 0x0fu: result = (a >> 1u) | ((a & 1u) << 7u);
                carry = a & 1u; break;
    case 0x17u: result = ((a << 1u) | (f & C)) & 255u;
                carry = a >> 7u; break;
    case 0x1fu: result = (a >> 1u) | ((f & C) << 7u);
                carry = a & 1u; break;
    case 0x27u: {
        unsigned adjustment = 0u;
        if ((f & H) != 0u || (a & 15u) > 9u) adjustment += 6u;
        if ((f & C) != 0u || a > 0x99u) { adjustment += 0x60u; carry = C; }
        result = (f & N) != 0u ? (a - adjustment) & 255u :
                                (a + adjustment) & 255u;
        flags = (result & (S | Y | X)) | (result == 0u ? Z : 0u) |
                (parity(result) ? PV : 0u) | (f & N) | carry |
                (((a ^ result) & 16u) != 0u ? H : 0u);
        break;
    }
    case 0x2fu:
        result = a ^ 255u;
        flags = (f & (S | Z | PV | C)) | H | N | (result & (Y | X));
        break;
    case 0x37u:
        flags = (f & (S | Z | PV)) | C |
                ((a | (prior_q != 0u ? 0u : f)) & (Y | X));
        break;
    case 0x3fu:
        flags = (f & (S | Z | PV)) |
                ((a | (prior_q != 0u ? 0u : f)) & (Y | X)) |
                ((f & C) != 0u ? H : C);
        break;
    default: break;
    }
    if (opcode <= 0x1fu) {
        flags = (f & (S | Z | PV)) | (result & (Y | X)) | carry;
    }
    answer.a = (wz_byte_t)result;
    answer.f = (wz_byte_t)flags;
    return answer;
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

static bool run_case(wz_machine_t* machine, capture_t* capture,
                     wz_byte_t opcode, unsigned a, unsigned f,
                     unsigned prior_q)
{
    const answer_t answer = reference(opcode, a, f, prior_q);
    const wz_master_tick_t base = machine->master_tick;
    wz_z80_state_t expected;
    memset(capture, 0, sizeof(*capture));
    machine->cpu.program_counter = 0x8000u;
    machine->cpu.main.a = (wz_byte_t)a;
    machine->cpu.main.f = (wz_byte_t)f;
    machine->cpu.r = 0u;
    machine->cpu.q = (wz_byte_t)prior_q;
    machine->cpu.memptr = 0xbeefu;
    wz_machine_memory_write(machine, 0x8000u, opcode);
    expected = machine->cpu;
    expected.program_counter = 0x8001u;
    expected.r = 1u;
    expected.q = 1u;
    expected.main.a = answer.a;
    expected.main.f = answer.f;
    if (wz_z80_step(machine) != WZ_RESULT_OK ||
        machine->master_tick != base + 8u ||
        memcmp(&machine->cpu, &expected, sizeof(expected)) != 0 ||
        !check_capture(capture, base, opcode)) {
        (void)fprintf(stderr,
            "special trace mismatch op=%02x A=%02x F=%02x priorQ=%u "
            "actual A/F=%02x/%02x expected=%02x/%02x PC=%04x/%04x "
            "bus=%zu trace=%zu\n",
            opcode, a, f, prior_q,
            machine->cpu.main.a, machine->cpu.main.f,
            expected.main.a, expected.main.f,
            machine->cpu.program_counter, expected.program_counter,
            capture->bus_count, capture->trace_count);
        return false;
    }
    return true;
}

int main(void)
{
    static const wz_byte_t opcodes[] = {
        0x07u, 0x0fu, 0x17u, 0x1fu, 0x27u, 0x2fu, 0x37u, 0x3fu
    };
    static const unsigned values[] = {0x00u, 0x55u, 0x9au, 0xffu};
    static const unsigned flags[] = {0x00u, 0x01u, 0x12u, 0xffu};
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
    for (size_t op = 0u; op < 8u; ++op) {
        for (size_t value = 0u; value < 4u; ++value) {
            for (size_t flag = 0u; flag < 4u; ++flag) {
                for (unsigned prior_q = 0u; prior_q < 2u; ++prior_q) {
                    if (!run_case(&machine, &capture, opcodes[op],
                                  values[value], flags[flag], prior_q)) goto failed;
                    ++cases;
                }
            }
        }
    }
    wz_machine_destroy(&machine);
    (void)printf("{\"testId\":\"cpu-special-trace\",\"status\":\"pass\","
                 "\"cases\":%u,\"opcodes\":8,\"qStates\":2}\n", cases);
    return 0;
failed:
    wz_machine_destroy(&machine);
    return 1;
}
