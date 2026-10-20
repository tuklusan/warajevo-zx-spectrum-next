/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_input_arbiter.h"
#include "app/wz_input_projection.h"
#include "app/wz_telnet_input.h"
#include "core/wz_keyboard_matrix.h"
#include "core/wz_machine.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "keyboard/core boundary failed at line %d: %s\n", \
                __LINE__, #condition); \
        goto cleanup; \
    } \
    checks += 1u; \
} while (0)

static bool key_pressed(const wz_machine_t* machine, wz_word_t row_address,
                        unsigned char column)
{
    return (wz_machine_ula_port_fe_read(machine, row_address) &
            (wz_byte_t)(1u << column)) == 0u;
}

int main(void)
{
    wz_machine_t machine;
    wz_input_arbiter_t arbiter;
    unsigned checks = 0u;
    int exit_code = 1;

    memset(&machine, 0, sizeof(machine));
    wz_input_arbiter_init(&arbiter);

    CHECK(!wz_input_projection_apply_keyboard(NULL, &machine));
    CHECK(!wz_input_projection_apply_keyboard(&arbiter, NULL));
    CHECK(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
          WZ_RESULT_OK);
    CHECK(!key_pressed(&machine, 0xfdfeu, 0u));

    CHECK(wz_input_arbiter_set(&arbiter, WZ_INPUT_SOURCE_LOCAL,
                               WZ_KEY_A, true));
    CHECK(wz_telnet_input_set_key(&arbiter, WZ_KEY_A, true));
    CHECK(wz_input_projection_apply_keyboard(&arbiter, &machine));
    CHECK(key_pressed(&machine, 0xfdfeu, 0u));

    CHECK(wz_input_arbiter_set(&arbiter, WZ_INPUT_SOURCE_LOCAL,
                               WZ_KEY_A, false));
    CHECK(!wz_input_arbiter_source_key_down(
        &arbiter, WZ_INPUT_SOURCE_LOCAL, WZ_KEY_A));
    CHECK(wz_input_arbiter_source_key_down(
        &arbiter, WZ_INPUT_SOURCE_TELNET, WZ_KEY_A));
    CHECK(wz_input_projection_apply_keyboard(&arbiter, &machine));
    CHECK(key_pressed(&machine, 0xfdfeu, 0u));

    CHECK(wz_telnet_input_release_all(&arbiter));
    CHECK(wz_input_projection_apply_keyboard(&arbiter, &machine));
    CHECK(!key_pressed(&machine, 0xfdfeu, 0u));
    CHECK(!key_pressed(&machine, 0xfefeu, 0u));
    exit_code = 0;

cleanup:
    wz_machine_destroy(&machine);
    if (exit_code == 0) {
        printf("passed %u app-to-core keyboard projection assertions\n", checks);
    }
    return exit_code;
}
