/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "core/wz_machine.h"

#include <stdio.h>

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "Tape Ear+Mic exclusion failed at line %d: %s\n", \
                      __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void)
{
    wz_machine_t machine;
    const wz_tape_segment_t segments[] = {{8u, 1u, 0u}};
    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    REQUIRE(wz_machine_mount_tape(&machine, segments,
                                  sizeof(segments) / sizeof(segments[0])) ==
            WZ_RESULT_OK);
    machine.networking_mode = WZ_NETWORKING_EAR_MIC;
    REQUIRE(wz_tape_state_set_motor(&machine.tape_state, true) == WZ_RESULT_OK);
    REQUIRE(machine.tape_state.motor_on && machine.tape_state.segment_index == 0u);

    REQUIRE(wz_machine_set_tape_motor(&machine, false) == WZ_RESULT_INVALID_STATE);
    REQUIRE(machine.tape_state.motor_on && machine.tape_state.segment_index == 0u);
    REQUIRE(wz_machine_rewind_tape(&machine) == WZ_RESULT_INVALID_STATE);
    REQUIRE(machine.tape_state.segment_index == 0u &&
            machine.tape_state.segment_elapsed == 0u);
    REQUIRE(wz_machine_advance_tape(&machine, 100u) == WZ_RESULT_OK);
    REQUIRE(machine.tape_state.segment_index == 0u &&
            machine.tape_state.segment_elapsed == 0u);
    REQUIRE(wz_machine_tape_ear_level(&machine) == 0u);

    wz_machine_destroy(&machine);
    (void)puts("Tape Ear+Mic exclusion passed");
    return 0;
}
