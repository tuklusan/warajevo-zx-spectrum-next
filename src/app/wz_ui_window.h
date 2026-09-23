/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_UI_WINDOW_H
#define WZ_APP_WZ_UI_WINDOW_H

#include <stdbool.h>

#include "app/wz_ui_layout.h"
#include "app/wz_telnet_client.h"

typedef struct {
    wz_ui_layout_state_t layout;
    wz_ui_remote_control_status_t remote_control;
    bool primary_display_visible;
    bool initialized;
} wz_ui_window_t;

bool wz_ui_window_init(wz_ui_window_t* window);
void wz_ui_window_destroy(wz_ui_window_t* window);
bool wz_ui_window_is_primary_display_visible(const wz_ui_window_t* window);
const wz_ui_layout_state_t* wz_ui_window_layout(const wz_ui_window_t* window);
const wz_ui_remote_control_status_t* wz_ui_window_remote_control(
    const wz_ui_window_t* window);
void wz_ui_window_sync_remote_control(
    wz_ui_window_t* window,
    const wz_control_port_owner_t* owner,
    const wz_telnet_client_gate_t* client);

#endif
