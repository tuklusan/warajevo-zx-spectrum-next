/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_UI_LAYOUT_H
#define WZ_APP_WZ_UI_LAYOUT_H

#include <stdbool.h>
#include <stddef.h>

#include "app/wz_speed_policy.h"
#include "app/wz_command_registry.h"

#define WZ_UI_MENU_COUNT 7u
#define WZ_UI_TOOLBAR_COUNT 11u
#define WZ_UI_STATUS_CAPACITY 192u
#define WZ_UI_MICRODRIVE_COUNT 8u

typedef struct {
    const char* id;
    const char* label;
} wz_ui_menu_node_t;

typedef struct {
    const char* command_id;
    const char* label;
} wz_ui_toolbar_item_t;

typedef struct {
    unsigned model_k;
    unsigned speed_percent;
    bool unlimited_speed;
    bool paused;
    bool audio_muted;
    bool tape_mounted;
    bool microdrive1_mounted;
    bool microdrive_mounted[WZ_UI_MICRODRIVE_COUNT];
    bool fullscreen;
    bool status_panel_visible;
    const char* networking_mode;
    bool remote_control_enabled;
    bool remote_control_client_connected;
    bool control_port_available;
    unsigned control_port;
} wz_ui_layout_state_t;

void wz_ui_layout_state_init(wz_ui_layout_state_t* state);
size_t wz_ui_layout_menu_count(void);
const wz_ui_menu_node_t* wz_ui_layout_menu_at(size_t index);
size_t wz_ui_layout_toolbar_count(void);
const wz_ui_toolbar_item_t* wz_ui_layout_toolbar_at(size_t index);
void wz_ui_layout_status_line(const wz_ui_layout_state_t* state,
                              char* output,
                              size_t capacity);
void wz_ui_layout_status_panel(const wz_ui_layout_state_t* state,
                               char* output,
                               size_t capacity);
bool wz_ui_layout_toggle_status_panel(wz_ui_layout_state_t* state);
size_t wz_ui_layout_speed_count(void);
const char* wz_ui_layout_speed_label(size_t index);
bool wz_ui_layout_select_speed(wz_ui_layout_state_t* state,
                               wz_speed_policy_t speed);
wz_result_t wz_ui_layout_activate_toolbar(
    const wz_command_registry_t* registry,
    size_t index,
    wz_command_arguments_t arguments,
    wz_command_result_t* result);
wz_command_state_t wz_ui_layout_command_state(
    const wz_command_registry_t* registry,
    const char* command_id,
    const char** disabled_reason);

#endif
