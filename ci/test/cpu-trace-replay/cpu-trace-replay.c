/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "core/wz_machine.h"
#include "diagnostics/wz_trace_file.h"

#define MAX_BUS 64u
#define MAX_BOUNDARIES 16u

typedef struct {
    wz_machine_t* machine;
    wz_z80_state_t boundaries[MAX_BOUNDARIES];
    wz_byte_t prior_q[MAX_BOUNDARIES];
    wz_bus_request_t bus[MAX_BUS];
    size_t boundary_count;
    size_t bus_count;
    bool overflow;
} live_capture_t;

typedef struct {
    const live_capture_t* live;
    wz_trace_cpu_state_sync_t sync;
    wz_qword_t previous_sequence;
    wz_master_tick_t previous_tick;
    size_t events;
    size_t bus_index;
    size_t boundary_index;
    size_t program_byte_index;
    unsigned opcode_bytes;
    unsigned m1_fetches;
    unsigned sync_groups;
    unsigned delta_chunks;
    unsigned nmi_accepts;
    bool saw_prior_q;
    bool valid;
} replay_t;

static void record_bus(const wz_bus_request_t* request, void* context)
{
    live_capture_t* live = (live_capture_t*)context;
    if (live->bus_count == MAX_BUS) live->overflow = true;
    else live->bus[live->bus_count++] = *request;
}

static void record_live(const wz_trace_event_t* event, void* context)
{
    live_capture_t* live = (live_capture_t*)context;
    if (event->kind != WZ_TRACE_CPU_INSTRUCTION) return;
    if (live->boundary_count == MAX_BOUNDARIES) {
        live->overflow = true;
        return;
    }
    live->boundaries[live->boundary_count] = live->machine->cpu;
    live->prior_q[live->boundary_count] = live->machine->z80_prior_q;
    ++live->boundary_count;
}

static bool banks_equal(const wz_z80_register_bank_t* a,
                        const wz_z80_register_bank_t* b)
{
    return a->a == b->a && a->f == b->f && a->b == b->b && a->c == b->c &&
           a->d == b->d && a->e == b->e && a->h == b->h && a->l == b->l;
}

static bool states_equal(const wz_z80_state_t* a, const wz_z80_state_t* b)
{
    return banks_equal(&a->main, &b->main) &&
           banks_equal(&a->alternate, &b->alternate) &&
           a->ix == b->ix && a->iy == b->iy &&
           a->stack_pointer == b->stack_pointer &&
           a->program_counter == b->program_counter &&
           a->memptr == b->memptr && a->i == b->i && a->r == b->r &&
           a->iff1 == b->iff1 && a->iff2 == b->iff2 &&
           a->interrupt_enable_delay == b->interrupt_enable_delay &&
           a->interrupt_mode == b->interrupt_mode &&
           a->halted == b->halted && a->q == b->q;
}

static bool check_boundary(replay_t* replay)
{
    const size_t index = replay->boundary_index - 1u;
    if (!replay->sync.has_absolute_state ||
        !states_equal(&replay->sync.state, &replay->live->boundaries[index]) ||
        replay->sync.prior_q != replay->live->prior_q[index]) {
        (void)fprintf(stderr,
            "replay state mismatch at boundary=%zu PC=%04x/%04x "
            "R=%02x/%02x Q=%u/%u priorQ=%u/%u absolute=%u\n",
            index, replay->sync.state.program_counter,
            replay->live->boundaries[index].program_counter,
            replay->sync.state.r, replay->live->boundaries[index].r,
            replay->sync.state.q, replay->live->boundaries[index].q,
            replay->sync.prior_q, replay->live->prior_q[index],
            (unsigned)replay->sync.has_absolute_state);
        return false;
    }
    if (replay->sync.prior_q == 1u) replay->saw_prior_q = true;
    return true;
}

static bool replay_event(const wz_trace_event_t* event, void* context)
{
    replay_t* replay = (replay_t*)context;
    if (replay->events != 0u &&
        (event->sequence <= replay->previous_sequence ||
         event->master_tick < replay->previous_tick)) goto failed;
    replay->previous_sequence = event->sequence;
    replay->previous_tick = event->master_tick;
    ++replay->events;

    if (event->kind == WZ_TRACE_CPU_BUS) {
        const wz_bus_request_t* bus;
        if (replay->bus_index >= replay->live->bus_count) goto failed;
        bus = &replay->live->bus[replay->bus_index++];
        if (event->cycle != (wz_byte_t)bus->cycle ||
            event->master_tick != bus->master_tick ||
            event->address != bus->address || event->value != bus->value ||
            event->t_states != bus->t_states) goto failed;
        if (event->cycle == WZ_BUS_M1_OPCODE_FETCH) ++replay->m1_fetches;
        if (event->address >= 0x8000u && event->address <= 0x8013u &&
            (event->cycle == WZ_BUS_M1_OPCODE_FETCH ||
             event->cycle == WZ_BUS_MEMORY_READ)) {
            const wz_word_t expected_address = replay->program_byte_index < 20u ?
                (wz_word_t)(0x8000u + replay->program_byte_index) : 0x8013u;
            if (replay->program_byte_index > 20u ||
                event->address != expected_address) goto failed;
            ++replay->program_byte_index;
        }
    } else if (event->kind == WZ_TRACE_CPU_OPCODE_BYTE) {
        ++replay->opcode_bytes;
    } else if (event->kind == WZ_TRACE_CPU_INSTRUCTION) {
        if (replay->boundary_index != 0u && !check_boundary(replay)) goto failed;
        if (replay->boundary_index >= replay->live->boundary_count) goto failed;
        ++replay->boundary_index;
    } else if (event->kind == WZ_TRACE_CPU_STATE_SYNC ||
               event->kind == WZ_TRACE_CPU_STATE_DELTA) {
        const bool applied = wz_trace_cpu_state_sync_apply(&replay->sync, event);
        if (event->kind == WZ_TRACE_CPU_STATE_SYNC) {
            if (event->cycle == 4u) {
                if (!applied) goto failed;
                ++replay->sync_groups;
            }
        } else {
            if (!applied) goto failed;
            ++replay->delta_chunks;
        }
    } else if (event->kind == WZ_TRACE_INTERRUPT &&
               event->value == WZ_TRACE_INTERRUPT_NMI_ACCEPT) {
        ++replay->nmi_accepts;
    }
    return true;
failed:
    replay->valid = false;
    (void)fprintf(stderr, "replay event mismatch kind=%u seq=%llu tick=%llu "
                          "bus=%zu boundary=%zu program=%zu\n",
        (unsigned)event->kind, (unsigned long long)event->sequence,
        (unsigned long long)event->master_tick, replay->bus_index,
        replay->boundary_index, replay->program_byte_index);
    return false;
}

int main(int argc, char** argv)
{
    static const wz_byte_t program[] = {
        0xfeu, 0x28u, 0x37u, 0x00u, 0x3fu,
        0xddu, 0xfdu, 0xddu, 0x21u, 0x00u, 0x90u,
        0xddu, 0xcbu, 0x01u, 0x30u, 0xedu, 0x44u,
        0xcbu, 0x00u, 0x76u
    };
    wz_machine_t machine;
    wz_trace_file_t file = {0};
    wz_trace_sink_t sink;
    wz_bus_observer_t observer;
    live_capture_t live = {0};
    replay_t replay = {0};
    size_t recovered_count = 0u;
    bool initialized = false;
    bool created = false;
    int result = 1;
    if (argc != 2 || wz_machine_init(&machine, wz_machine_profile_48k_pal()) !=
                     WZ_RESULT_OK) return 1;
    initialized = true;
    live.machine = &machine;
    for (size_t index = 0u; index < sizeof(program); ++index) {
        wz_machine_memory_write(&machine, (wz_word_t)(0x8000u + index),
                                program[index]);
    }
    wz_machine_memory_write(&machine, 0x9001u, 0x12u);
    machine.cpu.program_counter = 0x8000u;
    machine.cpu.stack_pointer = 0xa002u;
    machine.cpu.main.a = 0x30u;
    machine.cpu.main.b = 0x28u;
    machine.cpu.alternate.a = 0x51u;
    machine.cpu.alternate.f = 0x02u;
    machine.cpu.alternate.b = 0x63u;
    machine.cpu.alternate.c = 0x74u;
    machine.cpu.alternate.d = 0x85u;
    machine.cpu.alternate.e = 0x96u;
    machine.cpu.alternate.h = 0xa7u;
    machine.cpu.alternate.l = 0xb8u;
    machine.cpu.i = 0x40u;
    machine.cpu.r = 0x80u;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    if (wz_trace_file_create(&file, argv[1], 1u,
                             (wz_dword_t)machine.profile->kind,
                             0u, UINT32_MAX) != WZ_RESULT_OK) goto cleanup;
    created = true;
    wz_trace_file_set_forwarder(&file, record_live, &live);
    wz_trace_sink_init(&sink, wz_trace_file_emit, &file);
    wz_machine_set_timing_trace(&machine, &sink);
    wz_bus_observer_init(&observer, record_bus, &live);
    if (wz_machine_set_bus_observer(&machine, &observer) != WZ_RESULT_OK) goto cleanup;
    for (unsigned step = 0u; step < 10u; ++step) {
        if (wz_z80_step(&machine) != WZ_RESULT_OK) {
            (void)fprintf(stderr, "CPU step %u failed\n", step);
            goto cleanup;
        }
    }
    if (machine.cpu.halted != 1u ||
        wz_z80_accept_nmi(&machine) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        wz_trace_file_freeze(&file) != WZ_RESULT_OK || file.failed ||
        live.overflow) goto cleanup;
    wz_trace_file_close(&file);
    replay.live = &live;
    replay.valid = true;
    wz_trace_cpu_state_sync_init(&replay.sync);
    if (wz_trace_file_recover(argv[1], replay_event, &replay,
                              &recovered_count) != WZ_RESULT_OK ||
        !replay.valid || replay.boundary_index != live.boundary_count ||
        replay.boundary_index == 0u || !check_boundary(&replay) ||
        replay.bus_index != live.bus_count ||
        replay.program_byte_index != 21u ||
        replay.opcode_bytes != replay.m1_fetches ||
        replay.sync_groups == 0u || replay.delta_chunks == 0u ||
        replay.nmi_accepts != 1u || !replay.saw_prior_q ||
        recovered_count != replay.events) {
        (void)fprintf(stderr, "trace replay summary mismatch boundaries=%zu/%zu "
                              "bus=%zu/%zu code=%zu opcode=%u m1=%u "
                              "sync=%u delta=%u nmi=%u prior=%u\n",
            replay.boundary_index, live.boundary_count,
            replay.bus_index, live.bus_count, replay.program_byte_index,
            replay.opcode_bytes, replay.m1_fetches, replay.sync_groups,
            replay.delta_chunks, replay.nmi_accepts, (unsigned)replay.saw_prior_q);
        goto cleanup;
    }
    (void)printf("{\"testId\":\"cpu-trace-replay\",\"status\":\"pass\","
                 "\"boundaries\":%zu,\"busEvents\":%zu,\"programBytes\":%zu,"
                 "\"opcodeBytes\":%u,\"nmiAccepts\":%u}\n",
                 replay.boundary_index, replay.bus_index,
                 replay.program_byte_index, replay.opcode_bytes,
                 replay.nmi_accepts);
    result = 0;
cleanup:
    if (created) wz_trace_file_close(&file);
    if (initialized) wz_machine_destroy(&machine);
    return result;
}
