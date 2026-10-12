/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#include "app/wz_keyboard_mapping.h"
#include "app/wz_kempston_mapping.h"

#include <stdio.h>

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "input mapping contract failed at line %d\n", \
                __LINE__); \
        return 1; \
    } \
} while (0)

int main(void)
{
    wz_keyboard_mapping_t keyboard;
    wz_kempston_mapping_t joystick_mapping;
    wz_keyboard_key_t spectrum_key = WZ_KEY_SHIFT;
    wz_kempston_t joystick;
    unsigned host_key = 0u;
    static const unsigned defaults[WZ_KEMPSTON_MAPPING_CONTROL_COUNT] = {
        100u, 101u, 102u, 103u, 104u
    };

    wz_keyboard_mapping_init(&keyboard);
    REQUIRE(!wz_keyboard_mapping_resolve(&keyboard, 65u, &spectrum_key));
    REQUIRE(wz_keyboard_mapping_bind(&keyboard, 65u, WZ_KEY_A));
    REQUIRE(wz_keyboard_mapping_resolve(&keyboard, 65u, &spectrum_key));
    REQUIRE(spectrum_key == WZ_KEY_A);
    REQUIRE(wz_keyboard_mapping_bind(&keyboard, 65u, WZ_KEY_B));
    REQUIRE(!wz_keyboard_mapping_host_for(&keyboard, WZ_KEY_A, &host_key));
    REQUIRE(wz_keyboard_mapping_host_for(&keyboard, WZ_KEY_B, &host_key));
    REQUIRE(host_key == 65u);
    REQUIRE(wz_keyboard_mapping_bind(&keyboard, 66u, WZ_KEY_B));
    REQUIRE(wz_keyboard_mapping_resolve(&keyboard, 66u, &spectrum_key));
    REQUIRE(spectrum_key == WZ_KEY_B);
    REQUIRE(wz_keyboard_mapping_unbind(&keyboard, WZ_KEY_B));
    REQUIRE(!wz_keyboard_mapping_resolve(&keyboard, 65u, &spectrum_key));
    REQUIRE(!wz_keyboard_mapping_bind(&keyboard,
        WZ_KEYBOARD_MAPPING_HOST_KEY_LIMIT, WZ_KEY_A));
    REQUIRE(!wz_keyboard_mapping_bind(&keyboard, 0u, WZ_KEY_A));
    keyboard.bindings[0].host_key = 67u;
    keyboard.bindings[0].spectrum_key =
        (wz_keyboard_key_t)WZ_KEYBOARD_MATRIX_KEY_COUNT;
    keyboard.bindings[0].bound = true;
    REQUIRE(!wz_keyboard_mapping_resolve(&keyboard, 67u, &spectrum_key));

    wz_kempston_init(&joystick);
    wz_kempston_mapping_init(&joystick_mapping, defaults);
    REQUIRE(wz_kempston_mapping_apply(&joystick_mapping, &joystick,
                                      100u, true));
    REQUIRE(joystick.pressed[WZ_KEMPSTON_RIGHT]);
    REQUIRE(wz_kempston_mapping_bind(&joystick_mapping,
        WZ_KEMPSTON_FIRE, 105u));
    REQUIRE(!wz_kempston_mapping_apply(&joystick_mapping, &joystick,
                                       104u, true));
    REQUIRE(wz_kempston_mapping_apply(&joystick_mapping, &joystick,
                                      105u, true));
    REQUIRE(joystick.pressed[WZ_KEMPSTON_FIRE]);
    REQUIRE(wz_kempston_mapping_apply(&joystick_mapping, &joystick,
                                      105u, false));
    REQUIRE(!joystick.pressed[WZ_KEMPSTON_FIRE]);
    REQUIRE(wz_kempston_mapping_unbind(&joystick_mapping,
                                       WZ_KEMPSTON_RIGHT));
    REQUIRE(!wz_kempston_mapping_apply(&joystick_mapping, &joystick,
                                       100u, true));
    puts("input mapping contract passed");
    return 0;
}
