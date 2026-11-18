/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#include "app/wz_input_projection.h"

#include "core/wz_keyboard_matrix.h"

_Static_assert(WZ_INPUT_ARBITER_KEY_COUNT == WZ_KEYBOARD_MATRIX_KEY_COUNT,
               "host arbiter and Spectrum matrix sizes must match");

bool wz_input_projection_apply_keyboard(wz_input_arbiter_t* arbiter,
                                       wz_machine_t* machine)
{
    if (arbiter == 0 || machine == 0) {
        return false;
    }

    for (size_t index = 0u; index < WZ_KEYBOARD_MATRIX_KEY_COUNT; ++index) {
        size_t row;
        size_t column;
        const wz_keyboard_key_t key = (wz_keyboard_key_t)index;
        const bool state = wz_input_arbiter_key_down(arbiter, index);
        if (!wz_keyboard_matrix_key_position(key, &row, &column) ||
            row > 0xffu || column > 0xffu ||
            wz_machine_set_keyboard_key(machine, (wz_byte_t)row,
                (wz_byte_t)column,
                state) != WZ_RESULT_OK) {
            return false;
        }
        (void)wz_input_arbiter_record_effective(
            arbiter, index, state,
            machine->master_tick);
    }
    return true;
}
