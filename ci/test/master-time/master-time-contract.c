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
#include "core/wz_machine_profile.h"
#include "core/wz_types.h"
#include "core/wz_z80.h"

_Static_assert(sizeof(wz_master_tick_t) == 8u,
               "master time must be represented by a 64-bit integer");

static bool check_profile(const wz_machine_profile_t* profile)
{
    wz_machine_t machine;
    wz_master_tick_t expected_frame_ticks;

    if (profile == NULL || profile->master_ticks_per_cpu_tstate == 0u ||
        profile->tstates_per_frame == 0u ||
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
    return true;
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
