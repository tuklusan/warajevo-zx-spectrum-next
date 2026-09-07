/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>
#include <string.h>

#include "app/wz_ui_layout.h"

int main(void)
{
    static const char* expected_menus[] = {
        "file", "machine", "media", "view", "tools", "settings", "help"
    };
    static const char* expected_toolbar[] = {
        "file.open_run", "machine.pause_resume", "machine.reset",
        "machine.speed", "media.tape", "snapshot.load", "snapshot.save",
        "media.microdrive.drive1", "host.screenshot.save", "view.fullscreen",
        "tools.debugger"
    };
    static const char* expected_toolbar_labels[] = {
        "Open / Run", "Pause / Resume", "Reset", "Emulation Speed",
        "Tape", "Load Snapshot", "Save Snapshot", "MDV 1", "Screenshot",
        "Fullscreen", "Debugger"
    };
    wz_ui_layout_state_t state;
    char status[WZ_UI_STATUS_CAPACITY];
    size_t index;

    if (wz_ui_layout_menu_count() != WZ_UI_MENU_COUNT ||
        wz_ui_layout_toolbar_count() != WZ_UI_TOOLBAR_COUNT) {
        return 1;
    }
    for (index = 0u; index < WZ_UI_MENU_COUNT; ++index) {
        const wz_ui_menu_node_t* item = wz_ui_layout_menu_at(index);
        if (item == 0 || strcmp(item->id, expected_menus[index]) != 0) {
            return 1;
        }
    }
    for (index = 0u; index < WZ_UI_TOOLBAR_COUNT; ++index) {
        const wz_ui_toolbar_item_t* item = wz_ui_layout_toolbar_at(index);
        if (item == 0 || strcmp(item->command_id, expected_toolbar[index]) != 0 ||
            strcmp(item->label, expected_toolbar_labels[index]) != 0) {
            return 1;
        }
    }
    if (wz_ui_layout_menu_at(WZ_UI_MENU_COUNT) != 0 ||
        wz_ui_layout_toolbar_at(WZ_UI_TOOLBAR_COUNT) != 0) {
        return 1;
    }
    if (wz_ui_layout_speed_count() != WZ_SPEED_COUNT ||
        strcmp(wz_ui_layout_speed_label(0u), "25%") != 0 ||
        strcmp(wz_ui_layout_speed_label(6u), "Unlimited") != 0 ||
        wz_ui_layout_speed_label(WZ_SPEED_COUNT) != 0) {
        return 1;
    }
    wz_ui_layout_state_init(&state);
    if (!wz_ui_layout_select_speed(&state, WZ_SPEED_400) ||
        state.speed_percent != 400u || !state.audio_muted ||
        !wz_ui_layout_select_speed(&state, WZ_SPEED_100) ||
        state.speed_percent != 100u || state.audio_muted ||
        !wz_ui_layout_select_speed(&state, WZ_SPEED_UNLIMITED) ||
        !state.unlimited_speed || !state.audio_muted ||
        wz_ui_layout_select_speed(&state, (wz_speed_policy_t)WZ_SPEED_COUNT)) {
        return 1;
    }
    if (!wz_ui_layout_select_speed(&state, WZ_SPEED_100)) {
        return 1;
    }
    wz_ui_layout_status_line(&state, status, sizeof(status));
    if (strstr(status, "Model: 48K") == 0 ||
        strstr(status, "Speed: 100%") == 0 ||
        strstr(status, "Control Port: unavailable") == 0) {
        return 1;
    }
    state.control_port_available = true;
    state.control_port = 30740u;
    state.paused = true;
    state.audio_muted = true;
    wz_ui_layout_status_line(&state, status, sizeof(status));
    if (strstr(status, "paused") == 0 ||
        strstr(status, "Control Port: 30740") == 0) {
        return 1;
    }
    puts("wz_ui_layout contract passed");
    return 0;
}
