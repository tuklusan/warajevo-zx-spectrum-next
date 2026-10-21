/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "core/wz_machine.h"
#include "core/wz_machine_internal.h"
#include "core/wz_runner.h"
#include "core/wz_trace.h"
#include "core/wz_state.h"

#include <stdio.h>
#include <string.h>

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "Interface 1 WAIT contract failed at line %d: %s\n", \
                      __LINE__, #condition); \
        goto cleanup; \
    } \
    ++checks; \
} while (0)

typedef struct {
    size_t ula_fetches;
} trace_counts_t;

static void count_trace_event(const wz_trace_event_t* event, void* context)
{
    trace_counts_t* counts = (trace_counts_t*)context;
    if (event->kind == WZ_TRACE_ULA_FETCH) {
        ++counts->ula_fetches;
    }
}

static wz_result_t prepare_machine(wz_machine_t* machine,
                                   const wz_byte_t* rom,
                                   wz_byte_t opcode,
                                   wz_byte_t operand)
{
    if (wz_machine_init(machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK ||
        wz_machine_load_interface1_rom(machine, rom, WZ_INTERFACE1_ROM_SIZE,
            WZ_INTERFACE1_ROM_OLD) != WZ_RESULT_OK ||
        wz_machine_set_networking_mode(machine,
            WZ_NETWORKING_INTERFACE1) != WZ_RESULT_OK ||
        wz_machine_interface1_control_write(machine, 0x20u, 0u) !=
            WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }
    machine->memory[0] = opcode;
    machine->memory[1] = operand;
    machine->cpu.program_counter = 0u;
    return WZ_RESULT_OK;
}

static wz_result_t provide_input(wz_machine_t* machine,
                                 const wz_byte_t* block)
{
    if (wz_zxnet_feed_read_block(&machine->zxnet, 1u, block,
            WZ_ZXNET_DATA_CAPACITY) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }
    return wz_zxnet_start_read_collection(&machine->zxnet);
}

static wz_result_t provide_output(wz_machine_t* machine)
{
    if (wz_zxnet_begin_claim(&machine->zxnet, 0x42u) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }
    return wz_zxnet_accept_claim(&machine->zxnet, true);
}

int main(void)
{
    static wz_machine_t machine;
    static wz_machine_t restored;
    static wz_byte_t rom[WZ_INTERFACE1_ROM_SIZE];
    static wz_byte_t block[WZ_ZXNET_DATA_CAPACITY];
    static wz_snapshot_state_t snapshot;
    wz_headless_runner_t runner;
    wz_trace_sink_t trace_sink;
    trace_counts_t trace_counts = {0};
    wz_qword_t machine_hash = 0u;
    wz_qword_t restored_hash = 0u;
    wz_master_tick_t runner_start_tick;
    size_t ula_fetches_before_wait;
    unsigned checks = 0u;
    int result = 1;

    memset(rom, 0x5au, sizeof(rom));
    memset(block, 0xa5u, sizeof(block));
    block[0] = 0x01u;
    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    REQUIRE(wz_machine_load_interface1_rom(&machine, rom, sizeof(rom),
            WZ_INTERFACE1_ROM_OLD) == WZ_RESULT_OK);
    REQUIRE(wz_machine_set_networking_mode(&machine,
            WZ_NETWORKING_INTERFACE1) == WZ_RESULT_OK);
    REQUIRE(wz_machine_interface1_control_write(&machine, 0x20u, 0u) ==
            WZ_RESULT_OK);
    wz_trace_sink_init(&trace_sink, count_trace_event, &trace_counts);
    wz_machine_set_timing_trace(&machine, &trace_sink);
    wz_machine_advance_master_ticks(&machine,
        (wz_master_tick_t)machine.profile->ula_fetch_start_tstate * 2u - 40u);
    machine.memory[0] = 0xedu;
    machine.memory[1] = 0x40u; /* IN B,(C) */
    machine.cpu.main.c = 0xf7u;
    machine.cpu.q = 1u;
    machine.cpu.program_counter = 0u;
    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_PENDING);
    REQUIRE(machine.interface1_pending_io.active == 1u);
    REQUIRE(machine.cpu.program_counter == 2u);
    REQUIRE(machine.master_tick >= 16u);
    runner_start_tick = machine.master_tick;
    ula_fetches_before_wait = trace_counts.ula_fetches;
    REQUIRE(wz_headless_runner_init(&runner, &machine, 0) == WZ_RESULT_OK);
    REQUIRE(wz_headless_runner_execute(&runner, 100u) == WZ_RESULT_OK);
    REQUIRE(machine.master_tick >= runner_start_tick + 100u);
    REQUIRE(trace_counts.ula_fetches > ula_fetches_before_wait);
    REQUIRE(machine.interface1_pending_io.active == 1u);
    REQUIRE(wz_snapshot_state_capture(&snapshot, &machine) == WZ_RESULT_OK);

    REQUIRE(wz_machine_init(&restored, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    REQUIRE(wz_machine_load_interface1_rom(&restored, rom, sizeof(rom),
            WZ_INTERFACE1_ROM_OLD) == WZ_RESULT_OK);
    REQUIRE(wz_machine_set_networking_mode(&restored,
            WZ_NETWORKING_INTERFACE1) == WZ_RESULT_OK);
    REQUIRE(wz_state_deserialize_machine(&restored, snapshot.data,
            snapshot.length) == WZ_RESULT_OK);
    REQUIRE(restored.interface1_pending_io.active == 1u);
    REQUIRE(restored.master_tick == machine.master_tick);

    REQUIRE(wz_zxnet_feed_read_block(&machine.zxnet, 1u, block,
            sizeof(block)) == WZ_RESULT_OK);
    REQUIRE(wz_zxnet_start_read_collection(&machine.zxnet) == WZ_RESULT_OK);
    REQUIRE(wz_zxnet_feed_read_block(&restored.zxnet, 1u, block,
            sizeof(block)) == WZ_RESULT_OK);
    REQUIRE(wz_zxnet_start_read_collection(&restored.zxnet) == WZ_RESULT_OK);
    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_OK);
    REQUIRE(wz_z80_step(&restored) == WZ_RESULT_OK);
    REQUIRE(machine.interface1_pending_io.active == 0u);
    REQUIRE(restored.interface1_pending_io.active == 0u);
    REQUIRE(machine.cpu.main.b == 0xffu);
    REQUIRE(restored.cpu.main.b == machine.cpu.main.b);
    REQUIRE(wz_state_hash_machine(&machine, &machine_hash) == WZ_RESULT_OK);
    REQUIRE(wz_state_hash_machine(&restored, &restored_hash) == WZ_RESULT_OK);
    REQUIRE(machine_hash == restored_hash);

    wz_machine_set_timing_trace(&machine, 0);
    wz_machine_destroy(&machine);
    wz_machine_destroy(&restored);
    REQUIRE(prepare_machine(&machine, rom, 0xdbu, 0xf7u) == WZ_RESULT_OK);
    machine.cpu.main.a = 0u;
    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_PENDING);
    REQUIRE(provide_input(&machine, block) == WZ_RESULT_OK);
    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_OK);
    REQUIRE(machine.cpu.main.a == 0xffu);

    wz_machine_destroy(&machine);
    REQUIRE(prepare_machine(&machine, rom, 0xedu, 0xa2u) == WZ_RESULT_OK);
    machine.cpu.main.b = 1u;
    machine.cpu.main.c = 0xf7u;
    machine.cpu.main.h = 0x40u;
    machine.cpu.main.l = 0u;
    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_PENDING);
    REQUIRE(provide_input(&machine, block) == WZ_RESULT_OK);
    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_OK);
    REQUIRE(machine.memory[0x4000u] == 0xffu);
    REQUIRE(machine.cpu.main.b == 0u);
    REQUIRE(machine.cpu.main.l == 1u);
    REQUIRE(machine.cpu.memptr == 0x01f8u);

    wz_machine_destroy(&machine);
    REQUIRE(prepare_machine(&machine, rom, 0xedu, 0xb2u) == WZ_RESULT_OK);
    machine.cpu.main.b = 2u;
    machine.cpu.main.c = 0xf7u;
    machine.cpu.main.h = 0x40u;
    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_PENDING);
    REQUIRE(provide_input(&machine, block) == WZ_RESULT_OK);
    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_OK);
    REQUIRE(machine.cpu.main.b == 1u);
    REQUIRE(machine.cpu.program_counter == 0u);

    wz_machine_destroy(&machine);
    REQUIRE(prepare_machine(&machine, rom, 0xd3u, 0xf7u) == WZ_RESULT_OK);
    machine.cpu.main.a = 0u;
    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_PENDING);
    REQUIRE(provide_output(&machine) == WZ_RESULT_OK);
    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_OK);
    REQUIRE(machine.zxnet.state.bit_count == 8u);

    wz_machine_destroy(&machine);
    REQUIRE(prepare_machine(&machine, rom, 0xedu, 0x41u) == WZ_RESULT_OK);
    machine.cpu.main.c = 0xf7u;
    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_PENDING);
    REQUIRE(provide_output(&machine) == WZ_RESULT_OK);
    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_OK);
    REQUIRE(machine.zxnet.state.bit_count == 8u);

    wz_machine_destroy(&machine);
    REQUIRE(prepare_machine(&machine, rom, 0xedu, 0xa3u) == WZ_RESULT_OK);
    machine.cpu.main.b = 1u;
    machine.cpu.main.c = 0xf7u;
    machine.cpu.main.h = 0x40u;
    machine.memory[0x4000u] = 0u;
    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_PENDING);
    REQUIRE(provide_output(&machine) == WZ_RESULT_OK);
    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_OK);
    REQUIRE(machine.zxnet.state.bit_count == 8u);
    REQUIRE(machine.cpu.main.l == 1u);

    wz_machine_destroy(&machine);
    REQUIRE(prepare_machine(&machine, rom, 0xedu, 0xb3u) == WZ_RESULT_OK);
    machine.cpu.main.b = 2u;
    machine.cpu.main.c = 0xf7u;
    machine.cpu.main.h = 0x40u;
    machine.memory[0x4000u] = 0u;
    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_PENDING);
    REQUIRE(provide_output(&machine) == WZ_RESULT_OK);
    REQUIRE(wz_z80_step(&machine) == WZ_RESULT_OK);
    REQUIRE(machine.zxnet.state.bit_count == 8u);
    REQUIRE(machine.cpu.main.b == 1u);
    REQUIRE(machine.cpu.program_counter == 0u);

    (void)printf("PASS Interface 1 WAIT pending-cycle resume (%u checks)\n",
                 checks);
    result = 0;

cleanup:
    wz_machine_destroy(&machine);
    wz_machine_destroy(&restored);
    return result;
}
