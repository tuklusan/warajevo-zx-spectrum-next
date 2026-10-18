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

enum { MAX_EVENTS = 20 };

typedef struct {
    wz_bus_request_t bus[MAX_EVENTS];
    wz_trace_event_t trace[MAX_EVENTS];
    size_t bus_count, trace_count;
    bool overflow;
} capture_t;

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

static bool is_unused_ed(unsigned opcode)
{
    const bool middle = opcode >= 0x40u && opcode <= 0x7fu &&
        opcode != 0x77u && opcode != 0x7fu;
    const bool block = opcode >= 0xa0u && opcode <= 0xbbu &&
        (opcode & 0x04u) == 0u;
    return !middle && !block;
}

static bool check_capture(const capture_t* capture,
                          wz_master_tick_t base, const wz_byte_t* bytes,
                          size_t count)
{
    size_t bus_events = 0u, opcode_events = 0u, instruction_events = 0u;
    if (capture->overflow || capture->bus_count != count) return false;
    for (size_t index = 0u; index < count; ++index) {
        const wz_bus_request_t* event = &capture->bus[index];
        if (event->cycle != WZ_BUS_M1_OPCODE_FETCH ||
            event->master_tick != base + 8u * index ||
            event->address != 0x8000u + index ||
            event->value != bytes[index] || event->t_states != 4u ||
            event->contention_delay != 0u) return false;
    }
    for (size_t index = 0u; index < capture->trace_count; ++index) {
        const wz_trace_event_t* event = &capture->trace[index];
        if (index != 0u && event->sequence <= capture->trace[index - 1u].sequence) return false;
        if (event->kind == WZ_TRACE_CPU_BUS) {
            if (bus_events >= count ||
                event->master_tick != base + 8u * bus_events ||
                event->address != 0x8000u + bus_events ||
                event->value != bytes[bus_events] ||
                event->cycle != (wz_byte_t)WZ_BUS_M1_OPCODE_FETCH ||
                event->t_states != 4u) return false;
            ++bus_events;
        } else if (event->kind == WZ_TRACE_CPU_OPCODE_BYTE) {
            if (opcode_events >= count ||
                event->address != 0x8000u + opcode_events ||
                event->value != bytes[opcode_events]) return false;
            ++opcode_events;
        } else if (event->kind == WZ_TRACE_CPU_INSTRUCTION) {
            if (event->program_counter != 0x8000u ||
                event->value != bytes[0]) return false;
            ++instruction_events;
        }
    }
    return bus_events == count && opcode_events == count &&
           instruction_events == 1u;
}

/* mode: ED, DD ED, FD ED, DD FD ED. */
static bool run_case(wz_machine_t* machine, capture_t* capture,
                     unsigned mode, wz_byte_t opcode)
{
    wz_byte_t bytes[4] = {0};
    size_t count = 0u;
    const wz_master_tick_t base = machine->master_tick;
    wz_z80_state_t expected;
    if (mode == 1u || mode == 3u) bytes[count++] = 0xddu;
    if (mode == 2u || mode == 3u) bytes[count++] = 0xfdu;
    bytes[count++] = 0xedu;
    bytes[count++] = opcode;
    memset(capture, 0, sizeof(*capture));
    machine->cpu.program_counter = 0x8000u;
    machine->cpu.main.a = 0x81u;
    machine->cpu.main.f = 0xa5u;
    machine->cpu.ix = 0x1234u;
    machine->cpu.iy = 0x5678u;
    machine->cpu.memptr = 0xbeefu;
    machine->cpu.r = 0u;
    machine->cpu.q = 0u;
    for (size_t index = 0u; index < count; ++index) {
        wz_machine_memory_write(machine, (wz_word_t)(0x8000u + index), bytes[index]);
    }
    expected = machine->cpu;
    expected.program_counter = (wz_word_t)(0x8000u + count);
    expected.r = (wz_byte_t)count;
    if (wz_z80_step(machine) != WZ_RESULT_OK ||
        machine->master_tick != base + 8u * count ||
        memcmp(&machine->cpu, &expected, sizeof(expected)) != 0 ||
        !check_capture(capture, base, bytes, count)) {
        (void)fprintf(stderr,
            "ED NOP mismatch mode=%u opcode=%02x PC=%04x/%04x "
            "R=%u/%u ticks=%llu/%llu bus=%zu/%zu trace=%zu\n",
            mode, opcode, machine->cpu.program_counter,
            expected.program_counter, machine->cpu.r, expected.r,
            (unsigned long long)(machine->master_tick - base),
            (unsigned long long)(8u * count), capture->bus_count,
            count, capture->trace_count);
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
    unsigned cases = 0u, opcodes = 0u;
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) return 1;
    wz_bus_observer_init(&observer, on_bus, &capture);
    if (wz_machine_set_bus_observer(&machine, &observer) != WZ_RESULT_OK) goto failed;
    wz_trace_sink_init(&sink, on_trace, &capture);
    wz_machine_set_timing_trace(&machine, &sink);
    for (unsigned opcode = 0u; opcode < 256u; ++opcode) {
        if (!is_unused_ed(opcode)) continue;
        ++opcodes;
        for (unsigned mode = 0u; mode < 4u; ++mode) {
            if (!run_case(&machine, &capture, mode, (wz_byte_t)opcode)) goto failed;
            ++cases;
        }
    }
    wz_machine_destroy(&machine);
    if (opcodes != 178u || cases != 712u) return 1;
    (void)printf("{\"testId\":\"cpu-ed-undocumented-nop\",\"status\":\"pass\","
                 "\"cases\":%u,\"opcodes\":%u,\"prefixModes\":4}\n",
                 cases, opcodes);
    return 0;
failed:
    wz_machine_destroy(&machine);
    return 1;
}
