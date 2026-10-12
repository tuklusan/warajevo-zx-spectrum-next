/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_input_focus.h"
#include "app/wz_ui_keyboard_menu.h"

#include <stdio.h>

static unsigned cases;

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "Keyboard menu contract failed at line %d: %s\n", \
                      __LINE__, #condition); \
        return 1; \
    } \
    ++cases; \
} while (0)

int main(void)
{
    wz_input_arbiter_t arbiter;
    wz_input_focus_controller_t focus;
    wz_ui_keyboard_menu_t menu;
    const size_t action_count = 38u;
    wz_input_arbiter_init(&arbiter);
    wz_input_focus_init(&focus, &arbiter);
    wz_ui_keyboard_menu_init(&menu);
    REQUIRE(!wz_ui_keyboard_menu_is_active(&menu));
    REQUIRE(wz_ui_keyboard_menu_selection(&menu) == 0u);
    REQUIRE(!wz_ui_keyboard_menu_move(&menu, action_count, 1));
    REQUIRE(wz_input_arbiter_set(&arbiter, WZ_INPUT_SOURCE_LOCAL, 17u, true));
    REQUIRE(wz_input_arbiter_set(&arbiter, WZ_INPUT_SOURCE_TELNET, 17u, true));
    REQUIRE(wz_ui_keyboard_menu_open(&menu, &focus));
    REQUIRE(wz_ui_keyboard_menu_is_active(&menu));
    REQUIRE(wz_input_focus_target(&focus) == WZ_INPUT_FOCUS_TEXT_CONTROL);
    REQUIRE(!wz_input_focus_forwards_viewport_keys(&focus));
    REQUIRE(!wz_input_arbiter_source_key_down(&arbiter, WZ_INPUT_SOURCE_LOCAL, 17u));
    REQUIRE(wz_input_arbiter_source_key_down(&arbiter, WZ_INPUT_SOURCE_TELNET, 17u));
    REQUIRE(wz_ui_keyboard_menu_move(&menu, action_count, -1));
    REQUIRE(wz_ui_keyboard_menu_selection(&menu) == action_count - 1u);
    REQUIRE(wz_ui_keyboard_menu_move(&menu, action_count, 1));
    REQUIRE(wz_ui_keyboard_menu_selection(&menu) == 0u);
    REQUIRE(wz_ui_keyboard_menu_move(&menu, action_count, 1));
    REQUIRE(wz_ui_keyboard_menu_selection(&menu) == 1u);
    REQUIRE(!wz_ui_keyboard_menu_move(&menu, 0u, 1));
    REQUIRE(!wz_ui_keyboard_menu_move(&menu, action_count, 0));
    REQUIRE(!wz_ui_keyboard_menu_open(&menu, &focus));
    REQUIRE(wz_ui_keyboard_menu_close(&menu, &focus));
    REQUIRE(!wz_ui_keyboard_menu_is_active(&menu));
    REQUIRE(wz_input_focus_target(&focus) == WZ_INPUT_FOCUS_VIEWPORT);
    REQUIRE(wz_input_focus_forwards_viewport_keys(&focus));
    REQUIRE(wz_input_arbiter_source_key_down(&arbiter, WZ_INPUT_SOURCE_TELNET, 17u));
    REQUIRE(wz_ui_keyboard_menu_open(&menu, &focus));
    REQUIRE(wz_ui_keyboard_menu_set_return_target(
        &menu, WZ_INPUT_FOCUS_TEXT_CONTROL));
    REQUIRE(!wz_ui_keyboard_menu_set_return_target(
        &menu, (wz_input_focus_target_t)99));
    REQUIRE(wz_ui_keyboard_menu_close(&menu, &focus));
    REQUIRE(wz_input_focus_target(&focus) == WZ_INPUT_FOCUS_TEXT_CONTROL);
    REQUIRE(wz_input_focus_set_target(&focus, WZ_INPUT_FOCUS_VIEWPORT));
    REQUIRE(wz_ui_keyboard_menu_open(&menu, &focus));
    wz_ui_keyboard_menu_focus_lost(&menu, &focus);
    REQUIRE(!wz_ui_keyboard_menu_is_active(&menu));
    REQUIRE(wz_input_focus_lost(&focus));
    REQUIRE(wz_input_arbiter_source_key_down(&arbiter, WZ_INPUT_SOURCE_TELNET, 17u));
    REQUIRE(!wz_ui_keyboard_menu_close(&menu, &focus));
    REQUIRE(wz_input_focus_gained(&focus));
    REQUIRE(wz_ui_keyboard_menu_open(&menu, &focus));
    wz_ui_keyboard_menu_focus_lost(&menu, NULL);
    REQUIRE(!wz_ui_keyboard_menu_is_active(&menu));
    if (cases != 39u) {
        (void)fprintf(stderr, "Expected 39 assertions, ran %u\n", cases);
        return 1;
    }
    (void)puts("Keyboard menu focus and navigation contract passed (39 cases)");
    return 0;
}
