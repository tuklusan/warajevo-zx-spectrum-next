/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_INPUT_FOCUS_H
#define WZ_APP_WZ_INPUT_FOCUS_H

#include <stdbool.h>

#include "app/wz_input_arbiter.h"

typedef enum {
    WZ_INPUT_FOCUS_VIEWPORT = 0,
    WZ_INPUT_FOCUS_TEXT_CONTROL
} wz_input_focus_target_t;

typedef struct {
    wz_input_arbiter_t* arbiter;
    bool focused;
    wz_input_focus_target_t target;
} wz_input_focus_controller_t;

void wz_input_focus_init(wz_input_focus_controller_t* controller,
                         wz_input_arbiter_t* arbiter);
bool wz_input_focus_lost(wz_input_focus_controller_t* controller);
bool wz_input_focus_gained(wz_input_focus_controller_t* controller);
bool wz_input_focus_is_focused(const wz_input_focus_controller_t* controller);
bool wz_input_focus_set_target(wz_input_focus_controller_t* controller,
                               wz_input_focus_target_t target);
bool wz_input_focus_forwards_viewport_keys(
    const wz_input_focus_controller_t* controller);
wz_input_focus_target_t wz_input_focus_target(
    const wz_input_focus_controller_t* controller);
bool wz_input_focus_dialog_enter(wz_input_focus_controller_t* controller);
bool wz_input_focus_dialog_leave(wz_input_focus_controller_t* controller);

#endif
