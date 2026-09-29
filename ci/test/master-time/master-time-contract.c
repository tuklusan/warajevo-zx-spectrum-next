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

#include "core/wz_bus.h"
#include "core/wz_machine.h"
#include "core/wz_machine_profile.h"
#include "core/wz_types.h"
#include "core/wz_z80.h"

_Static_assert(sizeof(wz_master_tick_t) == 8u,
               "master time must be represented by a 64-bit integer");

typedef struct {
    wz_bus_request_t requests[3];
    size_t count;
    bool overflow;
} bus_capture_t;

static void capture_bus_request(const wz_bus_request_t* request, void* context)
{
    bus_capture_t* capture = (bus_capture_t*)context;
    if (capture->count >= sizeof(capture->requests) / sizeof(capture->requests[0])) {
        capture->overflow = true;
        return;
    }
    capture->requests[capture->count++] = *request;
}

static bool check_bus_timing(const wz_machine_profile_t* profile)
{
    wz_machine_t machine;
    wz_bus_observer_t observer;
    bus_capture_t capture;
    wz_master_tick_t tstate_ticks;

    memset(&machine, 0, sizeof(machine));
    memset(&capture, 0, sizeof(capture));
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        return false;
    }
    wz_machine_memory_write(&machine, 0x8000u, 0xd3u);
    wz_machine_memory_write(&machine, 0x8001u, 0xfeu);
    machine.cpu.program_counter = 0x8000u;
    machine.cpu.main.a = 0x12u;
    wz_bus_observer_init(&observer, capture_bus_request, &capture);
    if (wz_machine_set_bus_observer(&machine, &observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK) {
        wz_machine_destroy(&machine);
        return false;
    }

    tstate_ticks = profile->master_ticks_per_cpu_tstate;
    if (capture.overflow || capture.count != 3u ||
        capture.requests[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        capture.requests[0].master_tick != 0u ||
        capture.requests[0].address != 0x8000u ||
        capture.requests[0].t_states != 4u ||
        capture.requests[1].cycle != WZ_BUS_MEMORY_READ ||
        capture.requests[1].master_tick != (wz_master_tick_t)4u * tstate_ticks ||
        capture.requests[1].address != 0x8001u ||
        capture.requests[1].value != 0xfeu ||
        capture.requests[1].t_states != 3u ||
        capture.requests[2].cycle != WZ_BUS_IO_WRITE ||
        capture.requests[2].master_tick != (wz_master_tick_t)7u * tstate_ticks ||
        capture.requests[2].address != 0x12feu ||
        capture.requests[2].value != 0x12u ||
        capture.requests[2].t_states != 4u ||
        machine.master_tick != (wz_master_tick_t)11u * tstate_ticks) {
        wz_machine_destroy(&machine);
        return false;
    }

    wz_machine_destroy(&machine);
    return true;
}

static bool check_profile(const wz_machine_profile_t* profile)
{
    wz_machine_t machine;
    wz_master_tick_t expected_frame_ticks;

    if (profile == NULL || profile->master_ticks_per_cpu_tstate == 0u ||
        profile->master_hz_num == 0u || profile->master_hz_den == 0u ||
        profile->tstates_per_line == 0u || profile->lines_per_frame == 0u ||
        profile->tstates_per_frame !=
            (wz_qword_t)profile->tstates_per_line * profile->lines_per_frame ||
        profile->tstates_per_frame >
            UINT64_MAX / profile->master_ticks_per_cpu_tstate) {
        return false;
    }

    expected_frame_ticks =
        (wz_master_tick_t)profile->tstates_per_frame *
        profile->master_ticks_per_cpu_tstate;
    for (wz_master_tick_t tick = 0u; tick < expected_frame_ticks; ++tick) {
        if (wz_profile_cpu_tstate(tick, profile) !=
                tick / profile->master_ticks_per_cpu_tstate ||
            wz_profile_cpu_phase(tick, profile) !=
                tick % profile->master_ticks_per_cpu_tstate) {
            return false;
        }
    }
    if (wz_profile_cpu_tstate(expected_frame_ticks, profile) !=
            expected_frame_ticks / profile->master_ticks_per_cpu_tstate ||
        wz_profile_cpu_phase(expected_frame_ticks, profile) !=
            expected_frame_ticks % profile->master_ticks_per_cpu_tstate) {
        return false;
    }

    memset(&machine, 0, sizeof(machine));
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        return false;
    }
    for (size_t index = 0u; index < 256u; ++index) {
        wz_machine_memory_write(&machine, (wz_word_t)(0x8000u + index), 0x00u);
    }
    machine.cpu.program_counter = 0x8000u;

    for (size_t instruction = 0u; instruction < 256u; ++instruction) {
        wz_master_tick_t before = machine.master_tick;
        wz_master_tick_t expected_delta =
            (wz_master_tick_t)4u * profile->master_ticks_per_cpu_tstate;
        if (wz_z80_step(&machine) != WZ_RESULT_OK ||
            machine.master_tick - before != expected_delta ||
            machine.master_tick % profile->master_ticks_per_cpu_tstate != 0u) {
            wz_machine_destroy(&machine);
            return false;
        }
    }

    wz_machine_destroy(&machine);
    return check_bus_timing(profile);
}

int main(void)
{
    if (!check_profile(wz_machine_profile_48k_pal()) ||
        !check_profile(wz_machine_profile_128k_pal())) {
        fputs("Master-time contract failed.\n", stderr);
        return 1;
    }
    puts("Master-time contract passed for both certified PAL profiles.");
    return 0;
}
