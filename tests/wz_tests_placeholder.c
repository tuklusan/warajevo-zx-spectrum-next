/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>
#include <string.h>

#include "core/wz_machine.h"
#include "core/wz_bus.h"
#include "core/wz_scheduler.h"
#include "core/wz_state.h"
#include "core/wz_runner.h"
#include "diagnostics/wz_trace_file.h"

static void record_event(void* context)
{
    unsigned* value = (unsigned*)context;
    *value += 1u;
}

static void record_trace(const wz_trace_event_t* event, void* context)
{
    unsigned* count = (unsigned*)context;
    if (event->kind == WZ_TRACE_MASTER_TICK_ADVANCED) {
        *count += 1u;
    }
}

typedef struct {
    wz_trace_event_t events[16];
    size_t count;
} timing_trace_log_t;

static void record_timing_trace(const wz_trace_event_t* event, void* context)
{
    timing_trace_log_t* log = (timing_trace_log_t*)context;
    if (log->count < (sizeof(log->events) / sizeof(log->events[0]))) {
        log->events[log->count] = *event;
    }
    log->count += 1u;
}

static bool recover_trace(const wz_trace_event_t* event, void* context)
{
    wz_qword_t* last_sequence = (wz_qword_t*)context;
    *last_sequence = event->sequence;
    return true;
}

static bool recover_timing_trace(const wz_trace_event_t* event, void* context)
{
    record_timing_trace(event, context);
    return true;
}

typedef struct {
    wz_bus_request_t requests[8];
    size_t count;
} bus_log_t;

static void record_bus_request(const wz_bus_request_t* request, void* context)
{
    bus_log_t* log = (bus_log_t*)context;
    if (log->count < (sizeof(log->requests) / sizeof(log->requests[0]))) {
        log->requests[log->count] = *request;
    }
    log->count += 1u;
}

static wz_byte_t read_bus_input(wz_bus_cycle_t cycle,
                                wz_word_t address,
                                void* context)
{
    const wz_byte_t* interrupt_value = (const wz_byte_t*)context;
    if (cycle == WZ_BUS_IO_READ) {
        return (wz_byte_t)(address >> 8u);
    }
    return *interrupt_value;
}

int main(void)
{
    const wz_machine_profile_t* profile = wz_machine_profile_48k_pal();
    wz_machine_t machine;
    wz_machine_t restored;
    wz_scheduler_t scheduler;
    wz_byte_t serialized[65584u];
    wz_state_writer_t writer;
    wz_qword_t first_hash;
    wz_qword_t second_hash;
    wz_trace_sink_t trace_sink;
    wz_headless_runner_t runner;
    unsigned trace_count = 0u;
    unsigned dispatched = 0u;
    wz_trace_file_t trace_file;
    wz_trace_file_t duplicate;
    wz_bus_observer_t bus_observer;
    wz_bus_input_t bus_input;
    wz_bus_request_t bus_request;
    bus_log_t bus_log;
    timing_trace_log_t timing_trace_log;
    wz_trace_cpu_state_sync_t recovered_cpu_sync;
    wz_qword_t recovered_last = 0u;
    size_t recovered_count = 0u;
    size_t opcode_index;
    size_t documented_unimplemented = 0u;
    size_t implemented = 0u;
    size_t prefix = 0u;
    size_t undocumented = 0u;
    size_t illegal = 0u;
    size_t cb_documented_unimplemented = 0u;
    size_t cb_implemented = 0u;
    size_t cb_undocumented = 0u;
    size_t ed_documented_unimplemented = 0u;
    size_t ed_implemented = 0u;
    size_t ed_undocumented = 0u;
    wz_byte_t interrupt_value = 0x5au;
    FILE* trace_stream;
    const char* trace_path = "wz-trace-regression.bin";
    const char* failing_trace_path = "wz-trace-failing-opcode.bin";
    const char* state_trace_path = "wz-trace-state-regression.bin";

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine initialization failed\n", stderr);
        return 1;
    }
    if (machine.master_tick != 0u || machine.profile != profile) {
        fputs("machine did not initialize deterministic state\n", stderr);
        return 1;
    }
    if (wz_profile_cpu_tstate(5u, profile) != 2u ||
        wz_profile_cpu_phase(5u, profile) != 1u) {
        fputs("master-tick conversion failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK) {
        fputs("bus observer installation failed\n", stderr);
        return 1;
    }
    machine.memory[0x1234u] = 0x9au;
    wz_bus_request_init(&bus_request, WZ_BUS_M1_OPCODE_FETCH, 4u, 0x1234u, 0u, 4u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        bus_request.value != 0x9au) {
        fputs("bus opcode fetch failed\n", stderr);
        return 1;
    }
    wz_bus_request_init(&bus_request, WZ_BUS_MEMORY_WRITE, 8u, 0x4000u, 0x5cu, 3u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        machine.memory[0x4000u] != 0x5cu) {
        fputs("bus memory write failed\n", stderr);
        return 1;
    }
    wz_bus_request_init(&bus_request, WZ_BUS_IO_READ, 12u, 0x00feu, 0u, 4u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        bus_request.value != 0xffu) {
        fputs("bus I/O read failed\n", stderr);
        return 1;
    }
    wz_bus_request_init(&bus_request, WZ_BUS_INTERRUPT_ACKNOWLEDGE, 16u, 0xffffu, 0u, 7u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        bus_request.value != 0xffu) {
        fputs("bus interrupt acknowledge failed\n", stderr);
        return 1;
    }
    wz_bus_request_init(&bus_request, WZ_BUS_INTERNAL, 20u, 0u, 0u, 1u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        bus_log.count != 5u ||
        bus_log.requests[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[1].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[2].cycle != WZ_BUS_IO_READ ||
        bus_log.requests[3].cycle != WZ_BUS_INTERRUPT_ACKNOWLEDGE ||
        bus_log.requests[4].cycle != WZ_BUS_INTERNAL ||
        bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[1].address != 0x4000u ||
        bus_log.requests[1].value != 0x5cu) {
        fputs("mock bus did not record exact requests\n", stderr);
        return 1;
    }
    if (wz_machine_set_bus_observer(&machine, 0) != WZ_RESULT_OK) {
        fputs("bus observer removal failed\n", stderr);
        return 1;
    }
    wz_bus_input_init(&bus_input, read_bus_input, (void*)&interrupt_value);
    wz_bus_request_init(&bus_request, WZ_BUS_IO_READ, 24u, 0x34feu, 0u, 4u);
    if (wz_machine_set_bus_input(&machine, &bus_input) != WZ_RESULT_OK ||
        wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        bus_request.value != 0x34u) {
        fputs("bus input provider failed\n", stderr);
        return 1;
    }
    wz_bus_request_init(&bus_request, WZ_BUS_INTERRUPT_ACKNOWLEDGE,
                        28u, 0xffffu, 0u, 7u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        bus_request.value != interrupt_value ||
        wz_machine_set_bus_input(&machine, 0) != WZ_RESULT_OK) {
        fputs("bus input interrupt or removal failed\n", stderr);
        return 1;
    }
    if (wz_z80_primary_opcode_count() != 256u) {
        fputs("primary opcode table size failed\n", stderr);
        return 1;
    }
    for (opcode_index = 0u; opcode_index < wz_z80_primary_opcode_count(); ++opcode_index) {
        const wz_z80_opcode_decode_t* decode =
            wz_z80_primary_opcode_decode((wz_byte_t)opcode_index);
        if (decode == 0 || decode->opcode != (wz_byte_t)opcode_index) {
            fputs("primary opcode table identity failed\n", stderr);
            return 1;
        }
        if (decode->status == WZ_Z80_OPCODE_DOCUMENTED_UNIMPLEMENTED) {
            documented_unimplemented += 1u;
        } else if (decode->status == WZ_Z80_OPCODE_IMPLEMENTED) {
            implemented += 1u;
        } else if (decode->status == WZ_Z80_OPCODE_PREFIX) {
            prefix += 1u;
        } else if (decode->status == WZ_Z80_OPCODE_UNDOCUMENTED) {
            undocumented += 1u;
        } else if (decode->status == WZ_Z80_OPCODE_ILLEGAL) {
            illegal += 1u;
        } else {
            fputs("primary opcode table classification failed\n", stderr);
            return 1;
        }
    }
    if (implemented != 246u || prefix != 4u || documented_unimplemented != 6u ||
        undocumented != 0u || illegal != 0u ||
        wz_z80_primary_opcode_decode(0x00u)->operation != WZ_Z80_PRIMARY_OP_NOP ||
        wz_z80_primary_opcode_decode(0x32u)->operation != WZ_Z80_PRIMARY_OP_LD_NN_A ||
        wz_z80_primary_opcode_decode(0x01u)->operation != WZ_Z80_PRIMARY_OP_LOAD ||
        wz_z80_primary_opcode_decode(0x40u)->operation != WZ_Z80_PRIMARY_OP_LOAD ||
        wz_z80_primary_opcode_decode(0x3eu)->operation != WZ_Z80_PRIMARY_OP_LD_A_N ||
        wz_z80_primary_opcode_decode(0x80u)->operation != WZ_Z80_PRIMARY_OP_ALU ||
        wz_z80_primary_opcode_decode(0xbfu)->operation != WZ_Z80_PRIMARY_OP_ALU ||
        wz_z80_primary_opcode_decode(0xc6u)->operation != WZ_Z80_PRIMARY_OP_ALU ||
        wz_z80_primary_opcode_decode(0xfeu)->operation != WZ_Z80_PRIMARY_OP_ALU ||
        wz_z80_primary_opcode_decode(0x09u)->operation != WZ_Z80_PRIMARY_OP_ADD_HL_RR ||
        wz_z80_primary_opcode_decode(0x39u)->operation != WZ_Z80_PRIMARY_OP_ADD_HL_RR ||
        wz_z80_primary_opcode_decode(0x07u)->operation != WZ_Z80_PRIMARY_OP_SPECIAL_FLAGS ||
        wz_z80_primary_opcode_decode(0x3fu)->operation != WZ_Z80_PRIMARY_OP_SPECIAL_FLAGS ||
        wz_z80_primary_opcode_decode(0xf1u)->operation != WZ_Z80_PRIMARY_OP_POP ||
        wz_z80_primary_opcode_decode(0xc5u)->operation != WZ_Z80_PRIMARY_OP_PUSH ||
        wz_z80_primary_opcode_decode(0xc9u)->operation != WZ_Z80_PRIMARY_OP_RET ||
        wz_z80_primary_opcode_decode(0xfcu)->operation != WZ_Z80_PRIMARY_OP_CALL ||
        wz_z80_primary_opcode_decode(0xffu)->operation != WZ_Z80_PRIMARY_OP_RST ||
        wz_z80_primary_opcode_decode(0x10u)->operation != WZ_Z80_PRIMARY_OP_BRANCH ||
        wz_z80_primary_opcode_decode(0x38u)->operation != WZ_Z80_PRIMARY_OP_BRANCH ||
        wz_z80_primary_opcode_decode(0xc3u)->operation != WZ_Z80_PRIMARY_OP_BRANCH ||
        wz_z80_primary_opcode_decode(0xe9u)->operation != WZ_Z80_PRIMARY_OP_BRANCH ||
        wz_z80_primary_opcode_decode(0xfau)->operation != WZ_Z80_PRIMARY_OP_BRANCH ||
        wz_z80_primary_opcode_decode(0x04u)->operation != WZ_Z80_PRIMARY_OP_INC_DEC ||
        wz_z80_primary_opcode_decode(0x35u)->operation != WZ_Z80_PRIMARY_OP_INC_DEC ||
        wz_z80_primary_opcode_decode(0x3du)->operation != WZ_Z80_PRIMARY_OP_INC_DEC ||
        wz_z80_primary_opcode_decode(0x76u)->operation != WZ_Z80_PRIMARY_OP_HALT ||
        wz_z80_primary_opcode_decode(0xf3u)->operation != WZ_Z80_PRIMARY_OP_DI ||
        wz_z80_primary_opcode_decode(0xfbu)->operation != WZ_Z80_PRIMARY_OP_EI ||
        wz_z80_primary_opcode_decode(0xcbu)->operation != WZ_Z80_PRIMARY_OP_PREFIX_CB ||
        wz_z80_primary_opcode_decode(0xddu)->operation != WZ_Z80_PRIMARY_OP_PREFIX_DD ||
        wz_z80_primary_opcode_decode(0xedu)->operation != WZ_Z80_PRIMARY_OP_PREFIX_ED ||
        wz_z80_primary_opcode_decode(0xfdu)->operation != WZ_Z80_PRIMARY_OP_PREFIX_FD) {
        fputs("primary opcode table contents failed\n", stderr);
        return 1;
    }
    memset(&machine.cpu, 0, sizeof(machine.cpu));
    machine.master_tick = 0u;
    machine.cpu.program_counter = 0x2000u;
    machine.memory[0x2000u] = 0x01u;
    machine.memory[0x2001u] = 0x34u;
    machine.memory[0x2002u] = 0x12u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.main.b != 0x12u ||
        machine.cpu.main.c != 0x34u || machine.cpu.program_counter != 0x2003u ||
        machine.master_tick != 20u) {
        fputs("LD BC,nn state or timing failed\n", stderr);
        return 1;
    }
    memset(&machine.cpu, 0, sizeof(machine.cpu));
    machine.master_tick = 0u;
    machine.cpu.program_counter = 0x2050u;
    machine.cpu.main.a = 0x56u;
    machine.cpu.main.b = 0x00u;
    machine.cpu.main.c = 0x01u;
    machine.memory[0x2050u] = 0x02u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.memory[0x0001u] != 0x56u ||
        machine.cpu.memptr != 0x5602u || machine.master_tick != 14u) {
        fputs("LD (BC),A MEMPTR state failed\n", stderr);
        return 1;
    }
    memset(&machine.cpu, 0, sizeof(machine.cpu));
    memset(&bus_log, 0, sizeof(bus_log));
    machine.master_tick = 0u;
    machine.cpu.program_counter = 0x2100u;
    machine.cpu.main.h = 0x12u;
    machine.cpu.main.l = 0x34u;
    machine.memory[0x2100u] = 0x22u;
    machine.memory[0x2101u] = 0x00u;
    machine.memory[0x2102u] = 0x40u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK || machine.memory[0x4000u] != 0x34u ||
        machine.memory[0x4001u] != 0x12u || machine.cpu.program_counter != 0x2103u ||
        machine.cpu.memptr != 0x4001u || machine.master_tick != 32u || bus_log.count != 5u ||
        bus_log.requests[1].cycle != WZ_BUS_MEMORY_READ || bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ || bus_log.requests[2].master_tick != 14u ||
        bus_log.requests[3].cycle != WZ_BUS_MEMORY_WRITE || bus_log.requests[3].master_tick != 20u ||
        bus_log.requests[4].cycle != WZ_BUS_MEMORY_WRITE || bus_log.requests[4].master_tick != 26u ||
        wz_machine_set_bus_observer(&machine, 0) != WZ_RESULT_OK) {
        fputs("LD (nn),HL bus sequence failed\n", stderr);
        return 1;
    }
    memset(&machine.cpu, 0, sizeof(machine.cpu));
    machine.master_tick = 0u;
    machine.cpu.program_counter = 0x2200u;
    machine.cpu.main.h = 0x50u;
    machine.cpu.main.l = 0x00u;
    machine.memory[0x2200u] = 0x36u;
    machine.memory[0x2201u] = 0xa5u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.memory[0x5000u] != 0xa5u ||
        machine.cpu.program_counter != 0x2202u || machine.master_tick != 20u) {
        fputs("LD (HL),n state or timing failed\n", stderr);
        return 1;
    }
    memset(&machine.cpu, 0, sizeof(machine.cpu));
    machine.master_tick = 0u;
    machine.cpu.program_counter = 0x2300u;
    machine.cpu.main.a = 0x11u;
    machine.cpu.main.f = 0x22u;
    machine.cpu.alternate.a = 0x33u;
    machine.cpu.alternate.f = 0x44u;
    machine.memory[0x2300u] = 0x08u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.main.a != 0x33u ||
        machine.cpu.main.f != 0x44u || machine.cpu.alternate.a != 0x11u ||
        machine.cpu.alternate.f != 0x22u || machine.master_tick != 8u) {
        fputs("EX AF,AF' state or timing failed\n", stderr);
        return 1;
    }
    if (wz_z80_cb_opcode_count() != 256u) {
        fputs("CB opcode table size failed\n", stderr);
        return 1;
    }
    for (opcode_index = 0u; opcode_index < wz_z80_cb_opcode_count(); ++opcode_index) {
        const wz_z80_cb_opcode_decode_t* decode =
            wz_z80_cb_opcode_decode((wz_byte_t)opcode_index);
        if (decode == 0 || decode->opcode != (wz_byte_t)opcode_index ||
            decode->target != (wz_byte_t)(opcode_index & 0x07u) || decode->bit > 7u) {
            fputs("CB opcode table identity failed\n", stderr);
            return 1;
        }
        if (decode->status == WZ_Z80_OPCODE_DOCUMENTED_UNIMPLEMENTED) {
            cb_documented_unimplemented += 1u;
        } else if (decode->status == WZ_Z80_OPCODE_IMPLEMENTED) {
            cb_implemented += 1u;
        } else if (decode->status == WZ_Z80_OPCODE_UNDOCUMENTED) {
            cb_undocumented += 1u;
        } else {
            fputs("CB opcode table classification failed\n", stderr);
            return 1;
        }
    }
    if (cb_documented_unimplemented != 0u || cb_implemented != 248u || cb_undocumented != 8u ||
        wz_z80_cb_opcode_decode(0x00u)->operation != WZ_Z80_CB_OP_RLC ||
        wz_z80_cb_opcode_decode(0x30u)->operation != WZ_Z80_CB_OP_SLL ||
        wz_z80_cb_opcode_decode(0x30u)->status != WZ_Z80_OPCODE_UNDOCUMENTED ||
        wz_z80_cb_opcode_decode(0x40u)->operation != WZ_Z80_CB_OP_BIT ||
        wz_z80_cb_opcode_decode(0x40u)->bit != 0u ||
        wz_z80_cb_opcode_decode(0x7fu)->operation != WZ_Z80_CB_OP_BIT ||
        wz_z80_cb_opcode_decode(0x7fu)->bit != 7u ||
        wz_z80_cb_opcode_decode(0x80u)->operation != WZ_Z80_CB_OP_RES ||
        wz_z80_cb_opcode_decode(0xffu)->operation != WZ_Z80_CB_OP_SET ||
        wz_z80_cb_opcode_decode(0xffu)->target != 7u ||
        wz_z80_cb_opcode_decode(0xffu)->bit != 7u) {
        fputs("CB opcode table contents failed\n", stderr);
        return 1;
    }
    if (wz_z80_ed_opcode_count() != 256u) {
        fputs("ED opcode table size failed\n", stderr);
        return 1;
    }
    for (opcode_index = 0u; opcode_index < wz_z80_ed_opcode_count(); ++opcode_index) {
        wz_z80_ed_opcode_decode_t decode =
            wz_z80_ed_opcode_decode((wz_byte_t)opcode_index);
        if (decode.opcode != (wz_byte_t)opcode_index) {
            fputs("ED opcode table identity failed\n", stderr);
            return 1;
        }
        if (decode.status == WZ_Z80_OPCODE_DOCUMENTED_UNIMPLEMENTED) {
            ed_documented_unimplemented += 1u;
        } else if (decode.status == WZ_Z80_OPCODE_IMPLEMENTED) {
            ed_implemented += 1u;
        } else if (decode.status == WZ_Z80_OPCODE_UNDOCUMENTED) {
            ed_undocumented += 1u;
        } else {
            fputs("ED opcode table classification failed\n", stderr);
            return 1;
        }
    }
    if (ed_documented_unimplemented != 0u || ed_implemented == 0u ||
        ed_documented_unimplemented + ed_implemented + ed_undocumented != 256u ||
        wz_z80_ed_opcode_decode(0x44u).operation != WZ_Z80_ED_OP_NEG ||
        wz_z80_ed_opcode_decode(0x44u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0x4cu).operation != WZ_Z80_ED_OP_NEG ||
        wz_z80_ed_opcode_decode(0x4cu).status != WZ_Z80_OPCODE_UNDOCUMENTED ||
        wz_z80_ed_opcode_decode(0x45u).operation != WZ_Z80_ED_OP_RETN ||
        wz_z80_ed_opcode_decode(0x45u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0x4du).operation != WZ_Z80_ED_OP_RETI ||
        wz_z80_ed_opcode_decode(0x4du).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0x70u).operation != WZ_Z80_ED_OP_IN_R_C ||
        wz_z80_ed_opcode_decode(0x70u).status != WZ_Z80_OPCODE_UNDOCUMENTED ||
        wz_z80_ed_opcode_decode(0x43u).operation != WZ_Z80_ED_OP_LD_NN_RR ||
        wz_z80_ed_opcode_decode(0x4bu).operation != WZ_Z80_ED_OP_LD_RR_NN ||
        wz_z80_ed_opcode_decode(0xa0u).operation != WZ_Z80_ED_OP_LDI ||
        wz_z80_ed_opcode_decode(0xa0u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xa8u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xb0u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xb8u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xa1u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xa9u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xb1u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xb9u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xa2u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xa3u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xaau).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xabu).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xb2u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xb3u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xbau).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xbbu).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xbbu).operation != WZ_Z80_ED_OP_OTDR ||
        wz_z80_ed_opcode_decode(0xffu).operation != WZ_Z80_ED_OP_UNSUPPORTED ||
        wz_z80_ed_opcode_decode(0xffu).status != WZ_Z80_OPCODE_UNDOCUMENTED) {
        fputs("ED opcode table contents failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 skeleton test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.r != 1u ||
        machine.cpu.program_counter != 1u ||
        machine.master_tick != 8u ||
        bus_log.count != 1u ||
        bus_log.requests[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[0].master_tick != 0u ||
        bus_log.requests[0].address != 0u ||
        bus_log.requests[0].value != 0u) {
        fputs("Z80 NOP fetch trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before structured timing trace test failed\n", stderr);
        return 1;
    }
    memset(&timing_trace_log, 0, sizeof(timing_trace_log));
    wz_trace_sink_init(&trace_sink, record_timing_trace, &timing_trace_log);
    trace_sink.next_sequence = WZ_TRACE_CPU_SYNC_INTERVAL - 2u;
    wz_machine_set_timing_trace(&machine, &trace_sink);
    machine.cpu.main.a = 0x12u;
    machine.cpu.main.f = 0x34u;
    machine.cpu.alternate.a = 0x56u;
    machine.cpu.alternate.f = 0x78u;
    machine.cpu.ix = 0x9abcu;
    machine.cpu.iy = 0xdef0u;
    machine.cpu.stack_pointer = 0x1357u;
    machine.cpu.memptr = 0x2468u;
    machine.cpu.i = 0x9au;
    machine.cpu.r = 0x3cu;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.interrupt_enable_delay = 1u;
    machine.cpu.interrupt_mode = (wz_byte_t)WZ_Z80_INTERRUPT_MODE_2;
    machine.cpu.halted = 0u;
    machine.memory[0u] = 0x00u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK || timing_trace_log.count != 8u ||
        timing_trace_log.events[0].kind != WZ_TRACE_CPU_BUS ||
        timing_trace_log.events[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        timing_trace_log.events[0].address != 0u ||
        timing_trace_log.events[1].kind != WZ_TRACE_CPU_OPCODE_BYTE ||
        timing_trace_log.events[1].master_tick != timing_trace_log.events[0].master_tick ||
        timing_trace_log.events[1].address != 0u || timing_trace_log.events[1].value != 0x00u ||
        timing_trace_log.events[2].kind != WZ_TRACE_CPU_INSTRUCTION ||
        timing_trace_log.events[2].program_counter != 0u ||
        timing_trace_log.events[2].value != 0x00u ||
        timing_trace_log.events[2].sequence != WZ_TRACE_CPU_SYNC_INTERVAL ||
        (timing_trace_log.events[2].register_snapshot & UINT64_C(0xffff)) != UINT64_C(0x3412)) {
        fputs("structured CPU timing trace failed\n", stderr);
        return 1;
    }
    if (timing_trace_log.events[3].kind != WZ_TRACE_CPU_STATE_SYNC ||
        timing_trace_log.events[3].cycle != 0u ||
        timing_trace_log.events[3].register_snapshot != timing_trace_log.events[2].register_snapshot ||
        timing_trace_log.events[4].cycle != 1u ||
        (timing_trace_log.events[4].register_snapshot & UINT64_C(0xffff)) != UINT64_C(0x7856) ||
        timing_trace_log.events[5].cycle != 2u ||
        (timing_trace_log.events[5].register_snapshot & UINT64_C(0xffffffff)) != UINT64_C(0xdef09abc) ||
        timing_trace_log.events[6].cycle != 3u ||
        (timing_trace_log.events[6].register_snapshot & UINT64_C(0xffff)) != UINT64_C(0x2468) ||
        timing_trace_log.events[7].cycle != 4u ||
        timing_trace_log.events[7].register_snapshot != 0u ||
        timing_trace_log.events[3].sequence + 4u != timing_trace_log.events[7].sequence) {
        fputs("complete CPU timing synchronization trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before failing opcode trace test failed\n", stderr);
        return 1;
    }
    remove(failing_trace_path);
    if (wz_trace_file_create(&trace_file, failing_trace_path, 4u,
                             (wz_dword_t)profile->kind, 0x9abcu, UINT32_MAX) != WZ_RESULT_OK) {
        fputs("failing opcode trace creation failed\n", stderr);
        return 1;
    }
    wz_trace_sink_init(&trace_sink, wz_trace_file_emit, &trace_file);
    wz_machine_set_timing_trace(&machine, &trace_sink);
    machine.memory[0u] = 0xd3u;
    if (wz_z80_step(&machine) != WZ_RESULT_UNSUPPORTED_OPERATION ||
        wz_trace_file_freeze(&trace_file) != WZ_RESULT_OK) {
        wz_trace_file_close(&trace_file);
        fputs("failing opcode trace execution failed\n", stderr);
        return 1;
    }
    wz_trace_file_close(&trace_file);
    memset(&timing_trace_log, 0, sizeof(timing_trace_log));
    if (wz_trace_file_recover(failing_trace_path, recover_timing_trace, &timing_trace_log,
                              &recovered_count) != WZ_RESULT_OK ||
        recovered_count != 8u || timing_trace_log.count != 8u ||
        timing_trace_log.events[0].kind != WZ_TRACE_CPU_BUS ||
        timing_trace_log.events[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        timing_trace_log.events[0].address != 0u || timing_trace_log.events[0].value != 0xd3u ||
        timing_trace_log.events[1].kind != WZ_TRACE_CPU_OPCODE_BYTE ||
        timing_trace_log.events[1].address != 0u || timing_trace_log.events[1].value != 0xd3u ||
        timing_trace_log.events[2].kind != WZ_TRACE_CPU_INSTRUCTION ||
        timing_trace_log.events[2].program_counter != 0u || timing_trace_log.events[2].value != 0xd3u) {
        remove(failing_trace_path);
        fputs("failing opcode trace recovery failed\n", stderr);
        return 1;
    }
    wz_trace_cpu_state_sync_init(&recovered_cpu_sync);
    {
        bool recovered_state = false;
        for (size_t index = 3u; index < timing_trace_log.count; ++index) {
            recovered_state = wz_trace_cpu_state_sync_apply(&recovered_cpu_sync,
                                                             &timing_trace_log.events[index]);
        }
        if (!recovered_state || !recovered_cpu_sync.has_absolute_state ||
            recovered_cpu_sync.master_tick != 0u ||
            recovered_cpu_sync.state.program_counter != 1u || recovered_cpu_sync.state.r != 1u) {
            remove(failing_trace_path);
            fputs("failing opcode state reconstruction failed\n", stderr);
            return 1;
        }
    }
    remove(failing_trace_path);
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 load test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.memory[0u] = 0x3eu;
    machine.memory[1u] = 0x77u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x77u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 14u ||
        bus_log.count != 2u ||
        bus_log.requests[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[1].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[1].address != 1u ||
        bus_log.requests[1].value != 0x77u) {
        fputs("Z80 immediate load trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 store test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.a = 0x22u;
    machine.memory[0u] = 0x32u;
    machine.memory[1u] = 0x00u;
    machine.memory[2u] = 0x40u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.memory[0x4000u] != 0x22u ||
        machine.cpu.program_counter != 3u ||
        machine.cpu.memptr != 0x2201u ||
        machine.master_tick != 26u ||
        bus_log.count != 4u ||
        bus_log.requests[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[1].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[3].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[3].master_tick != 20u ||
        bus_log.requests[3].address != 0x4000u ||
        bus_log.requests[3].value != 0x22u) {
        fputs("Z80 absolute store trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 unsupported-opcode test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.memory[0u] = 0xd3u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_UNSUPPORTED_OPERATION ||
        machine.cpu.program_counter != 1u ||
        machine.master_tick != 0u ||
        bus_log.count != 1u ||
        bus_log.requests[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[0].address != 0u ||
        bus_log.requests[0].value != 0xd3u) {
        fputs("Z80 unsupported opcode trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ADD HL edge test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.h = 0x0fu;
    machine.cpu.main.l = 0xffu;
    machine.cpu.main.b = 0xf0u;
    machine.cpu.main.c = 0x01u;
    machine.cpu.main.f = 0xc7u;
    machine.memory[0u] = 0x09u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.h != 0x00u || machine.cpu.main.l != 0x00u ||
        machine.cpu.main.f != 0xd5u || machine.cpu.memptr != 0x1000u ||
        machine.cpu.program_counter != 1u || machine.master_tick != 22u) {
        fputs("Z80 ADD HL,BC carry/half-carry flags failed\n", stderr);
        return 1;
    }
    machine.cpu.main.h = 0x20u;
    machine.cpu.main.l = 0x00u;
    machine.cpu.stack_pointer = 0x0800u;
    machine.cpu.main.f = 0x45u;
    machine.memory[1u] = 0x39u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.h != 0x28u || machine.cpu.main.l != 0x00u ||
        machine.cpu.main.f != 0x6cu || machine.cpu.memptr != 0x2001u ||
        machine.cpu.program_counter != 2u || machine.master_tick != 44u) {
        fputs("Z80 ADD HL,SP preserved/undocumented flags failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before special flag vectors failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x9au;
    machine.memory[0u] = 0x27u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x00u || machine.cpu.main.f != 0x55u) {
        fputs("Z80 DAA addition correction failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x0fu;
    machine.cpu.main.f = 0x12u;
    machine.memory[1u] = 0x27u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x09u || machine.cpu.main.f != 0x0eu) {
        fputs("Z80 DAA subtraction correction failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x9au;
    machine.cpu.main.f = 0x02u;
    machine.memory[2u] = 0x27u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x34u || machine.cpu.main.f != 0x23u) {
        fputs("Z80 DAA subtraction invalid-BCD correction failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x55u;
    machine.cpu.main.f = 0x45u;
    machine.memory[3u] = 0x2fu;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0xaau || machine.cpu.main.f != 0x7fu) {
        fputs("Z80 CPL flags failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x28u;
    machine.cpu.main.f = 0x44u;
    machine.memory[4u] = 0x37u;
    machine.memory[5u] = 0x3fu;
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.main.f != 0x6du ||
        wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.main.f != 0x7cu) {
        fputs("Z80 SCF/CCF flags failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x00u;
    machine.cpu.main.f = 0xffu;
    machine.memory[6u] = 0x37u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.main.f != 0xedu) {
        fputs("Z80 SCF prior-flag X/Y sourcing failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0xffu;
    machine.cpu.main.f = 0x00u;
    machine.memory[7u] = 0x37u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.main.f != 0x29u) {
        fputs("Z80 SCF accumulator X/Y sourcing failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before accumulator rotate vectors failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x81u;
    machine.cpu.main.f = 0x44u;
    machine.memory[0u] = 0x07u;
    machine.memory[1u] = 0x1fu;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x03u || machine.cpu.main.f != 0x45u ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x81u || machine.cpu.main.f != 0x45u ||
        machine.cpu.program_counter != 2u || machine.master_tick != 16u) {
        fputs("Z80 accumulator rotate flags failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 CB-prefix test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.memory[0u] = 0xcbu;
    machine.memory[1u] = 0x11u;
    machine.cpu.main.c = 0x80u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.c != 0x00u ||
        machine.cpu.main.f != 0x45u ||
        machine.cpu.r != 2u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 16u ||
        bus_log.count != 2u ||
        bus_log.requests[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[0].address != 0u ||
        bus_log.requests[0].value != 0xcbu ||
        bus_log.requests[1].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[1].master_tick != 4u ||
        bus_log.requests[1].address != 1u ||
        bus_log.requests[1].value != 0x11u) {
        fputs("Z80 CB-prefix fetch trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED-prefix test failed\n", stderr);
        return 1;
    }

    machine.memory[0u] = 0xddu;
    machine.memory[1u] = 0xfdu;
    machine.memory[2u] = 0x00u;
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 3u ||
        machine.cpu.r != 3u ||
        machine.master_tick != 24u ||
        bus_log.count != 3u ||
        bus_log.requests[0].value != 0xddu ||
        bus_log.requests[1].value != 0xfdu ||
        bus_log.requests[2].value != 0x00u) {
        fputs("Z80 repeated index-prefix NOP failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before ignored index-prefix ED test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x15u;
    machine.memory[0u] = 0xddu;
    machine.memory[1u] = 0xedu;
    machine.memory[2u] = 0x44u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0xebu ||
        machine.cpu.program_counter != 3u ||
        machine.cpu.r != 3u ||
        machine.master_tick != 24u) {
        fputs("Z80 ignored index-prefix ED execution failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before index register load test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.memory[0u] = 0xddu;
    machine.memory[1u] = 0x21u;
    machine.memory[2u] = 0x34u;
    machine.memory[3u] = 0x12u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.ix != 0x1234u || machine.cpu.iy != 0u ||
        machine.cpu.program_counter != 4u || machine.cpu.r != 2u ||
        machine.master_tick != 28u || bus_log.count != 4u ||
        bus_log.requests[1].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[1].address != 1u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 16u ||
        bus_log.requests[3].master_tick != 22u) {
        fputs("Z80 DD LD IX,nn trace failed\n", stderr);
        return 1;
    }

    machine.memory[4u] = 0xfdu;
    machine.memory[5u] = 0x21u;
    machine.memory[6u] = 0x78u;
    machine.memory[7u] = 0x56u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.ix != 0x1234u || machine.cpu.iy != 0x5678u ||
        machine.cpu.program_counter != 8u || machine.cpu.r != 4u ||
        machine.master_tick != 56u) {
        fputs("Z80 FD LD IY,nn failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before index memory transfer test failed\n", stderr);
        return 1;
    }
    machine.cpu.ix = 0xabcdu;
    machine.memory[0u] = 0xddu;
    machine.memory[1u] = 0x22u;
    machine.memory[2u] = 0x00u;
    machine.memory[3u] = 0x40u;
    machine.memory[4u] = 0xfdu;
    machine.memory[5u] = 0x2au;
    machine.memory[6u] = 0x00u;
    machine.memory[7u] = 0x40u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.memory[0x4000u] != 0xcdu || machine.memory[0x4001u] != 0xabu ||
        machine.cpu.memptr != 0x4001u || machine.master_tick != 40u ||
        wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.iy != 0xabcdu ||
        machine.cpu.program_counter != 8u || machine.cpu.memptr != 0x4001u ||
        machine.master_tick != 80u) {
        fputs("Z80 DD/FD index memory transfer failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before index arithmetic test failed\n", stderr);
        return 1;
    }
    machine.cpu.ix = 0x0fffu;
    machine.cpu.main.b = 0x00u;
    machine.cpu.main.c = 0x01u;
    machine.cpu.main.f = 0xc5u;
    machine.memory[0u] = 0xddu;
    machine.memory[1u] = 0x09u;
    machine.memory[2u] = 0xddu;
    machine.memory[3u] = 0x23u;
    machine.memory[4u] = 0xddu;
    machine.memory[5u] = 0x2bu;
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.ix != 0x1000u ||
        machine.cpu.main.f != 0xd4u || machine.cpu.memptr != 0x1000u ||
        machine.master_tick != 30u ||
        wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.ix != 0x1001u ||
        machine.cpu.main.f != 0xd4u || machine.master_tick != 50u ||
        wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.ix != 0x1000u ||
        machine.cpu.main.f != 0xd4u || machine.master_tick != 70u) {
        fputs("Z80 index add/inc/dec failed\n", stderr);
        return 1;
    }

    machine.cpu.iy = 0x3456u;
    machine.memory[6u] = 0xfdu;
    machine.memory[7u] = 0xf9u;
    machine.memory[8u] = 0xfdu;
    machine.memory[9u] = 0xe9u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.stack_pointer != 0x3456u || machine.master_tick != 90u ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x3456u || machine.master_tick != 106u) {
        fputs("Z80 index stack-pointer/jump transfer failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before indexed CB rotate test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.ix = 0x4002u;
    machine.memory[0u] = 0xddu;
    machine.memory[1u] = 0xcbu;
    machine.memory[2u] = 0xfeu;
    machine.memory[3u] = 0x00u;
    machine.memory[0x4000u] = 0x80u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.memory[0x4000u] != 0x01u || machine.cpu.main.b != 0x01u ||
        machine.cpu.main.f != 0x01u || machine.cpu.memptr != 0x4000u ||
        machine.cpu.program_counter != 4u || machine.cpu.r != 2u ||
        machine.master_tick != 46u || bus_log.count != 7u ||
        bus_log.requests[1].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 16u ||
        bus_log.requests[3].master_tick != 22u ||
        bus_log.requests[4].address != 0x4000u ||
        bus_log.requests[5].cycle != WZ_BUS_INTERNAL ||
        bus_log.requests[5].master_tick != 34u ||
        bus_log.requests[6].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[6].master_tick != 40u) {
        fputs("Z80 DDCB rotate/writeback trace failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before indexed CB bit test failed\n", stderr);
        return 1;
    }
    machine.cpu.iy = 0x27ffu;
    machine.cpu.main.b = 0x55u;
    machine.cpu.main.f = 0x01u;
    machine.memory[0u] = 0xfdu;
    machine.memory[1u] = 0xcbu;
    machine.memory[2u] = 0x01u;
    machine.memory[3u] = 0x78u;
    machine.memory[0x2800u] = 0x80u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.memory[0x2800u] != 0x80u || machine.cpu.main.b != 0x55u ||
        machine.cpu.main.f != 0xb9u || machine.cpu.memptr != 0x2800u ||
        machine.cpu.program_counter != 4u || machine.cpu.r != 2u ||
        machine.master_tick != 40u) {
        fputs("Z80 FDCB BIT memory-only behavior failed\n", stderr);
        return 1;
    }

    machine.cpu.program_counter = 4u;
    machine.memory[4u] = 0xfdu;
    machine.memory[5u] = 0xcbu;
    machine.memory[6u] = 0x01u;
    machine.memory[7u] = 0xdeu;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.memory[0x2800u] != 0x88u || machine.cpu.main.f != 0xb9u ||
        machine.cpu.program_counter != 8u || machine.master_tick != 86u) {
        fputs("Z80 FDCB SET memory-only behavior failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before ALU edge vectors failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x7fu;
    machine.memory[0u] = 0xc6u; machine.memory[1u] = 0x01u;
    machine.memory[2u] = 0xceu; machine.memory[3u] = 0x00u;
    machine.memory[4u] = 0xd6u; machine.memory[5u] = 0x01u;
    machine.memory[6u] = 0xdeu; machine.memory[7u] = 0x7fu;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x80u || machine.cpu.main.f != 0x94u ||
        machine.cpu.program_counter != 2u || machine.master_tick != 14u) {
        fputs("Z80 ADD immediate flags failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0xffu; machine.cpu.main.f = 0x01u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x00u || machine.cpu.main.f != 0x51u) {
        fputs("Z80 ADC immediate flags failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x00u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0xffu || machine.cpu.main.f != 0xbbu) {
        fputs("Z80 SUB immediate flags failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x80u; machine.cpu.main.f = 0x01u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x00u || machine.cpu.main.f != 0x56u) {
        fputs("Z80 SBC immediate flags failed\n", stderr);
        return 1;
    }

    machine.memory[8u] = 0xe6u; machine.memory[9u] = 0x3cu;
    machine.memory[10u] = 0xeeu; machine.memory[11u] = 0xffu;
    machine.memory[12u] = 0xf6u; machine.memory[13u] = 0x08u;
    machine.memory[14u] = 0xfeu; machine.memory[15u] = 0x28u;
    machine.cpu.main.a = 0xf0u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x30u || machine.cpu.main.f != 0x34u ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0xcfu || machine.cpu.main.f != 0x8cu) {
        fputs("Z80 AND/XOR immediate flags failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x80u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x88u || machine.cpu.main.f != 0x8cu) {
        fputs("Z80 OR immediate flags failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x10u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x10u || machine.cpu.main.f != 0xbbu ||
        machine.cpu.program_counter != 16u || machine.master_tick != 112u) {
        fputs("Z80 CP immediate flags/writeback failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before ALU operand-path test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.a = 1u; machine.cpu.main.b = 2u;
    machine.cpu.main.h = 0x40u; machine.cpu.main.l = 0x00u;
    machine.memory[0u] = 0x80u; machine.memory[1u] = 0x86u;
    machine.memory[0x4000u] = 3u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.main.a != 3u ||
        machine.master_tick != 8u ||
        wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.main.a != 6u ||
        machine.master_tick != 22u || bus_log.count != 3u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 16u ||
        bus_log.requests[2].address != 0x4000u) {
        fputs("Z80 ALU register/memory operand trace failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before CP operand-flag test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.a = 0xf5u;
    machine.cpu.main.b = 0x0fu;
    machine.cpu.main.h = 0x40u;
    machine.cpu.main.l = 0x00u;
    machine.memory[0u] = 0xb8u;
    machine.memory[1u] = 0xbeu;
    machine.memory[0x4000u] = 0x0fu;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0xf5u || machine.cpu.main.f != 0x9au ||
        machine.cpu.program_counter != 1u || machine.master_tick != 8u ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0xf5u || machine.cpu.main.f != 0x9au ||
        machine.cpu.program_counter != 2u || machine.master_tick != 22u ||
        bus_log.count != 3u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 16u ||
        bus_log.requests[2].address != 0x4000u || bus_log.requests[2].value != 0x0fu) {
        fputs("Z80 CP operand X/Y flags or memory trace failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before BIT (HL) MEMPTR flag test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.f = 0x01u;
    machine.cpu.main.h = 0x61u;
    machine.cpu.main.l = 0x31u;
    machine.cpu.memptr = 0xff00u;
    machine.memory[0u] = 0xcbu;
    machine.memory[1u] = 0x46u;
    machine.memory[0x6131u] = 0xd5u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.f != 0x39u || machine.cpu.memptr != 0xff00u ||
        machine.cpu.program_counter != 2u || machine.cpu.r != 2u ||
        machine.master_tick != 24u) {
        fputs("Z80 BIT (HL) MEMPTR X/Y flags failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 unsupported ED-prefix test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0xffu;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_UNSUPPORTED_OPERATION ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 0u ||
        bus_log.count != 2u ||
        bus_log.requests[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[0].address != 0u ||
        bus_log.requests[0].value != 0xedu ||
        bus_log.requests[1].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[1].master_tick != 4u ||
        bus_log.requests[1].address != 1u ||
        bus_log.requests[1].value != 0xffu) {
        fputs("Z80 ED-prefix fetch trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED neg test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x15u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x44u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0xebu ||
        machine.cpu.main.f != 0xbbu ||
        machine.cpu.r != 2u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 16u) {
        fputs("Z80 ED NEG failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED interrupt-mode test failed\n", stderr);
        return 1;
    }
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x5eu;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.interrupt_mode != (wz_byte_t)WZ_Z80_INTERRUPT_MODE_2 ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 16u) {
        fputs("Z80 ED IM failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED I/R transfer test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x28u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x47u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.i != 0x28u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 18u) {
        fputs("Z80 ED LD I,A failed\n", stderr);
        return 1;
    }
    machine.cpu.iff2 = 1u;
    machine.cpu.main.f = 0x01u;
    machine.memory[2u] = 0xedu;
    machine.memory[3u] = 0x57u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x28u ||
        machine.cpu.main.f != 0x2du ||
        machine.cpu.program_counter != 4u ||
        machine.master_tick != 36u) {
        fputs("Z80 ED LD A,I failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED input register test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.b = 0x12u;
    machine.cpu.main.c = 0xfeu;
    machine.cpu.main.f = 0x01u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x40u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.b != 0xffu ||
        machine.cpu.main.f != 0xadu ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 24u ||
        bus_log.count != 3u ||
        bus_log.requests[2].cycle != WZ_BUS_IO_READ ||
        bus_log.requests[2].master_tick != 8u ||
        bus_log.requests[2].address != 0x12feu ||
        bus_log.requests[2].value != 0xffu) {
        fputs("Z80 ED IN r,(C) trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED input flags-only test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.b = 0x12u;
    machine.cpu.main.c = 0xfeu;
    machine.cpu.main.f = 0u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x70u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.b != 0x12u ||
        machine.cpu.main.f != 0xacu ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 24u) {
        fputs("Z80 ED IN (C) flags failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED output register test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.b = 0x12u;
    machine.cpu.main.c = 0xfeu;
    machine.cpu.main.f = 0xa5u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x49u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.f != 0xa5u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 24u ||
        bus_log.count != 3u ||
        bus_log.requests[2].cycle != WZ_BUS_IO_WRITE ||
        bus_log.requests[2].master_tick != 8u ||
        bus_log.requests[2].address != 0x12feu ||
        bus_log.requests[2].value != 0xfeu) {
        fputs("Z80 ED OUT (C),r trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED ADC HL,rr test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.h = 0x7fu;
    machine.cpu.main.l = 0xffu;
    machine.cpu.main.d = 0x00u;
    machine.cpu.main.e = 0x01u;
    machine.cpu.main.f = 0u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x5au;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.h != 0x80u ||
        machine.cpu.main.l != 0x00u ||
        machine.cpu.main.f != 0x94u ||
        machine.cpu.memptr != 0x8000u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 30u) {
        fputs("Z80 ED ADC HL,rr failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED SBC HL,rr test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.h = 0x00u;
    machine.cpu.main.l = 0x00u;
    machine.cpu.stack_pointer = 0x0001u;
    machine.cpu.main.f = 0x01u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x72u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.h != 0xffu ||
        machine.cpu.main.l != 0xfeu ||
        machine.cpu.main.f != 0xbbu ||
        machine.cpu.memptr != 0x0001u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 30u) {
        fputs("Z80 ED SBC HL,rr failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED RRD test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.a = 0x84u;
    machine.cpu.main.f = 0x01u;
    machine.cpu.main.h = 0x40u;
    machine.cpu.main.l = 0x00u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x67u;
    machine.memory[0x4000u] = 0x31u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x81u ||
        machine.cpu.main.f != 0x85u ||
        machine.memory[0x4000u] != 0x43u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 36u ||
        bus_log.count != 4u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 8u ||
        bus_log.requests[2].address != 0x4000u ||
        bus_log.requests[2].value != 0x31u ||
        bus_log.requests[3].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[3].master_tick != 14u ||
        bus_log.requests[3].address != 0x4000u ||
        bus_log.requests[3].value != 0x43u ||
        machine.cpu.memptr != 0x4001u) {
        fputs("Z80 ED RRD trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED RLD test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x25u;
    machine.cpu.main.f = 0x01u;
    machine.cpu.main.h = 0x40u;
    machine.cpu.main.l = 0x00u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x6fu;
    machine.memory[0x4000u] = 0x96u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x29u ||
        machine.cpu.main.f != 0x29u ||
        machine.memory[0x4000u] != 0x65u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 36u) {
        fputs("Z80 ED RLD failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED LDI test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.a = 0x10u;
    machine.cpu.main.b = 0x00u;
    machine.cpu.main.c = 0x02u;
    machine.cpu.main.d = 0x50u;
    machine.cpu.main.e = 0x00u;
    machine.cpu.main.h = 0x40u;
    machine.cpu.main.l = 0x00u;
    machine.cpu.main.f = 0xd3u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0xa0u;
    machine.memory[0x4000u] = 0x22u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.memory[0x5000u] != 0x22u ||
        machine.cpu.main.b != 0x00u ||
        machine.cpu.main.c != 0x01u ||
        machine.cpu.main.d != 0x50u ||
        machine.cpu.main.e != 0x01u ||
        machine.cpu.main.h != 0x40u ||
        machine.cpu.main.l != 0x01u ||
        machine.cpu.main.f != 0xe5u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 32u ||
        bus_log.count != 4u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 8u ||
        bus_log.requests[2].address != 0x4000u ||
        bus_log.requests[2].value != 0x22u ||
        bus_log.requests[3].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[3].master_tick != 14u ||
        bus_log.requests[3].address != 0x5000u ||
        bus_log.requests[3].value != 0x22u) {
        fputs("Z80 ED LDI trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED LDDR test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x01u;
    machine.cpu.main.b = 0x00u;
    machine.cpu.main.c = 0x02u;
    machine.cpu.main.d = 0x50u;
    machine.cpu.main.e = 0x00u;
    machine.cpu.main.h = 0x40u;
    machine.cpu.main.l = 0x00u;
    machine.cpu.main.f = 0xc1u;
    machine.cpu.memptr = 0x1234u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0xb8u;
    machine.memory[0x4000u] = 0x01u;
    machine.memory[0x3fffu] = 0x07u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0u ||
        machine.master_tick != 42u ||
        machine.cpu.main.b != 0x00u ||
        machine.cpu.main.c != 0x01u ||
        machine.cpu.main.d != 0x4fu ||
        machine.cpu.main.e != 0xffu ||
        machine.cpu.main.h != 0x3fu ||
        machine.cpu.main.l != 0xffu ||
        machine.cpu.main.f != 0xe5u ||
        machine.memory[0x5000u] != 0x01u ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 74u ||
        machine.cpu.main.b != 0x00u ||
        machine.cpu.main.c != 0x00u ||
        machine.cpu.main.d != 0x4fu ||
        machine.cpu.main.e != 0xfeu ||
        machine.cpu.main.h != 0x3fu ||
        machine.cpu.main.l != 0xfeu ||
        machine.cpu.main.f != 0xc9u ||
        machine.cpu.memptr != 0x0001u ||
        machine.memory[0x4fffu] != 0x07u) {
        fputs("Z80 ED LDDR repeat failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED CPI test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.a = 0x10u;
    machine.cpu.main.b = 0x00u;
    machine.cpu.main.c = 0x02u;
    machine.cpu.main.h = 0x40u;
    machine.cpu.main.l = 0x00u;
    machine.cpu.main.f = 0xc1u;
    machine.cpu.memptr = 0x1234u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0xa1u;
    machine.memory[0x4000u] = 0x01u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x10u ||
        machine.cpu.main.b != 0x00u ||
        machine.cpu.main.c != 0x01u ||
        machine.cpu.main.h != 0x40u ||
        machine.cpu.main.l != 0x01u ||
        machine.cpu.main.f != 0x3fu ||
        machine.cpu.memptr != 0x1235u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 32u ||
        bus_log.count != 3u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 8u ||
        bus_log.requests[2].address != 0x4000u ||
        bus_log.requests[2].value != 0x01u) {
        fputs("Z80 ED CPI trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED CPDR test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x10u;
    machine.cpu.main.b = 0x00u;
    machine.cpu.main.c = 0x02u;
    machine.cpu.main.h = 0x40u;
    machine.cpu.main.l = 0x00u;
    machine.cpu.main.f = 0x01u;
    machine.cpu.memptr = 0x1234u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0xb9u;
    machine.memory[0x4000u] = 0x01u;
    machine.memory[0x3fffu] = 0x10u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0u ||
        machine.master_tick != 42u ||
        machine.cpu.main.b != 0x00u ||
        machine.cpu.main.c != 0x01u ||
        machine.cpu.main.h != 0x3fu ||
        machine.cpu.main.l != 0xffu ||
        machine.cpu.main.f != 0x3fu ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 74u ||
        machine.cpu.main.b != 0x00u ||
        machine.cpu.main.c != 0x00u ||
        machine.cpu.main.h != 0x3fu ||
        machine.cpu.main.l != 0xfeu ||
        machine.cpu.main.f != 0x43u ||
        machine.cpu.memptr != 0x0000u) {
        fputs("Z80 ED CPDR repeat failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED block I/O test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.b = 0x02u;
    machine.cpu.main.c = 0x00u;
    machine.cpu.main.h = 0x40u;
    machine.cpu.main.l = 0x00u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0xb2u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0u ||
        machine.master_tick != 42u ||
        machine.cpu.main.b != 0x01u ||
        machine.cpu.main.h != 0x40u ||
        machine.cpu.main.l != 0x01u ||
        machine.cpu.main.f != 0x13u ||
        machine.memory[0x4000u] != 0xffu ||
        bus_log.count != 4u ||
        bus_log.requests[2].cycle != WZ_BUS_IO_READ ||
        bus_log.requests[2].address != 0x0200u ||
        bus_log.requests[3].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[3].address != 0x4000u ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 74u ||
        machine.cpu.main.b != 0x00u ||
        machine.cpu.main.h != 0x40u ||
        machine.cpu.main.l != 0x02u ||
        machine.cpu.main.f != 0x57u ||
        machine.cpu.memptr != 0x0101u ||
        machine.memory[0x4001u] != 0xffu) {
        fputs("Z80 ED INIR repeat failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED OUTD test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.b = 0x02u;
    machine.cpu.main.c = 0x10u;
    machine.cpu.main.h = 0x40u;
    machine.cpu.main.l = 0x00u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0xabu;
    machine.memory[0x4000u] = 0x80u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.b != 0x01u ||
        machine.cpu.main.h != 0x3fu ||
        machine.cpu.main.l != 0xffu ||
        machine.cpu.main.f != 0x17u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 32u ||
        bus_log.count != 4u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].address != 0x4000u ||
        bus_log.requests[3].cycle != WZ_BUS_IO_WRITE ||
        bus_log.requests[3].address != 0x0110u ||
        bus_log.requests[3].value != 0x80u ||
        machine.cpu.memptr != 0x010fu) {
        fputs("Z80 ED OUTD trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED RETN test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.stack_pointer = 0xfffeu;
    machine.cpu.iff1 = 0u;
    machine.cpu.iff2 = 1u;
    machine.cpu.main.f = 0xa5u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x45u;
    machine.memory[0xfffeu] = 0x34u;
    machine.memory[0xffffu] = 0x12u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x1234u ||
        machine.cpu.stack_pointer != 0u ||
        machine.cpu.iff1 != 1u ||
        machine.cpu.iff2 != 1u ||
        machine.cpu.main.f != 0xa5u ||
        machine.cpu.memptr != 0x1234u ||
        machine.master_tick != 28u ||
        bus_log.count != 4u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 8u ||
        bus_log.requests[2].address != 0xfffeu ||
        bus_log.requests[2].value != 0x34u ||
        bus_log.requests[3].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[3].master_tick != 14u ||
        bus_log.requests[3].address != 0xffffu ||
        bus_log.requests[3].value != 0x12u) {
        fputs("Z80 ED RETN trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED RETI test failed\n", stderr);
        return 1;
    }
    machine.cpu.stack_pointer = 0x4000u;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 0u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x4du;
    machine.memory[0x4000u] = 0x78u;
    machine.memory[0x4001u] = 0x56u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x5678u ||
        machine.cpu.stack_pointer != 0x4002u ||
        machine.cpu.iff1 != 0u ||
        machine.master_tick != 28u) {
        fputs("Z80 ED RETI failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED store pair test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.b = 0x12u;
    machine.cpu.main.c = 0x34u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x43u;
    machine.memory[2u] = 0xfeu;
    machine.memory[3u] = 0xffu;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.memory[0xfffeu] != 0x34u ||
        machine.memory[0xffffu] != 0x12u ||
        machine.cpu.program_counter != 4u ||
        machine.master_tick != 40u ||
        bus_log.count != 6u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 8u ||
        bus_log.requests[2].address != 2u ||
        bus_log.requests[3].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[3].master_tick != 14u ||
        bus_log.requests[3].address != 3u ||
        bus_log.requests[4].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[4].master_tick != 20u ||
        bus_log.requests[4].address != 0xfffeu ||
        bus_log.requests[4].value != 0x34u ||
        bus_log.requests[5].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[5].master_tick != 26u ||
        bus_log.requests[5].address != 0xffffu ||
        bus_log.requests[5].value != 0x12u ||
        machine.cpu.memptr != 0xffffu) {
        fputs("Z80 ED store pair trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED load pair test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x7bu;
    machine.memory[2u] = 0x00u;
    machine.memory[3u] = 0x40u;
    machine.memory[0x4000u] = 0x78u;
    machine.memory[0x4001u] = 0x56u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.stack_pointer != 0x5678u ||
        machine.cpu.program_counter != 4u ||
        machine.master_tick != 40u ||
        bus_log.count != 6u ||
        bus_log.requests[4].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[4].master_tick != 20u ||
        bus_log.requests[4].address != 0x4000u ||
        bus_log.requests[4].value != 0x78u ||
        bus_log.requests[5].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[5].master_tick != 26u ||
        bus_log.requests[5].address != 0x4001u ||
        bus_log.requests[5].value != 0x56u) {
        fputs("Z80 ED load pair trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 CB memory rotate test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.h = 0x40u;
    machine.cpu.main.l = 0x00u;
    machine.memory[0u] = 0xcbu;
    machine.memory[1u] = 0x36u;
    machine.memory[0x4000u] = 0x80u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.memory[0x4000u] != 0x01u ||
        machine.cpu.main.f != 0x01u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 30u ||
        bus_log.count != 4u ||
        bus_log.requests[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[1].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 8u ||
        bus_log.requests[2].address != 0x4000u ||
        bus_log.requests[2].value != 0x80u ||
        bus_log.requests[3].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[3].master_tick != 14u ||
        bus_log.requests[3].address != 0x4000u ||
        bus_log.requests[3].value != 0x01u) {
        fputs("Z80 CB memory rotate trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 CB bit test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.h = 0x28u;
    machine.cpu.main.l = 0x00u;
    machine.cpu.main.f = 0x01u;
    machine.memory[0u] = 0xcbu;
    machine.memory[1u] = 0x7eu;
    machine.memory[0x2800u] = 0x80u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.memory[0x2800u] != 0x80u ||
        machine.cpu.main.f != 0xb9u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 24u ||
        bus_log.count != 3u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].address != 0x2800u ||
        bus_log.requests[2].value != 0x80u) {
        fputs("Z80 CB bit trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 CB set/res test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.b = 0x80u;
    machine.cpu.main.f = 0xa5u;
    machine.memory[0u] = 0xcbu;
    machine.memory[1u] = 0xd8u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.b != 0x88u ||
        machine.cpu.main.f != 0xa5u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 16u) {
        fputs("Z80 CB set register failed\n", stderr);
        return 1;
    }
    machine.memory[2u] = 0xcbu;
    machine.memory[3u] = 0x80u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.b != 0x88u ||
        machine.cpu.main.f != 0xa5u ||
        machine.cpu.program_counter != 4u ||
        machine.master_tick != 32u) {
        fputs("Z80 CB res register failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 PUSH test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.b = 0x12u;
    machine.cpu.main.c = 0x34u;
    machine.cpu.stack_pointer = 0u;
    machine.memory[0u] = 0xc5u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 1u ||
        machine.cpu.stack_pointer != 0xfffeu ||
        machine.memory[0xffffu] != 0x12u ||
        machine.memory[0xfffeu] != 0x34u ||
        machine.master_tick != 22u ||
        bus_log.count != 4u ||
        bus_log.requests[1].cycle != WZ_BUS_INTERNAL ||
        bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[2].master_tick != 10u ||
        bus_log.requests[2].address != 0xffffu ||
        bus_log.requests[2].value != 0x12u ||
        bus_log.requests[3].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[3].master_tick != 16u ||
        bus_log.requests[3].address != 0xfffeu ||
        bus_log.requests[3].value != 0x34u) {
        fputs("Z80 PUSH wrapping trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 POP test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.stack_pointer = 0xfffeu;
    machine.memory[0u] = 0xf1u;
    machine.memory[0xfffeu] = 0xa5u;
    machine.memory[0xffffu] = 0x5au;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x5au ||
        machine.cpu.main.f != 0xa5u ||
        machine.cpu.stack_pointer != 0u ||
        machine.master_tick != 20u ||
        bus_log.count != 3u ||
        bus_log.requests[1].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[1].address != 0xfffeu ||
        bus_log.requests[1].value != 0xa5u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 14u ||
        bus_log.requests[2].address != 0xffffu ||
        bus_log.requests[2].value != 0x5au) {
        fputs("Z80 POP AF wrapping trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 CALL test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.stack_pointer = 0u;
    machine.memory[0u] = 0xcdu;
    machine.memory[1u] = 0x78u;
    machine.memory[2u] = 0x56u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x5678u ||
        machine.cpu.memptr != 0x5678u ||
        machine.cpu.stack_pointer != 0xfffeu ||
        machine.memory[0xffffu] != 0u ||
        machine.memory[0xfffeu] != 3u ||
        machine.master_tick != 34u ||
        bus_log.count != 6u ||
        bus_log.requests[1].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 14u ||
        bus_log.requests[3].cycle != WZ_BUS_INTERNAL ||
        bus_log.requests[3].master_tick != 20u ||
        bus_log.requests[4].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[4].master_tick != 22u ||
        bus_log.requests[4].address != 0xffffu ||
        bus_log.requests[4].value != 0u ||
        bus_log.requests[5].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[5].master_tick != 28u ||
        bus_log.requests[5].address != 0xfffeu ||
        bus_log.requests[5].value != 3u) {
        fputs("Z80 CALL trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before untaken conditional CALL failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.f = 0x40u;
    machine.cpu.stack_pointer = 0x4000u;
    machine.memory[0u] = 0xc4u;
    machine.memory[1u] = 0x34u;
    machine.memory[2u] = 0x12u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 3u ||
        machine.cpu.memptr != 0x1234u ||
        machine.cpu.stack_pointer != 0x4000u ||
        machine.master_tick != 20u ||
        bus_log.count != 3u) {
        fputs("Z80 untaken conditional CALL trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before taken conditional CALL failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.stack_pointer = 0x4000u;
    machine.memory[0u] = 0xc4u;
    machine.memory[1u] = 0x34u;
    machine.memory[2u] = 0x12u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x1234u ||
        machine.cpu.stack_pointer != 0x3ffeu ||
        machine.memory[0x3fffu] != 0u ||
        machine.memory[0x3ffeu] != 3u ||
        machine.master_tick != 34u ||
        bus_log.count != 6u) {
        fputs("Z80 taken conditional CALL trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 RET test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.stack_pointer = 0xffffu;
    machine.memory[0u] = 0xc9u;
    machine.memory[0xffffu] = 0x34u;
    machine.memory[0u] = 0x12u;
    machine.memory[1u] = 0xc9u;
    machine.cpu.program_counter = 1u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x1234u ||
        machine.cpu.memptr != 0x1234u ||
        machine.cpu.stack_pointer != 1u ||
        machine.master_tick != 20u ||
        bus_log.count != 3u ||
        bus_log.requests[1].address != 0xffffu ||
        bus_log.requests[1].value != 0x34u ||
        bus_log.requests[2].address != 0u ||
        bus_log.requests[2].value != 0x12u) {
        fputs("Z80 RET wrapping trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before conditional RET tests failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.f = 0x40u;
    machine.cpu.stack_pointer = 0x4000u;
    machine.memory[0u] = 0xc0u;
    machine.memory[0x4000u] = 0x78u;
    machine.memory[0x4001u] = 0x56u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 1u ||
        machine.cpu.stack_pointer != 0x4000u ||
        machine.master_tick != 10u ||
        bus_log.count != 2u ||
        bus_log.requests[1].cycle != WZ_BUS_INTERNAL ||
        bus_log.requests[1].master_tick != 8u) {
        fputs("Z80 untaken conditional RET trace failed\n", stderr);
        return 1;
    }
    machine.cpu.main.f = 0u;
    machine.cpu.program_counter = 0u;
    machine.master_tick = 0u;
    memset(&bus_log, 0, sizeof(bus_log));
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x5678u ||
        machine.cpu.stack_pointer != 0x4002u ||
        machine.master_tick != 22u ||
        bus_log.count != 4u ||
        bus_log.requests[1].cycle != WZ_BUS_INTERNAL ||
        bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 10u ||
        bus_log.requests[3].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[3].master_tick != 16u) {
        fputs("Z80 taken conditional RET trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 RST test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.program_counter = 0x1234u;
    machine.cpu.stack_pointer = 1u;
    machine.memory[0x1234u] = 0xffu;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x38u ||
        machine.cpu.memptr != 0x38u ||
        machine.cpu.stack_pointer != 0xffffu ||
        machine.memory[0u] != 0x12u ||
        machine.memory[0xffffu] != 0x35u ||
        machine.master_tick != 22u ||
        bus_log.count != 4u ||
        bus_log.requests[1].cycle != WZ_BUS_INTERNAL ||
        bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[2].master_tick != 10u ||
        bus_log.requests[2].address != 0u ||
        bus_log.requests[2].value != 0x12u ||
        bus_log.requests[3].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[3].master_tick != 16u ||
        bus_log.requests[3].address != 0xffffu ||
        bus_log.requests[3].value != 0x35u) {
        fputs("Z80 RST wrapping trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 INC register test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.b = 0x7fu;
    machine.cpu.main.f = 0x01u;
    machine.memory[0u] = 0x04u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.b != 0x80u ||
        machine.cpu.main.f != 0x95u ||
        machine.cpu.program_counter != 1u ||
        machine.master_tick != 8u) {
        fputs("Z80 INC register overflow flags failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 DEC register test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.c = 0x80u;
    machine.cpu.main.f = 0x01u;
    machine.memory[0u] = 0x0du;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.c != 0x7fu ||
        machine.cpu.main.f != 0x3fu ||
        machine.cpu.program_counter != 1u ||
        machine.master_tick != 8u) {
        fputs("Z80 DEC register overflow flags failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 INC memory test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.h = 0xffu;
    machine.cpu.main.l = 0xffu;
    machine.cpu.main.f = 0x01u;
    machine.memory[0u] = 0x34u;
    machine.memory[0xffffu] = 0xffu;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.memory[0xffffu] != 0u ||
        machine.cpu.main.f != 0x51u ||
        machine.cpu.program_counter != 1u ||
        machine.master_tick != 22u ||
        bus_log.count != 4u ||
        bus_log.requests[1].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[1].address != 0xffffu ||
        bus_log.requests[1].value != 0xffu ||
        bus_log.requests[2].cycle != WZ_BUS_INTERNAL ||
        bus_log.requests[2].master_tick != 14u ||
        bus_log.requests[2].address != 0xffffu ||
        bus_log.requests[2].t_states != 1u ||
        bus_log.requests[3].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[3].master_tick != 16u ||
        bus_log.requests[3].address != 0xffffu ||
        bus_log.requests[3].value != 0u) {
        fputs("Z80 INC memory trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 JR test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.program_counter = 0xfffeu;
    machine.memory[0xfffeu] = 0x18u;
    machine.memory[0xffffu] = 0xfeu;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0xfffeu ||
        machine.cpu.memptr != 0xfffeu ||
        machine.master_tick != 24u ||
        bus_log.count != 3u ||
        bus_log.requests[1].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[1].address != 0xffffu ||
        bus_log.requests[1].value != 0xfeu ||
        bus_log.requests[2].cycle != WZ_BUS_INTERNAL ||
        bus_log.requests[2].master_tick != 14u ||
        bus_log.requests[2].address != 0xffffu ||
        bus_log.requests[2].t_states != 5u) {
        fputs("Z80 JR signed wrapping trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before conditional JR test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.f = 0x40u;
    machine.cpu.memptr = 0x1234u;
    machine.memory[0u] = 0x20u;
    machine.memory[1u] = 0x40u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 2u ||
        machine.cpu.memptr != 0x1234u ||
        machine.master_tick != 14u ||
        bus_log.count != 2u) {
        fputs("Z80 untaken conditional JR trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 DJNZ taken test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.b = 2u;
    machine.memory[0u] = 0x10u;
    machine.memory[1u] = 0xfeu;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.b != 1u ||
        machine.cpu.program_counter != 0u ||
        machine.cpu.memptr != 0u ||
        machine.master_tick != 26u ||
        bus_log.count != 4u ||
        bus_log.requests[1].cycle != WZ_BUS_INTERNAL ||
        bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[1].t_states != 1u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 10u ||
        bus_log.requests[2].address != 1u ||
        bus_log.requests[3].cycle != WZ_BUS_INTERNAL ||
        bus_log.requests[3].master_tick != 16u ||
        bus_log.requests[3].address != 1u ||
        bus_log.requests[3].t_states != 5u) {
        fputs("Z80 DJNZ taken trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 DJNZ untaken test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.b = 1u;
    machine.cpu.memptr = 0x5678u;
    machine.memory[0u] = 0x10u;
    machine.memory[1u] = 0xfeu;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.b != 0u ||
        machine.cpu.program_counter != 2u ||
        machine.cpu.memptr != 0x5678u ||
        machine.master_tick != 16u ||
        bus_log.count != 3u) {
        fputs("Z80 DJNZ untaken trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before conditional JP test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.f = 0x40u;
    machine.memory[0u] = 0xc2u;
    machine.memory[1u] = 0x34u;
    machine.memory[2u] = 0x12u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 3u ||
        machine.cpu.memptr != 0x1234u ||
        machine.master_tick != 20u ||
        bus_log.count != 3u ||
        bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[1].address != 1u ||
        bus_log.requests[2].master_tick != 14u ||
        bus_log.requests[2].address != 2u) {
        fputs("Z80 untaken conditional JP trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 JP HL test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.h = 0xcau;
    machine.cpu.main.l = 0xfeu;
    machine.cpu.memptr = 0x9abcu;
    machine.memory[0u] = 0xe9u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0xcafeu ||
        machine.cpu.memptr != 0x9abcu ||
        machine.master_tick != 8u ||
        bus_log.count != 1u) {
        fputs("Z80 JP HL trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 HALT test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.program_counter = 0x1234u;
    machine.cpu.r = 0xffu;
    machine.memory[0x1234u] = 0x76u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.halted != 1u ||
        machine.cpu.program_counter != 0x1234u ||
        machine.cpu.r != 0x80u ||
        machine.master_tick != 8u ||
        bus_log.count != 1u ||
        bus_log.requests[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[0].address != 0x1234u ||
        bus_log.requests[0].value != 0x76u) {
        fputs("Z80 HALT entry failed\n", stderr);
        return 1;
    }
    machine.memory[0x1234u] = 0u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.halted != 1u ||
        machine.cpu.program_counter != 0x1234u ||
        machine.cpu.r != 0x81u ||
        machine.master_tick != 16u ||
        bus_log.count != 2u ||
        bus_log.requests[1].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[1].address != 0x1234u ||
        bus_log.requests[1].value != 0u) {
        fputs("Z80 repeated halted M1 failed\n", stderr);
        return 1;
    }
    wz_z80_exit_halt_for_interrupt(&machine.cpu);
    if (machine.cpu.halted != 0u || machine.cpu.program_counter != 0x1235u ||
        machine.cpu.r != 0x81u || machine.master_tick != 16u) {
        fputs("Z80 accepted-interrupt HALT exit boundary failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 EI/DI test failed\n", stderr);
        return 1;
    }
    machine.memory[0u] = 0xfbu;
    machine.memory[1u] = 0x00u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.iff1 != 1u || machine.cpu.iff2 != 1u ||
        machine.cpu.interrupt_enable_delay != 1u ||
        wz_z80_maskable_interrupts_acceptable(&machine.cpu)) {
        fputs("Z80 EI did not defer maskable interrupt acceptance\n", stderr);
        return 1;
    }
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.interrupt_enable_delay != 0u ||
        !wz_z80_maskable_interrupts_acceptable(&machine.cpu)) {
        fputs("Z80 EI delay did not expire after one instruction\n", stderr);
        return 1;
    }
    machine.memory[2u] = 0xf3u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.iff1 != 0u || machine.cpu.iff2 != 0u ||
        machine.cpu.interrupt_enable_delay != 0u ||
        wz_z80_maskable_interrupts_acceptable(&machine.cpu)) {
        fputs("Z80 DI did not immediately disable maskable interrupts\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 EI HALT delay test failed\n", stderr);
        return 1;
    }
    machine.memory[0u] = 0xfbu;
    machine.memory[1u] = 0x76u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.halted != 1u || machine.cpu.interrupt_enable_delay != 0u ||
        !wz_z80_maskable_interrupts_acceptable(&machine.cpu)) {
        fputs("Z80 EI HALT acceptance boundary failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 IM0 interrupt test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    wz_bus_input_init(&bus_input, read_bus_input, (void*)&interrupt_value);
    interrupt_value = 0x3eu;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.program_counter = 0x1234u;
    machine.cpu.stack_pointer = 0x8000u;
    machine.cpu.r = 0x7fu;
    machine.memory[0x1234u] = 0x5au;
    memset(&timing_trace_log, 0, sizeof(timing_trace_log));
    wz_trace_sink_init(&trace_sink, record_timing_trace, &timing_trace_log);
    wz_machine_set_timing_trace(&machine, &trace_sink);
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_machine_set_bus_input(&machine, &bus_input) != WZ_RESULT_OK ||
        wz_z80_accept_maskable_interrupt(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x1235u || machine.cpu.stack_pointer != 0x8000u ||
        machine.cpu.main.a != 0x5au ||
        machine.cpu.iff1 != 0u || machine.cpu.iff2 != 0u || machine.cpu.r != 0u ||
        machine.master_tick != 20u || bus_log.count != 2u ||
        timing_trace_log.events[0].kind != WZ_TRACE_INTERRUPT ||
        timing_trace_log.events[0].value != WZ_TRACE_INTERRUPT_MASKABLE_SAMPLE ||
        timing_trace_log.events[1].kind != WZ_TRACE_INTERRUPT ||
        timing_trace_log.events[1].value != WZ_TRACE_INTERRUPT_MASKABLE_ACCEPT ||
        timing_trace_log.events[2].kind != WZ_TRACE_CPU_BUS ||
        timing_trace_log.events[2].cycle != WZ_BUS_INTERRUPT_ACKNOWLEDGE ||
        timing_trace_log.events[0].sequence + 1u != timing_trace_log.events[1].sequence ||
        timing_trace_log.events[1].sequence + 1u != timing_trace_log.events[2].sequence ||
        bus_log.requests[0].cycle != WZ_BUS_INTERRUPT_ACKNOWLEDGE ||
        bus_log.requests[0].master_tick != 0u || bus_log.requests[0].t_states != 7u ||
        bus_log.requests[1].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[1].master_tick != 14u || bus_log.requests[1].address != 0x1234u) {
        fputs("Z80 IM0 injected primary opcode test failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 IM0 CB interrupt test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    wz_bus_input_init(&bus_input, read_bus_input, (void*)&interrupt_value);
    interrupt_value = 0xcbu;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.program_counter = 0x1234u;
    machine.cpu.r = 0x7fu;
    machine.cpu.main.b = 0x80u;
    machine.memory[0x1234u] = 0x00u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_machine_set_bus_input(&machine, &bus_input) != WZ_RESULT_OK ||
        wz_z80_accept_maskable_interrupt(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x1235u || machine.cpu.main.b != 0x01u ||
        machine.cpu.r != 0x01u || machine.master_tick != 22u || bus_log.count != 2u ||
        bus_log.requests[0].cycle != WZ_BUS_INTERRUPT_ACKNOWLEDGE ||
        bus_log.requests[1].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[1].master_tick != 10u || bus_log.requests[1].address != 0x1234u) {
        fputs("Z80 IM0 injected CB opcode test failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 IM0 ED interrupt test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    wz_bus_input_init(&bus_input, read_bus_input, (void*)&interrupt_value);
    interrupt_value = 0xedu;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.program_counter = 0x1234u;
    machine.cpu.r = 0x7fu;
    machine.cpu.main.a = 0x01u;
    machine.memory[0x1234u] = 0x44u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_machine_set_bus_input(&machine, &bus_input) != WZ_RESULT_OK ||
        wz_z80_accept_maskable_interrupt(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x1235u || machine.cpu.main.a != 0xffu ||
        machine.cpu.r != 0x01u || machine.master_tick != 22u || bus_log.count != 2u ||
        bus_log.requests[0].cycle != WZ_BUS_INTERRUPT_ACKNOWLEDGE ||
        bus_log.requests[1].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[1].master_tick != 10u || bus_log.requests[1].address != 0x1234u) {
        fputs("Z80 IM0 injected ED opcode test failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 IM0 DD interrupt test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    wz_bus_input_init(&bus_input, read_bus_input, (void*)&interrupt_value);
    interrupt_value = 0xddu;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.program_counter = 0x1234u;
    machine.cpu.r = 0x7fu;
    machine.memory[0x1234u] = 0x21u;
    machine.memory[0x1235u] = 0x78u;
    machine.memory[0x1236u] = 0x56u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_machine_set_bus_input(&machine, &bus_input) != WZ_RESULT_OK ||
        wz_z80_accept_maskable_interrupt(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x1237u || machine.cpu.ix != 0x5678u ||
        machine.cpu.r != 0x01u || machine.master_tick != 34u || bus_log.count != 4u ||
        bus_log.requests[0].cycle != WZ_BUS_INTERRUPT_ACKNOWLEDGE ||
        bus_log.requests[1].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[1].master_tick != 14u || bus_log.requests[1].address != 0x1234u ||
        bus_log.requests[2].master_tick != 22u || bus_log.requests[3].master_tick != 28u) {
        fputs("Z80 IM0 injected DD opcode test failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 IM0 FD interrupt test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    wz_bus_input_init(&bus_input, read_bus_input, (void*)&interrupt_value);
    interrupt_value = 0xfdu;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.program_counter = 0x1234u;
    machine.cpu.r = 0x7fu;
    machine.memory[0x1234u] = 0x21u;
    machine.memory[0x1235u] = 0x78u;
    machine.memory[0x1236u] = 0x56u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_machine_set_bus_input(&machine, &bus_input) != WZ_RESULT_OK ||
        wz_z80_accept_maskable_interrupt(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x1237u || machine.cpu.iy != 0x5678u ||
        machine.cpu.r != 0x01u || machine.master_tick != 34u || bus_log.count != 4u ||
        bus_log.requests[0].cycle != WZ_BUS_INTERRUPT_ACKNOWLEDGE ||
        bus_log.requests[1].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[1].master_tick != 14u || bus_log.requests[1].address != 0x1234u ||
        bus_log.requests[2].master_tick != 22u || bus_log.requests[3].master_tick != 28u) {
        fputs("Z80 IM0 injected FD opcode test failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 IM0 RST interrupt test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    wz_bus_input_init(&bus_input, read_bus_input, (void*)&interrupt_value);
    interrupt_value = 0xffu;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.program_counter = 0x1234u;
    machine.cpu.stack_pointer = 0x8000u;
    machine.cpu.r = 0x7fu;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_machine_set_bus_input(&machine, &bus_input) != WZ_RESULT_OK ||
        wz_z80_accept_maskable_interrupt(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x0038u || machine.cpu.stack_pointer != 0x7ffeu ||
        machine.memory[0x7fffu] != 0x12u || machine.memory[0x7ffeu] != 0x34u ||
        machine.cpu.r != 0u || machine.master_tick != 28u || bus_log.count != 4u ||
        bus_log.requests[0].cycle != WZ_BUS_INTERRUPT_ACKNOWLEDGE ||
        bus_log.requests[1].master_tick != 14u || bus_log.requests[2].master_tick != 16u ||
        bus_log.requests[3].master_tick != 22u) {
        fputs("Z80 IM0 injected RST opcode test failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 IM1 interrupt test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    memset(&timing_trace_log, 0, sizeof(timing_trace_log));
    wz_trace_sink_init(&trace_sink, record_timing_trace, &timing_trace_log);
    wz_machine_set_timing_trace(&machine, &trace_sink);
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_accept_maskable_interrupt(&machine) != WZ_RESULT_UNSUPPORTED_OPERATION ||
        timing_trace_log.count != 1u ||
        timing_trace_log.events[0].kind != WZ_TRACE_INTERRUPT ||
        timing_trace_log.events[0].value != WZ_TRACE_INTERRUPT_MASKABLE_SAMPLE ||
        machine.cpu.program_counter != 0u || machine.cpu.stack_pointer != 0xffffu ||
        machine.cpu.iff1 != 0u || machine.cpu.iff2 != 0u ||
        machine.master_tick != 0u || bus_log.count != 0u) {
        fputs("Z80 rejected maskable interrupt trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset after rejected interrupt trace test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.interrupt_mode = (wz_byte_t)WZ_Z80_INTERRUPT_MODE_1;
    machine.cpu.program_counter = 0x3456u;
    machine.cpu.stack_pointer = 0x8000u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_accept_maskable_interrupt(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x0038u || machine.memory[0x7fffu] != 0x34u ||
        machine.memory[0x7ffeu] != 0x56u || machine.master_tick != 26u ||
        bus_log.count != 3u || bus_log.requests[0].cycle != WZ_BUS_INTERRUPT_ACKNOWLEDGE ||
        bus_log.requests[1].master_tick != 14u || bus_log.requests[2].master_tick != 20u) {
        fputs("Z80 IM1 interrupt trace failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 IM2 interrupt test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    interrupt_value = 0x10u;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.interrupt_mode = (wz_byte_t)WZ_Z80_INTERRUPT_MODE_2;
    machine.cpu.i = 0x80u;
    machine.cpu.program_counter = 0x3456u;
    machine.cpu.stack_pointer = 0x8000u;
    machine.memory[0x8010u] = 0x34u;
    machine.memory[0x8011u] = 0x12u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_machine_set_bus_input(&machine, &bus_input) != WZ_RESULT_OK ||
        wz_z80_accept_maskable_interrupt(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x1234u || machine.cpu.memptr != 0x1234u ||
        machine.memory[0x7fffu] != 0x34u || machine.memory[0x7ffeu] != 0x56u ||
        machine.master_tick != 38u || bus_log.count != 5u ||
        bus_log.requests[0].cycle != WZ_BUS_INTERRUPT_ACKNOWLEDGE ||
        bus_log.requests[1].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[1].master_tick != 14u || bus_log.requests[1].address != 0x8010u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 20u || bus_log.requests[2].address != 0x8011u ||
        bus_log.requests[3].master_tick != 26u || bus_log.requests[4].master_tick != 32u) {
        fputs("Z80 IM2 interrupt trace failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 NMI test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 0u;
    machine.cpu.halted = 1u;
    machine.cpu.program_counter = 0x2000u;
    machine.cpu.stack_pointer = 0x8000u;
    machine.cpu.r = 0x7fu;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_accept_nmi(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x0066u || machine.cpu.memptr != 0x0066u ||
        machine.cpu.halted != 0u || machine.cpu.iff1 != 0u || machine.cpu.iff2 != 1u ||
        machine.cpu.r != 0u || machine.memory[0x7fffu] != 0x20u ||
        machine.memory[0x7ffeu] != 0x01u || machine.master_tick != 22u ||
        bus_log.count != 3u || bus_log.requests[0].cycle != WZ_BUS_INTERNAL ||
        bus_log.requests[0].master_tick != 0u || bus_log.requests[0].t_states != 5u ||
        bus_log.requests[1].master_tick != 10u || bus_log.requests[2].master_tick != 16u) {
        fputs("Z80 NMI interrupt trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_set_bus_input(&machine, 0) != WZ_RESULT_OK) {
        fputs("Z80 interrupt input removal failed\n", stderr);
        return 1;
    }

    wz_state_writer_init(&writer, serialized, sizeof(serialized));
    if (wz_state_serialize_machine(&machine, &writer) != WZ_RESULT_OK ||
        writer.length != 65579u ||
        wz_state_hash_machine(&machine, &first_hash) != WZ_RESULT_OK) {
        fputs("canonical state serialization failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x12u;
    machine.cpu.alternate.f = 0x34u;
    machine.cpu.ix = 0xabcdu;
    machine.cpu.iy = 0x2345u;
    machine.cpu.memptr = 0x6789u;
    machine.cpu.i = 0x56u;
    machine.cpu.r = 0x78u;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.interrupt_enable_delay = 1u;
    machine.cpu.interrupt_mode = (wz_byte_t)WZ_Z80_INTERRUPT_MODE_2;
    machine.cpu.halted = 1u;
    wz_state_writer_init(&writer, serialized, sizeof(serialized));
    if (wz_state_serialize_machine(&machine, &writer) != WZ_RESULT_OK ||
        wz_state_deserialize_machine(&restored, serialized, writer.length) != WZ_RESULT_OK ||
        restored.cpu.main.a != 0x12u ||
        restored.cpu.alternate.f != 0x34u ||
        restored.cpu.ix != 0xabcdu ||
        restored.cpu.iy != 0x2345u ||
        restored.cpu.memptr != 0x6789u ||
        restored.cpu.i != 0x56u ||
        restored.cpu.r != 0x78u ||
        restored.cpu.iff1 != 1u ||
        restored.cpu.iff2 != 1u ||
        restored.cpu.interrupt_enable_delay != 1u ||
        restored.cpu.interrupt_mode != (wz_byte_t)WZ_Z80_INTERRUPT_MODE_2 ||
        restored.cpu.halted != 1u) {
        fputs("Z80 state round trip failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reinitialization failed\n", stderr);
        return 1;
    }
    wz_state_writer_init(&writer, serialized, sizeof(serialized));
    if (wz_state_serialize_machine(&machine, &writer) != WZ_RESULT_OK ||
        wz_state_hash_machine(&machine, &first_hash) != WZ_RESULT_OK) {
        fputs("canonical state baseline refresh failed\n", stderr);
        return 1;
    }
    machine.memory[0] = 0x42u;
    if (wz_state_hash_machine(&machine, &second_hash) != WZ_RESULT_OK ||
        first_hash == second_hash) {
        fputs("canonical state hash did not reflect machine state\n", stderr);
        return 1;
    }
    if (wz_state_deserialize_machine(&restored, serialized, writer.length) != WZ_RESULT_OK ||
        wz_state_hash_machine(&restored, &second_hash) != WZ_RESULT_OK ||
        first_hash != second_hash) {
        fputs("canonical state round trip failed\n", stderr);
        return 1;
    }

    wz_trace_sink_init(&trace_sink, record_trace, &trace_count);
    if (wz_headless_runner_init(&runner, &machine, &trace_sink) != WZ_RESULT_OK ||
        wz_headless_runner_advance(&runner, 3u) != WZ_RESULT_OK ||
        machine.master_tick != 3u || trace_count != 3u) {
        fputs("headless runner or trace sink failed\n", stderr);
        return 1;
    }

    wz_scheduler_init(&scheduler);
    if (wz_scheduler_schedule(&scheduler, 10u, WZ_EVENT_EXTERNAL,
                              record_event, &dispatched) != WZ_RESULT_OK ||
        wz_scheduler_schedule(&scheduler, 10u, WZ_EVENT_CPU,
                              record_event, &dispatched) != WZ_RESULT_OK ||
        wz_scheduler_dispatch_next(&scheduler) != WZ_RESULT_OK ||
        dispatched != 1u) {
        fputs("same-tick scheduler ordering failed\n", stderr);
        return 1;
    }

    if (strstr(wz_machine_boot_message(), "Warajevo") == NULL) {
        fputs("bootstrap message does not identify the project\n", stderr);
        return 1;
    }

    remove(trace_path);
    if (wz_trace_file_create(&trace_file, trace_path, 1u,
                             (wz_dword_t)profile->kind, 0x1234u, UINT32_MAX) != WZ_RESULT_OK ||
        wz_trace_file_create(&duplicate, trace_path, 2u,
                             (wz_dword_t)profile->kind, 0x1234u, UINT32_MAX) == WZ_RESULT_OK) {
        fputs("exclusive trace creation failed\n", stderr);
        return 1;
    }
    wz_trace_sink_init(&trace_sink, wz_trace_file_emit, &trace_file);
    for (wz_qword_t index = 0u; index < 800000u; ++index) {
        wz_trace_emit(&trace_sink, WZ_TRACE_MASTER_TICK_ADVANCED, index);
    }
    if (wz_trace_file_freeze(&trace_file) != WZ_RESULT_OK) {
        fputs("trace freeze failed\n", stderr);
        return 1;
    }
    wz_trace_emit(&trace_sink, WZ_TRACE_MASTER_TICK_ADVANCED, 800001u);
    wz_trace_file_close(&trace_file);
    if (wz_trace_file_recover(trace_path, recover_trace, &recovered_last,
                              &recovered_count) != WZ_RESULT_OK ||
        recovered_count < (8u * 69888u) || recovered_last != 799999u) {
        fputs("trace wrap recovery failed\n", stderr);
        return 1;
    }
    trace_stream = fopen(trace_path, "r+b");
    if (trace_stream == NULL || fseek(trace_stream, 0, SEEK_END) != 0 ||
        ftell(trace_stream) != (long)WZ_TRACE_FILE_SIZE ||
        fseek(trace_stream,
              (long)(WZ_TRACE_HEADER_SIZE +
                     (799999u % ((WZ_TRACE_FILE_SIZE - WZ_TRACE_HEADER_SIZE) /
                                  WZ_TRACE_RECORD_SIZE)) * WZ_TRACE_RECORD_SIZE +
                     WZ_TRACE_COMMIT_OFFSET),
              SEEK_SET) != 0 || fputc(0, trace_stream) == EOF ||
        fclose(trace_stream) != 0) {
        fputs("trace size or truncation fixture failed\n", stderr);
        return 1;
    }
    recovered_last = 0u;
    if (wz_trace_file_recover(trace_path, recover_trace, &recovered_last,
                              &recovered_count) != WZ_RESULT_OK ||
        recovered_last != 799998u) {
        fputs("incomplete trace record was not rejected\n", stderr);
        return 1;
    }
    remove(trace_path);

    remove(state_trace_path);
    if (wz_trace_file_create(&trace_file, state_trace_path, 3u,
                             (wz_dword_t)profile->kind, 0x5678u, UINT32_MAX) != WZ_RESULT_OK) {
        fputs("state trace creation failed\n", stderr);
        return 1;
    }
    wz_trace_sink_init(&trace_sink, wz_trace_file_emit, &trace_file);
    for (wz_byte_t chunk = 0u; chunk < 5u; ++chunk) {
        static const wz_qword_t state_chunks[5] = {
            UINT64_C(0x8877665544332211),
            UINT64_C(0x0123456789abcdef),
            UINT64_C(0x13572468def09abc),
            UINT64_C(0x020101013c9a2468),
            UINT64_C(0x0000000000000000)
        };
        wz_trace_event_t state_event = {0};
        state_event.kind = WZ_TRACE_CPU_STATE_SYNC;
        state_event.master_tick = 1234u;
        state_event.cycle = chunk;
        state_event.register_snapshot = state_chunks[chunk];
        wz_trace_emit_detail(&trace_sink, &state_event);
    }
    {
        wz_trace_event_t state_delta = {0};
        state_delta.kind = WZ_TRACE_CPU_STATE_DELTA;
        state_delta.master_tick = 1242u;
        state_delta.cycle = 2u;
        state_delta.register_snapshot = UINT64_C(0xbeef2468def09abc);
        wz_trace_emit_detail(&trace_sink, &state_delta);
    }
    if (wz_trace_file_freeze(&trace_file) != WZ_RESULT_OK) {
        fputs("state trace freeze failed\n", stderr);
        return 1;
    }
    wz_trace_file_close(&trace_file);
    memset(&timing_trace_log, 0, sizeof(timing_trace_log));
    if (wz_trace_file_recover(state_trace_path, recover_timing_trace, &timing_trace_log,
                              &recovered_count) != WZ_RESULT_OK ||
        recovered_count != 6u || timing_trace_log.count != 6u ||
        timing_trace_log.events[0].kind != WZ_TRACE_CPU_STATE_SYNC ||
        timing_trace_log.events[0].register_snapshot != UINT64_C(0x8877665544332211) ||
        timing_trace_log.events[0].master_tick != 1234u ||
        timing_trace_log.events[4].master_tick != 1234u ||
        timing_trace_log.events[4].cycle != 4u ||
        timing_trace_log.events[4].register_snapshot != 0u ||
        timing_trace_log.events[5].kind != WZ_TRACE_CPU_STATE_DELTA ||
        timing_trace_log.events[5].master_tick != 1242u ||
        timing_trace_log.events[5].cycle != 2u) {
        fputs("state trace recovery failed\n", stderr);
        return 1;
    }
    wz_trace_cpu_state_sync_init(&recovered_cpu_sync);
    {
        bool recovered_state = false;
    for (size_t index = 0u; index < timing_trace_log.count; ++index) {
            recovered_state = wz_trace_cpu_state_sync_apply(&recovered_cpu_sync,
                                                             &timing_trace_log.events[index]);
        }
        if (!recovered_state || recovered_cpu_sync.master_tick != 1242u ||
            recovered_cpu_sync.state.main.a != 0x11u ||
            recovered_cpu_sync.state.main.l != 0x88u ||
            recovered_cpu_sync.state.alternate.a != 0xefu ||
            recovered_cpu_sync.state.alternate.l != 0x01u ||
            recovered_cpu_sync.state.ix != 0x9abcu ||
            recovered_cpu_sync.state.iy != 0xdef0u ||
            recovered_cpu_sync.state.stack_pointer != 0x2468u ||
            recovered_cpu_sync.state.program_counter != 0xbeefu ||
            recovered_cpu_sync.state.memptr != 0x2468u ||
            recovered_cpu_sync.state.i != 0x9au || recovered_cpu_sync.state.r != 0x3cu ||
            recovered_cpu_sync.state.iff1 != 1u || recovered_cpu_sync.state.iff2 != 1u ||
            recovered_cpu_sync.state.interrupt_enable_delay != 1u ||
            recovered_cpu_sync.state.interrupt_mode != WZ_Z80_INTERRUPT_MODE_2 ||
            recovered_cpu_sync.state.halted != 0u) {
            fprintf(stderr, "state trace decoder failed: complete=%u tick=%llu pc=%04x sp=%04x im=%u\n",
                    recovered_state ? 1u : 0u,
                    (unsigned long long)recovered_cpu_sync.master_tick,
                    (unsigned)recovered_cpu_sync.state.program_counter,
                    (unsigned)recovered_cpu_sync.state.stack_pointer,
                    (unsigned)recovered_cpu_sync.state.interrupt_mode);
            return 1;
        }
    }
    remove(state_trace_path);

    wz_machine_destroy(&machine);
    wz_machine_destroy(&restored);
    return 0;
}
