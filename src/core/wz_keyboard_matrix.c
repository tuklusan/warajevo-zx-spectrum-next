/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_keyboard_matrix.h"

#include "core/wz_types.h"

void wz_keyboard_matrix_init(wz_keyboard_matrix_t* matrix)
{
    if (matrix == 0) {
        return;
    }
    for (size_t row = 0u; row < WZ_KEYBOARD_MATRIX_ROW_COUNT; ++row) {
        matrix->rows[row] = 0x1fu;
    }
}

bool wz_keyboard_matrix_key_position(wz_keyboard_key_t key,
                                     size_t* row, size_t* column)
{
    if ((size_t)key >= WZ_KEYBOARD_MATRIX_KEY_COUNT || row == 0 || column == 0) {
        return false;
    }
    *row = (size_t)key / WZ_KEYBOARD_MATRIX_KEYS_PER_ROW;
    *column = (size_t)key % WZ_KEYBOARD_MATRIX_KEYS_PER_ROW;
    return true;
}

bool wz_keyboard_matrix_set(wz_keyboard_matrix_t* matrix,
                            wz_keyboard_key_t key, bool pressed)
{
    size_t row;
    size_t column;
    wz_byte_t mask;

    if (matrix == 0 || !wz_keyboard_matrix_key_position(key, &row, &column)) {
        return false;
    }
    mask = (wz_byte_t)(1u << column);
    if (pressed) {
        matrix->rows[row] &= (wz_byte_t)~mask;
    } else {
        matrix->rows[row] |= mask;
    }
    return true;
}

wz_byte_t wz_keyboard_matrix_scan(const wz_keyboard_matrix_t* matrix,
                                  unsigned char row_select)
{
    bool reachable_rows[WZ_KEYBOARD_MATRIX_ROW_COUNT] = {false};
    bool reachable_columns[WZ_KEYBOARD_MATRIX_KEYS_PER_ROW] = {false};
    wz_byte_t result = 0x1fu;

    if (matrix == 0) {
        return result;
    }
    for (size_t start = 0u; start < WZ_KEYBOARD_MATRIX_ROW_COUNT; ++start) {
        bool changed;

        if ((row_select & (unsigned char)(1u << start)) != 0u ||
            reachable_rows[start]) {
            continue;
        }
        reachable_rows[start] = true;
        do {
            changed = false;
            for (size_t row = 0u; row < WZ_KEYBOARD_MATRIX_ROW_COUNT; ++row) {
                if (!reachable_rows[row]) {
                    continue;
                }
                for (size_t column = 0u;
                     column < WZ_KEYBOARD_MATRIX_KEYS_PER_ROW; ++column) {
                    if ((matrix->rows[row] & (wz_byte_t)(1u << column)) != 0u ||
                        reachable_columns[column]) {
                        continue;
                    }
                    reachable_columns[column] = true;
                    changed = true;
                }
            }
            for (size_t row = 0u; row < WZ_KEYBOARD_MATRIX_ROW_COUNT; ++row) {
                if (reachable_rows[row]) {
                    continue;
                }
                for (size_t column = 0u;
                     column < WZ_KEYBOARD_MATRIX_KEYS_PER_ROW; ++column) {
                    if (reachable_columns[column] &&
                        (matrix->rows[row] & (wz_byte_t)(1u << column)) == 0u) {
                        reachable_rows[row] = true;
                        changed = true;
                        break;
                    }
                }
            }
        } while (changed);
        for (size_t column = 0u; column < WZ_KEYBOARD_MATRIX_KEYS_PER_ROW; ++column) {
            if (reachable_columns[column]) {
                result &= (wz_byte_t)~(1u << column);
            }
        }
    }
    return result;
}
