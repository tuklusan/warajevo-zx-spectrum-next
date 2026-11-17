/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_input_arbiter.h"
#include "app/wz_keyboard_mapping.h"
#include "app/wz_kempston_mapping.h"
#include "app/wz_input_projection.h"
#include "core/wz_kempston.h"
#include "core/wz_keyboard_matrix.h"
#include "core/wz_machine.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

bool wz_input_event_set_local_key(
    const wz_keyboard_mapping_t* keyboard_mapping,
    wz_kempston_mapping_t* kempston_mapping,
    wz_kempston_t* kempston,
    wz_input_arbiter_t* arbiter,
    unsigned char* host_key_down,
    uint16_t* spectrum_key_refcount,
    unsigned host_key,
    bool pressed);
bool wz_input_event_release_local_keys(
    wz_kempston_mapping_t* kempston_mapping,
    wz_kempston_t* kempston,
    wz_input_arbiter_t* arbiter,
    unsigned char* host_key_down,
    uint16_t* spectrum_key_refcount);

#define REQUIRE(expression) do { \
    if (!(expression)) { \
        fprintf(stderr, "input event contract failed at line %d: %s\\n", \
                __LINE__, #expression); \
        return 1; \
    } \
} while (0)

static bool projection_has_key(wz_machine_t* machine, wz_keyboard_key_t key)
{
    size_t row;
    size_t column;

    if (!wz_keyboard_matrix_key_position(key, &row, &column)) return false;
    return (machine->keyboard_rows[row] & (unsigned char)(1u << column)) == 0u;
}

int main(void)
{
    static const unsigned kempston_defaults[WZ_KEMPSTON_MAPPING_CONTROL_COUNT] = {
        100u, 101u, 102u, 103u, 104u
    };
    wz_keyboard_mapping_t mapping;
    wz_kempston_mapping_t joystick_mapping;
    wz_input_arbiter_t arbiter;
    wz_kempston_t kempston;
    unsigned char host_key_down[WZ_KEYBOARD_MAPPING_HOST_KEY_LIMIT];
    uint16_t refcounts[WZ_INPUT_ARBITER_KEY_COUNT];
    wz_machine_t machine;

    memset(host_key_down, 0, sizeof(host_key_down));
    memset(refcounts, 0, sizeof(refcounts));
    wz_keyboard_mapping_init(&mapping);
    wz_kempston_mapping_init(&joystick_mapping, kempston_defaults);
    wz_input_arbiter_init(&arbiter);
    wz_kempston_init(&kempston);
    REQUIRE(wz_keyboard_mapping_bind(&mapping, 65u, WZ_KEY_A));
    REQUIRE(wz_keyboard_mapping_bind(&mapping, 66u, WZ_KEY_A));
    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) == WZ_RESULT_OK);

    REQUIRE(!wz_input_event_set_local_key(
        &mapping, &joystick_mapping, &kempston, &arbiter,
        host_key_down, refcounts, 0u, true));
    REQUIRE(wz_input_event_set_local_key(
        &mapping, &joystick_mapping, &kempston, &arbiter,
        host_key_down, refcounts, 65u, true));
    REQUIRE(wz_input_arbiter_source_key_down(
        &arbiter, WZ_INPUT_SOURCE_LOCAL, WZ_KEY_A));
    REQUIRE(refcounts[WZ_KEY_A] == 1u);
    REQUIRE(wz_input_projection_apply_keyboard(&arbiter, &machine));
    REQUIRE(projection_has_key(&machine, WZ_KEY_A));

    /* Two host aliases own one Spectrum key; one release cannot drop it. */
    REQUIRE(wz_input_event_set_local_key(
        &mapping, &joystick_mapping, &kempston, &arbiter,
        host_key_down, refcounts, 66u, true));
    REQUIRE(refcounts[WZ_KEY_A] == 2u);
    REQUIRE(wz_input_event_set_local_key(
        &mapping, &joystick_mapping, &kempston, &arbiter,
        host_key_down, refcounts, 65u, false));
    REQUIRE(refcounts[WZ_KEY_A] == 1u);
    REQUIRE(wz_input_arbiter_key_down(&arbiter, WZ_KEY_A));
    REQUIRE(wz_input_event_set_local_key(
        &mapping, &joystick_mapping, &kempston, &arbiter,
        host_key_down, refcounts, 66u, false));
    REQUIRE(refcounts[WZ_KEY_A] == 0u);
    REQUIRE(!wz_input_arbiter_key_down(&arbiter, WZ_KEY_A));

    /* Local release does not steal a key owned by Telnet. */
    REQUIRE(wz_input_arbiter_set(&arbiter, WZ_INPUT_SOURCE_TELNET,
                                 WZ_KEY_A, true));
    REQUIRE(wz_input_event_set_local_key(
        &mapping, &joystick_mapping, &kempston, &arbiter,
        host_key_down, refcounts, 65u, true));
    REQUIRE(wz_input_event_release_local_keys(
        &joystick_mapping, &kempston, &arbiter,
        host_key_down, refcounts));
    REQUIRE(wz_input_arbiter_key_down(&arbiter, WZ_KEY_A));
    REQUIRE(wz_input_arbiter_release_source(&arbiter, WZ_INPUT_SOURCE_TELNET));
    REQUIRE(!wz_input_arbiter_key_down(&arbiter, WZ_KEY_A));

    /* A mapped host key reaches Kempston independently of the matrix. */
    REQUIRE(wz_input_event_set_local_key(
        &mapping, &joystick_mapping, &kempston, &arbiter,
        host_key_down, refcounts, 100u, true));
    REQUIRE((wz_kempston_read(&kempston, WZ_KEMPSTON_PORT) & 0x01u) != 0u);
    REQUIRE(wz_input_event_release_local_keys(
        &joystick_mapping, &kempston, &arbiter,
        host_key_down, refcounts));
    REQUIRE(wz_kempston_read(&kempston, WZ_KEMPSTON_PORT) == 0u);

    wz_machine_destroy(&machine);
    puts("PASS host key events project to the Spectrum matrix and Kempston with source ownership");
    return 0;
}
