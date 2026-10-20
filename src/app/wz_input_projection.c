/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#include "app/wz_input_projection.h"

#include "core/wz_keyboard_matrix.h"

_Static_assert(WZ_INPUT_ARBITER_KEY_COUNT == WZ_KEYBOARD_MATRIX_KEY_COUNT,
               "host arbiter and Spectrum matrix sizes must match");

bool wz_input_projection_apply_keyboard(const wz_input_arbiter_t* arbiter,
                                       wz_machine_t* machine)
{
    if (arbiter == 0 || machine == 0) {
        return false;
    }

    for (size_t index = 0u; index < WZ_KEYBOARD_MATRIX_KEY_COUNT; ++index) {
        size_t row;
        size_t column;
        const wz_keyboard_key_t key = (wz_keyboard_key_t)index;
        if (!wz_keyboard_matrix_key_position(key, &row, &column) ||
            row > 0xffu || column > 0xffu ||
            wz_machine_set_keyboard_key(machine, (wz_byte_t)row,
                (wz_byte_t)column,
                wz_input_arbiter_key_down(arbiter, index)) != WZ_RESULT_OK) {
            return false;
        }
    }
    return true;
}
