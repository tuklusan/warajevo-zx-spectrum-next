/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdbool.h>
#include <stdio.h>

#include "core/wz_machine.h"

typedef struct {
    wz_bus_request_t bus[8];
    wz_trace_event_t trace[32];
    size_t bus_count;
    size_t trace_count;
    bool overflow;
} capture_t;

typedef struct {
    wz_bus_cycle_t cycle;
    wz_master_tick_t tick;
    wz_word_t address;
    wz_byte_t value;
    wz_byte_t states;
} expected_bus_t;

typedef struct {
    const char* name;
    wz_byte_t program[5];
    size_t length;
    expected_bus_t bus[5];
    size_t bus_count;
    wz_word_t pc;
    wz_word_t ix;
    wz_word_t iy;
    wz_byte_t a;
    wz_byte_t r;
    wz_master_tick_t tick;
} prefix_case_t;

static void record_bus(const wz_bus_request_t* request, void* context)
{
    capture_t* capture = (capture_t*)context;
    if (capture->bus_count >= 8u) {
        capture->overflow = true;
        return;
    }
    capture->bus[capture->bus_count++] = *request;
}

static void record_trace(const wz_trace_event_t* event, void* context)
{
    capture_t* capture = (capture_t*)context;
    if (capture->trace_count >= 32u) {
        capture->overflow = true;
        return;
    }
    capture->trace[capture->trace_count++] = *event;
}

static bool check_decoders(void)
{
    if (wz_z80_primary_opcode_count() != 256u ||
        wz_z80_cb_opcode_count() != 256u ||
        wz_z80_ed_opcode_count() != 256u) return false;
    for (unsigned value = 0u; value < 256u; ++value) {
        const wz_byte_t opcode = (wz_byte_t)value;
        const wz_z80_opcode_decode_t* primary =
            wz_z80_primary_opcode_decode(opcode);
        const wz_z80_cb_opcode_decode_t* cb =
            wz_z80_cb_opcode_decode(opcode);
        const wz_z80_ed_opcode_decode_t ed =
            wz_z80_ed_opcode_decode(opcode);
        if (primary == 0 || primary->opcode != opcode ||
            primary->status > WZ_Z80_OPCODE_ILLEGAL ||
            primary->operation > WZ_Z80_PRIMARY_OP_PREFIX_FD ||
            cb == 0 || cb->opcode != opcode ||
            cb->status > WZ_Z80_OPCODE_ILLEGAL ||
            cb->operation > WZ_Z80_CB_OP_SET ||
            cb->target > 7u || cb->bit > 7u ||
            ed.opcode != opcode || ed.status > WZ_Z80_OPCODE_ILLEGAL ||
            ed.operation > WZ_Z80_ED_OP_OTDR) {
            (void)fprintf(stderr, "decoder mismatch at %02x\n", value);
            return false;
        }
    }
    return true;
}

static bool check_prefix_capture(const prefix_case_t* expected,
                                 const capture_t* capture)
{
    size_t traced_bus = 0u;
    size_t traced_opcode = 0u;
    if (capture->overflow || capture->bus_count != expected->bus_count) return false;
    for (size_t index = 0u; index < expected->bus_count; ++index) {
        const wz_bus_request_t* bus = &capture->bus[index];
        const expected_bus_t* want = &expected->bus[index];
        if (bus->cycle != want->cycle || bus->master_tick != want->tick ||
            bus->address != want->address || bus->value != want->value ||
            bus->t_states != want->states || bus->contention_delay != 0u) {
            return false;
        }
    }
    for (size_t index = 0u; index < capture->trace_count; ++index) {
        const wz_trace_event_t* event = &capture->trace[index];
        if (index != 0u &&
            event->sequence <= capture->trace[index - 1u].sequence) return false;
        if (event->kind == WZ_TRACE_CPU_BUS) {
            if (traced_bus >= expected->bus_count ||
                event->master_tick != expected->bus[traced_bus].tick ||
                event->address != expected->bus[traced_bus].address ||
                event->value != expected->bus[traced_bus].value ||
                event->cycle != (wz_byte_t)expected->bus[traced_bus].cycle ||
                event->t_states != expected->bus[traced_bus].states) return false;
            ++traced_bus;
        } else if (event->kind == WZ_TRACE_CPU_OPCODE_BYTE) {
            ++traced_opcode;
        }
    }
    return traced_bus == expected->bus_count &&
           traced_opcode == expected->r;
}

static bool run_prefix_case(const prefix_case_t* expected)
{
    wz_machine_t machine;
    wz_bus_observer_t observer;
    wz_trace_sink_t sink;
    capture_t capture = {0};
    bool initialized = false;
    bool passed = false;
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        (void)fprintf(stderr, "%s: init failed\n", expected->name);
        return false;
    }
    initialized = true;
    for (size_t index = 0u; index < expected->length; ++index) {
        wz_machine_memory_write(&machine, (wz_word_t)(0x8000u + index),
                                expected->program[index]);
    }
    machine.cpu.program_counter = 0x8000u;
    machine.cpu.ix = 0x1111u;
    machine.cpu.iy = 0x2222u;
    machine.cpu.main.a = 0x01u;
    wz_bus_observer_init(&observer, record_bus, &capture);
    if (wz_machine_set_bus_observer(&machine, &observer) != WZ_RESULT_OK) {
        goto cleanup;
    }
    wz_trace_sink_init(&sink, record_trace, &capture);
    wz_machine_set_timing_trace(&machine, &sink);
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != expected->pc ||
        machine.cpu.ix != expected->ix || machine.cpu.iy != expected->iy ||
        machine.cpu.main.a != expected->a || machine.cpu.r != expected->r ||
        machine.master_tick != expected->tick ||
        !check_prefix_capture(expected, &capture)) {
        (void)fprintf(stderr, "%s: state or trace mismatch\n", expected->name);
        goto cleanup;
    }
    passed = true;
cleanup:
    if (initialized) wz_machine_destroy(&machine);
    return passed;
}

int main(void)
{
    static const prefix_case_t cases[] = {
        {"DD ignored before NOP", {0xddu,0x00u}, 2u,
         {{WZ_BUS_M1_OPCODE_FETCH,0u,0x8000u,0xddu,4u},
          {WZ_BUS_M1_OPCODE_FETCH,8u,0x8001u,0x00u,4u}},
         2u,0x8002u,0x1111u,0x2222u,0x01u,2u,16u},
        {"FD ignored before NOP", {0xfdu,0x00u}, 2u,
         {{WZ_BUS_M1_OPCODE_FETCH,0u,0x8000u,0xfdu,4u},
          {WZ_BUS_M1_OPCODE_FETCH,8u,0x8001u,0x00u,4u}},
         2u,0x8002u,0x1111u,0x2222u,0x01u,2u,16u},
        {"DD FD selects IY", {0xddu,0xfdu,0x21u,0x34u,0x12u}, 5u,
         {{WZ_BUS_M1_OPCODE_FETCH,0u,0x8000u,0xddu,4u},
          {WZ_BUS_M1_OPCODE_FETCH,8u,0x8001u,0xfdu,4u},
          {WZ_BUS_M1_OPCODE_FETCH,16u,0x8002u,0x21u,4u},
          {WZ_BUS_MEMORY_READ,24u,0x8003u,0x34u,3u},
          {WZ_BUS_MEMORY_READ,30u,0x8004u,0x12u,3u}},
         5u,0x8005u,0x1111u,0x1234u,0x01u,3u,36u},
        {"FD DD selects IX", {0xfdu,0xddu,0x21u,0x78u,0x56u}, 5u,
         {{WZ_BUS_M1_OPCODE_FETCH,0u,0x8000u,0xfdu,4u},
          {WZ_BUS_M1_OPCODE_FETCH,8u,0x8001u,0xddu,4u},
          {WZ_BUS_M1_OPCODE_FETCH,16u,0x8002u,0x21u,4u},
          {WZ_BUS_MEMORY_READ,24u,0x8003u,0x78u,3u},
          {WZ_BUS_MEMORY_READ,30u,0x8004u,0x56u,3u}},
         5u,0x8005u,0x5678u,0x2222u,0x01u,3u,36u},
        {"DD DD FD ignored before NOP", {0xddu,0xddu,0xfdu,0x00u}, 4u,
         {{WZ_BUS_M1_OPCODE_FETCH,0u,0x8000u,0xddu,4u},
          {WZ_BUS_M1_OPCODE_FETCH,8u,0x8001u,0xddu,4u},
          {WZ_BUS_M1_OPCODE_FETCH,16u,0x8002u,0xfdu,4u},
          {WZ_BUS_M1_OPCODE_FETCH,24u,0x8003u,0x00u,4u}},
         4u,0x8004u,0x1111u,0x2222u,0x01u,4u,32u},
        {"FD DD FD ignored before NOP", {0xfdu,0xddu,0xfdu,0x00u}, 4u,
         {{WZ_BUS_M1_OPCODE_FETCH,0u,0x8000u,0xfdu,4u},
          {WZ_BUS_M1_OPCODE_FETCH,8u,0x8001u,0xddu,4u},
          {WZ_BUS_M1_OPCODE_FETCH,16u,0x8002u,0xfdu,4u},
          {WZ_BUS_M1_OPCODE_FETCH,24u,0x8003u,0x00u,4u}},
         4u,0x8004u,0x1111u,0x2222u,0x01u,4u,32u},
        {"DD ignored before LD A,n", {0xddu,0x3eu,0x5au}, 3u,
         {{WZ_BUS_M1_OPCODE_FETCH,0u,0x8000u,0xddu,4u},
          {WZ_BUS_M1_OPCODE_FETCH,8u,0x8001u,0x3eu,4u},
          {WZ_BUS_MEMORY_READ,16u,0x8002u,0x5au,3u}},
         3u,0x8003u,0x1111u,0x2222u,0x5au,2u,22u}
    };
    if (!check_decoders()) return 1;
    for (size_t index = 0u; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        if (!run_prefix_case(&cases[index])) return 1;
    }
    (void)puts("{\"testId\":\"cpu-decode-prefix\",\"status\":\"pass\","
               "\"cases\":775,\"decodeEntries\":768,\"prefixCases\":7}");
    return 0;
}
