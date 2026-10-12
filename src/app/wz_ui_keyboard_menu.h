/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#ifndef WZ_APP_WZ_UI_KEYBOARD_MENU_H
#define WZ_APP_WZ_UI_KEYBOARD_MENU_H

#include <stdbool.h>
#include <stddef.h>

#include "app/wz_input_focus.h"

typedef struct {
    bool active;
    size_t selection;
    wz_input_focus_target_t previous_target;
} wz_ui_keyboard_menu_t;

void wz_ui_keyboard_menu_init(wz_ui_keyboard_menu_t* menu);
bool wz_ui_keyboard_menu_open(wz_ui_keyboard_menu_t* menu,
                              wz_input_focus_controller_t* focus);
bool wz_ui_keyboard_menu_close(wz_ui_keyboard_menu_t* menu,
                               wz_input_focus_controller_t* focus);
bool wz_ui_keyboard_menu_set_return_target(
    wz_ui_keyboard_menu_t* menu, wz_input_focus_target_t target);
void wz_ui_keyboard_menu_focus_lost(
    wz_ui_keyboard_menu_t* menu,
    wz_input_focus_controller_t* focus);
bool wz_ui_keyboard_menu_move(wz_ui_keyboard_menu_t* menu,
                              size_t action_count, int direction);
size_t wz_ui_keyboard_menu_selection(const wz_ui_keyboard_menu_t* menu);
bool wz_ui_keyboard_menu_is_active(const wz_ui_keyboard_menu_t* menu);

#endif
