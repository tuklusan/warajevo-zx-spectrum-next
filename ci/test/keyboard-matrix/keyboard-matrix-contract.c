/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "core/wz_keyboard_matrix.h"

#include <stdio.h>

#define REQUIRE(expression) do { \
    if (!(expression)) { \
        fprintf(stderr, "failed: %s\n", #expression); \
        return false; \
    } \
} while (0)

static bool set_position(wz_keyboard_matrix_t* matrix,
                         size_t row, size_t column)
{
    if (row >= WZ_KEYBOARD_MATRIX_ROW_COUNT ||
        column >= WZ_KEYBOARD_MATRIX_KEYS_PER_ROW) return false;
    return wz_keyboard_matrix_set(matrix,
        (wz_keyboard_key_t)(row * WZ_KEYBOARD_MATRIX_KEYS_PER_ROW + column),
        true);
}

static unsigned char select_rows(size_t first, size_t second)
{
    unsigned int selected = 0xffu;
    selected &= ~(1u << first);
    if (second < WZ_KEYBOARD_MATRIX_ROW_COUNT)
        selected &= ~(1u << second);
    return (unsigned char)selected;
}

static bool verify_empty_and_active_low_selection(void)
{
    wz_keyboard_matrix_t matrix;
    wz_keyboard_matrix_init(&matrix);
    REQUIRE(wz_keyboard_matrix_scan(&matrix, 0xffu) == 0x1fu);
    REQUIRE(set_position(&matrix, 0u, 2u));
    REQUIRE(wz_keyboard_matrix_scan(&matrix, 0xfeu) == 0x1bu);
    REQUIRE(wz_keyboard_matrix_scan(&matrix, 0xffu) == 0x1fu);
    return true;
}

static bool verify_simultaneous_row_union(void)
{
    wz_keyboard_matrix_t matrix;
    wz_keyboard_matrix_init(&matrix);
    REQUIRE(set_position(&matrix, 1u, 1u));
    REQUIRE(set_position(&matrix, 3u, 4u));
    REQUIRE(wz_keyboard_matrix_scan(&matrix, select_rows(1u, 3u)) == 0x0du);
    return true;
}

static bool verify_three_corner_ghosting(void)
{
    wz_keyboard_matrix_t matrix;
    wz_keyboard_matrix_init(&matrix);
    REQUIRE(set_position(&matrix, 0u, 0u));
    REQUIRE(set_position(&matrix, 0u, 1u));
    REQUIRE(set_position(&matrix, 1u, 0u));
    /* Three closed contacts bridge the fourth row/column intersection. */
    REQUIRE(wz_keyboard_matrix_scan(&matrix, select_rows(1u, 8u)) == 0x1cu);
    return true;
}

static bool verify_disconnected_unselected_rows(void)
{
    wz_keyboard_matrix_t matrix;
    wz_keyboard_matrix_init(&matrix);
    REQUIRE(set_position(&matrix, 1u, 0u));
    REQUIRE(set_position(&matrix, 3u, 4u));
    REQUIRE(wz_keyboard_matrix_scan(&matrix, select_rows(1u, 8u)) == 0x1eu);
    return true;
}

int main(void)
{
    static const struct {
        const char* name;
        bool (*run)(void);
    } cases[] = {
        {"empty-and-active-low", verify_empty_and_active_low_selection},
        {"simultaneous-row-union", verify_simultaneous_row_union},
        {"three-corner-ghosting", verify_three_corner_ghosting},
        {"disconnected-unselected-rows", verify_disconnected_unselected_rows}
    };
    unsigned passed = 0u;
    for (size_t index = 0u; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        if (!cases[index].run()) {
            fprintf(stderr, "case failed: %s\n", cases[index].name);
            return 1;
        }
        ++passed;
    }
    printf("keyboard-matrix-contract cases=%u status=pass\n", passed);
    return 0;
}
