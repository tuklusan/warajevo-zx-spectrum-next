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

static bool recover_trace(const wz_trace_event_t* event, void* context)
{
    wz_qword_t* last_sequence = (wz_qword_t*)context;
    *last_sequence = event->sequence;
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
    wz_bus_request_t bus_request;
    bus_log_t bus_log;
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
    FILE* trace_stream;
    const char* trace_path = "wz-trace-regression.bin";

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
    if (implemented != 3u || prefix != 4u || documented_unimplemented != 249u ||
        undocumented != 0u || illegal != 0u ||
        wz_z80_primary_opcode_decode(0x00u)->operation != WZ_Z80_PRIMARY_OP_NOP ||
        wz_z80_primary_opcode_decode(0x32u)->operation != WZ_Z80_PRIMARY_OP_LD_NN_A ||
        wz_z80_primary_opcode_decode(0x3eu)->operation != WZ_Z80_PRIMARY_OP_LD_A_N ||
        wz_z80_primary_opcode_decode(0xcbu)->operation != WZ_Z80_PRIMARY_OP_PREFIX_CB ||
        wz_z80_primary_opcode_decode(0xddu)->operation != WZ_Z80_PRIMARY_OP_PREFIX_DD ||
        wz_z80_primary_opcode_decode(0xedu)->operation != WZ_Z80_PRIMARY_OP_PREFIX_ED ||
        wz_z80_primary_opcode_decode(0xfdu)->operation != WZ_Z80_PRIMARY_OP_PREFIX_FD) {
        fputs("primary opcode table contents failed\n", stderr);
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
    machine.memory[0u] = 0x01u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_UNSUPPORTED_OPERATION ||
        machine.cpu.program_counter != 1u ||
        machine.master_tick != 0u ||
        bus_log.count != 1u ||
        bus_log.requests[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[0].address != 0u ||
        bus_log.requests[0].value != 0x01u) {
        fputs("Z80 unsupported opcode trace failed\n", stderr);
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

    wz_state_writer_init(&writer, serialized, sizeof(serialized));
    if (wz_state_serialize_machine(&machine, &writer) != WZ_RESULT_OK ||
        writer.length != 65578u ||
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
                                  WZ_TRACE_RECORD_SIZE)) * WZ_TRACE_RECORD_SIZE + 20u),
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

    wz_machine_destroy(&machine);
    wz_machine_destroy(&restored);
    return 0;
}
