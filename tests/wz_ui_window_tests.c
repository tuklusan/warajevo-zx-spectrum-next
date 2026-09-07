/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>

#include "app/wz_ui_window.h"

int main(void)
{
    wz_ui_window_t window;
    const wz_ui_layout_state_t* layout;

    if (wz_ui_window_init(0)) {
        return 1;
    }
    if (!wz_ui_window_init(&window) ||
        !wz_ui_window_is_primary_display_visible(&window)) {
        return 1;
    }
    layout = wz_ui_window_layout(&window);
    if (layout == 0 || wz_ui_layout_menu_count() != 7u ||
        wz_ui_layout_toolbar_count() != 11u || layout->model_k != 48u) {
        return 1;
    }
    wz_ui_window_destroy(&window);
    if (wz_ui_window_is_primary_display_visible(&window) ||
        wz_ui_window_layout(&window) != 0) {
        return 1;
    }
    puts("wz_ui_window acceptance contract passed");
    return 0;
}
