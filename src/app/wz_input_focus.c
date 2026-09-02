/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_input_focus.h"

void wz_input_focus_init(wz_input_focus_controller_t* controller,
                         wz_input_arbiter_t* arbiter)
{
    if (controller != 0) {
        controller->arbiter = arbiter;
        controller->focused = true;
    }
}

bool wz_input_focus_lost(wz_input_focus_controller_t* controller)
{
    if (controller == 0 || controller->arbiter == 0) {
        return false;
    }
    if (controller->focused &&
        !wz_input_arbiter_release_source(controller->arbiter,
                                         WZ_INPUT_SOURCE_LOCAL)) {
        return false;
    }
    controller->focused = false;
    return true;
}

bool wz_input_focus_gained(wz_input_focus_controller_t* controller)
{
    if (controller == 0 || controller->arbiter == 0) {
        return false;
    }
    controller->focused = true;
    return true;
}

bool wz_input_focus_is_focused(const wz_input_focus_controller_t* controller)
{
    return controller != 0 && controller->focused;
}
