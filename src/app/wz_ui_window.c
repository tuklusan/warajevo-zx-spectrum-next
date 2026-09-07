/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_ui_window.h"

bool wz_ui_window_init(wz_ui_window_t* window)
{
    if (window == 0) {
        return false;
    }
    wz_ui_layout_state_init(&window->layout);
    window->primary_display_visible = true;
    window->initialized = true;
    return true;
}

void wz_ui_window_destroy(wz_ui_window_t* window)
{
    if (window == 0) {
        return;
    }
    window->primary_display_visible = false;
    window->initialized = false;
}

bool wz_ui_window_is_primary_display_visible(const wz_ui_window_t* window)
{
    return window != 0 && window->initialized && window->primary_display_visible;
}

const wz_ui_layout_state_t* wz_ui_window_layout(const wz_ui_window_t* window)
{
    return window != 0 && window->initialized ? &window->layout : 0;
}
