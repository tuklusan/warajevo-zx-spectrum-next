/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_ui_keyboard_menu.h"

#include <string.h>

void wz_ui_keyboard_menu_init(wz_ui_keyboard_menu_t* menu)
{
    if (menu != NULL) memset(menu, 0, sizeof(*menu));
}

bool wz_ui_keyboard_menu_open(wz_ui_keyboard_menu_t* menu,
                              wz_input_focus_controller_t* focus)
{
    if (menu == NULL || focus == NULL || menu->active ||
        !wz_input_focus_is_focused(focus)) return false;
    menu->previous_target = wz_input_focus_target(focus);
    if (!wz_input_focus_dialog_enter(focus)) return false;
    menu->selection = 0u;
    menu->active = true;
    return true;
}

bool wz_ui_keyboard_menu_close(wz_ui_keyboard_menu_t* menu,
                               wz_input_focus_controller_t* focus)
{
    if (menu == NULL || focus == NULL || !menu->active) return false;
    menu->active = false;
    return wz_input_focus_set_target(focus, menu->previous_target);
}

bool wz_ui_keyboard_menu_set_return_target(
    wz_ui_keyboard_menu_t* menu, wz_input_focus_target_t target)
{
    if (menu == NULL || !menu->active ||
        (target != WZ_INPUT_FOCUS_VIEWPORT &&
         target != WZ_INPUT_FOCUS_TEXT_CONTROL)) return false;
    menu->previous_target = target;
    return true;
}

void wz_ui_keyboard_menu_focus_lost(
    wz_ui_keyboard_menu_t* menu,
    wz_input_focus_controller_t* focus)
{
    if (menu == NULL || !menu->active) return;
    if (focus == NULL) {
        menu->active = false;
        return;
    }
    (void)wz_ui_keyboard_menu_close(menu, focus);
}

bool wz_ui_keyboard_menu_move(wz_ui_keyboard_menu_t* menu,
                              size_t action_count, int direction)
{
    if (menu == NULL || !menu->active || action_count == 0u ||
        (direction != -1 && direction != 1)) return false;
    if (menu->selection >= action_count) menu->selection = 0u;
    if (direction < 0) {
        menu->selection = menu->selection == 0u ? action_count - 1u :
            menu->selection - 1u;
    } else {
        menu->selection = (menu->selection + 1u) % action_count;
    }
    return true;
}

size_t wz_ui_keyboard_menu_selection(const wz_ui_keyboard_menu_t* menu)
{
    return menu == NULL ? 0u : menu->selection;
}

bool wz_ui_keyboard_menu_is_active(const wz_ui_keyboard_menu_t* menu)
{
    return menu != NULL && menu->active;
}
