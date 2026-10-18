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
typedef enum { REGISTER, IMMEDIATE, PAIR_IMMEDIATE, INDIRECT_A, ABSOLUTE } kind_t;

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

static wz_byte_t* reg(wz_z80_register_bank_t* bank, unsigned index)
{
    switch (index) {
    case 0u: return &bank->b;
    case 1u: return &bank->c;
    case 2u: return &bank->d;
    case 3u: return &bank->e;
    case 4u: return &bank->h;
    case 5u: return &bank->l;
    case 7u: return &bank->a;
    default: return 0;
    }
}

static void set_pair(wz_z80_state_t* cpu, unsigned pair, wz_word_t value)
{
    switch (pair) {
    case 0u: cpu->main.b = (wz_byte_t)(value >> 8u);
             cpu->main.c = (wz_byte_t)value; break;
    case 1u: cpu->main.d = (wz_byte_t)(value >> 8u);
             cpu->main.e = (wz_byte_t)value; break;
    case 2u: cpu->main.h = (wz_byte_t)(value >> 8u);
             cpu->main.l = (wz_byte_t)value; break;
    default: cpu->stack_pointer = value; break;
    }
}

static bool check_bus_trace(const capture_t* capture,
                            const expected_bus_t* expected,
                            size_t count, wz_byte_t opcode)
{
    size_t bus_events = 0u;
    size_t opcode_events = 0u;
    size_t instruction_events = 0u;
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
            if (bus_events >= count ||
                event->master_tick != expected[bus_events].tick ||
                event->address != expected[bus_events].address ||
                event->value != expected[bus_events].value ||
                event->cycle != (wz_byte_t)expected[bus_events].cycle ||
                event->t_states != expected[bus_events].duration) return false;
            ++bus_events;
        } else if (event->kind == WZ_TRACE_CPU_OPCODE_BYTE) {
            if (opcode_events != 0u ||
                event->address != 0x8000u || event->value != opcode) return false;
            ++opcode_events;
        } else if (event->kind == WZ_TRACE_CPU_INSTRUCTION) {
            if (event->program_counter != 0x8000u || event->value != opcode) return false;
            ++instruction_events;
        }
    }
    return bus_events == count && opcode_events == 1u && instruction_events == 1u;
}

static bool run_case(wz_machine_t* machine, capture_t* capture,
                     kind_t kind, wz_byte_t opcode)
{
    const wz_master_tick_t base = machine->master_tick;
    wz_z80_state_t expected;
    expected_bus_t bus[6] = {{WZ_BUS_M1_OPCODE_FETCH, base, 0x8000u, opcode, 4u}};
    size_t count = 1u;
    wz_master_tick_t duration = 8u;
    wz_byte_t memory_b000 = 0x3cu;
    wz_byte_t memory_9000 = 0x3cu;
    wz_byte_t memory_a000 = 0x3cu;
    wz_byte_t memory_b800 = 0x3cu;
    wz_byte_t memory_b801 = 0x47u;
    memset(capture, 0, sizeof(*capture));
    machine->cpu.program_counter = 0x8000u;
    machine->cpu.main.a = 0x5au;
    machine->cpu.main.f = 0xd7u;
    machine->cpu.main.b = 0x90u; machine->cpu.main.c = 0x00u;
    machine->cpu.main.d = 0xa0u; machine->cpu.main.e = 0x00u;
    machine->cpu.main.h = 0xb0u; machine->cpu.main.l = 0x00u;
    machine->cpu.stack_pointer = 0xd000u;
    machine->cpu.memptr = 0xbeefu;
    machine->cpu.r = 0u;
    machine->cpu.q = 0u;
    wz_machine_memory_write(machine, 0x8000u, opcode);
    wz_machine_memory_write(machine, 0x8001u,
                            kind == ABSOLUTE ? 0x00u : 0x23u);
    wz_machine_memory_write(machine, 0x8002u,
                            kind == ABSOLUTE ? 0xb8u : 0xa1u);
    wz_machine_memory_write(machine, 0x9000u, memory_9000);
    wz_machine_memory_write(machine, 0xa000u, memory_a000);
    wz_machine_memory_write(machine, 0xb000u, memory_b000);
    wz_machine_memory_write(machine, 0xb800u, memory_b800);
    wz_machine_memory_write(machine, 0xb801u, memory_b801);
    expected = machine->cpu;
    expected.program_counter = 0x8001u;
    expected.r = 1u;
    if (kind == REGISTER) {
        const unsigned source = opcode & 7u;
        const unsigned target = (opcode >> 3u) & 7u;
        const wz_byte_t value = source == 6u ? memory_b000 :
            *reg(&expected.main, source);
        if (source == 6u) {
            bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
                base + 8u, 0xb000u, value, 3u};
        }
        if (target == 6u) {
            memory_b000 = value;
            bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_WRITE,
                base + 8u, 0xb000u, value, 3u};
        } else {
            *reg(&expected.main, target) = value;
        }
        if (source == 6u || target == 6u) duration = 14u;
    } else if (kind == IMMEDIATE) {
        const unsigned target = (opcode >> 3u) & 7u;
        expected.program_counter = 0x8002u;
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
            base + 8u, 0x8001u, 0x23u, 3u};
        if (target == 6u) {
            memory_b000 = 0x23u;
            bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_WRITE,
                base + 14u, 0xb000u, 0x23u, 3u};
            duration = 20u;
        } else {
            *reg(&expected.main, target) = 0x23u;
            duration = 14u;
        }
    } else if (kind == PAIR_IMMEDIATE) {
        expected.program_counter = 0x8003u;
        set_pair(&expected, opcode >> 4u, 0xa123u);
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
            base + 8u, 0x8001u, 0x23u, 3u};
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
            base + 14u, 0x8002u, 0xa1u, 3u};
        duration = 20u;
    } else if (kind == INDIRECT_A) {
        const bool de = (opcode & 0x10u) != 0u;
        const bool read = (opcode & 0x08u) != 0u;
        const wz_word_t address = de ? 0xa000u : 0x9000u;
        if (read) {
            expected.main.a = 0x3cu;
            expected.memptr = (wz_word_t)(address + 1u);
            bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
                base + 8u, address, 0x3cu, 3u};
        } else {
            expected.memptr = 0x5a01u;
            if (de) memory_a000 = 0x5au;
            else memory_9000 = 0x5au;
            bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_WRITE,
                base + 8u, address, 0x5au, 3u};
        }
        duration = 14u;
    } else {
        const bool store_hl = opcode == 0x22u;
        const bool load_hl = opcode == 0x2au;
        const bool store_a = opcode == 0x32u;
        expected.program_counter = 0x8003u;
        expected.memptr = store_a ? 0x5a01u : 0xb801u;
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
            base + 8u, 0x8001u, 0x00u, 3u};
        bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
            base + 14u, 0x8002u, 0xb8u, 3u};
        if (store_hl) {
            memory_b800 = 0x00u;
            memory_b801 = 0xb0u;
            bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_WRITE,
                base + 20u, 0xb800u, 0x00u, 3u};
            bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_WRITE,
                base + 26u, 0xb801u, 0xb0u, 3u};
            duration = 32u;
        } else if (load_hl) {
            expected.main.h = 0x47u; expected.main.l = 0x3cu;
            bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
                base + 20u, 0xb800u, 0x3cu, 3u};
            bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
                base + 26u, 0xb801u, 0x47u, 3u};
            duration = 32u;
        } else if (store_a) {
            memory_b800 = 0x5au;
            bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_WRITE,
                base + 20u, 0xb800u, 0x5au, 3u};
            duration = 26u;
        } else {
            expected.main.a = 0x3cu;
            bus[count++] = (expected_bus_t){WZ_BUS_MEMORY_READ,
                base + 20u, 0xb800u, 0x3cu, 3u};
            duration = 26u;
        }
    }
    if (wz_z80_step(machine) != WZ_RESULT_OK ||
        machine->master_tick != base + duration ||
        memcmp(&machine->cpu, &expected, sizeof(expected)) != 0 ||
        wz_machine_memory_read(machine, 0x9000u) != memory_9000 ||
        wz_machine_memory_read(machine, 0xa000u) != memory_a000 ||
        wz_machine_memory_read(machine, 0xb000u) != memory_b000 ||
        wz_machine_memory_read(machine, 0xb800u) != memory_b800 ||
        wz_machine_memory_read(machine, 0xb801u) != memory_b801 ||
        !check_bus_trace(capture, bus, count, opcode)) {
        (void)fprintf(stderr,
            "primary load mismatch kind=%u opcode=%02x PC=%04x/%04x "
            "A=%02x/%02x F=%02x/%02x R=%02x/%02x ticks=%llu/%llu "
            "bus=%zu/%zu trace=%zu\n",
            (unsigned)kind, opcode, machine->cpu.program_counter,
            expected.program_counter, machine->cpu.main.a, expected.main.a,
            machine->cpu.main.f, expected.main.f,
            machine->cpu.r, expected.r,
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
    for (unsigned target = 0u; target < 8u; ++target) {
        for (unsigned source = 0u; source < 8u; ++source) {
            if (source == 6u && target == 6u) continue;
            if (!run_case(&machine, &capture, REGISTER,
                          (wz_byte_t)(0x40u | (target << 3u) | source))) goto failed;
            ++cases;
        }
        if (!run_case(&machine, &capture, IMMEDIATE,
                      (wz_byte_t)(0x06u | (target << 3u)))) goto failed;
        ++cases;
    }
    for (unsigned pair = 0u; pair < 4u; ++pair) {
        if (!run_case(&machine, &capture, PAIR_IMMEDIATE,
                      (wz_byte_t)(0x01u | (pair << 4u)))) goto failed;
        ++cases;
    }
    const wz_byte_t indirect[] = {0x02u, 0x12u, 0x0au, 0x1au};
    const wz_byte_t absolute[] = {0x22u, 0x2au, 0x32u, 0x3au};
    for (unsigned index = 0u; index < 4u; ++index) {
        if (!run_case(&machine, &capture, INDIRECT_A, indirect[index]) ||
            !run_case(&machine, &capture, ABSOLUTE, absolute[index])) goto failed;
        cases += 2u;
    }
    wz_machine_destroy(&machine);
    (void)printf("{\"testId\":\"cpu-primary-load\",\"status\":\"pass\","
                 "\"cases\":%u,\"registerCases\":63,\"immediateCases\":8,"
                 "\"pairCases\":4,\"indirectCases\":4,\"absoluteCases\":4}\n", cases);
    return 0;
failed:
    wz_machine_destroy(&machine);
    return 1;
}
