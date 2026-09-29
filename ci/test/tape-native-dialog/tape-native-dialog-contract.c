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

#include "app/wz_file_dialog.h"
#include "core/wz_machine.h"
#include "core/wz_machine_profile.h"

static bool same_tape_state(const wz_tape_state_t* left,
                            const wz_tape_state_t* right)
{
    return left->tape == right->tape &&
        left->segment_index == right->segment_index &&
        left->segment_elapsed == right->segment_elapsed &&
        left->ear_level == right->ear_level &&
        left->motor_on == right->motor_on && left->at_end == right->at_end;
}

int main(void)
{
    static const wz_tape_segment_t segments[] = {{41u, 1u, 0u}};
    wz_machine_t machine = {0};
    wz_tape_t saved_tape;
    wz_tape_state_t saved_state;
    wz_byte_t saved_mounted;
    char path[4096];
    wz_file_dialog_result_t result;

    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) !=
            WZ_RESULT_OK ||
        wz_machine_mount_tape(&machine, segments, 1u) != WZ_RESULT_OK) {
        (void)fprintf(stderr, "unable to prepare mounted tape state\n");
        return 1;
    }
    machine.tape_state.segment_elapsed = 17u;
    machine.tape_state.motor_on = true;
    saved_tape = machine.tape;
    saved_state = machine.tape_state;
    saved_mounted = machine.tape_mounted;
    memset(path, 0x5a, sizeof(path));

    result = wz_file_dialog_open(path, sizeof(path));
    if (result != WZ_FILE_DIALOG_CANCELLED || path[0] != '\0' ||
        machine.tape.segments != saved_tape.segments ||
        machine.tape.segment_count != saved_tape.segment_count ||
        !same_tape_state(&machine.tape_state, &saved_state) ||
        machine.tape_mounted != saved_mounted) {
        wz_machine_destroy(&machine);
        (void)fprintf(stderr, "chooser cancellation changed result or tape state\n");
        return 1;
    }
    wz_machine_destroy(&machine);
    (void)puts("PASS: native chooser cancellation clears output and preserves mounted transport");
    return 0;
}
