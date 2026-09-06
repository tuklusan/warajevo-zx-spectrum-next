/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_APPLICATION_LIFECYCLE_H
#define WZ_APP_WZ_APPLICATION_LIFECYCLE_H

#include <stdbool.h>

#include "core/wz_types.h"

typedef enum {
    WZ_APPLICATION_RUNNING = 0,
    WZ_APPLICATION_QUIT_REQUESTED,
    WZ_APPLICATION_TERMINATED
} wz_application_state_t;

typedef void (*wz_application_quit_callback_t)(void* context);

typedef struct {
    wz_application_state_t state;
    wz_application_quit_callback_t quit_callback;
    void* quit_context;
    bool quit_callback_invoked;
} wz_application_lifecycle_t;

wz_result_t wz_application_lifecycle_init(
    wz_application_lifecycle_t* lifecycle,
    wz_application_quit_callback_t quit_callback,
    void* quit_context);
wz_result_t wz_application_request_quit(wz_application_lifecycle_t* lifecycle);
wz_result_t wz_application_mark_terminated(
    wz_application_lifecycle_t* lifecycle);
wz_result_t wz_application_read_state(
    const wz_application_lifecycle_t* lifecycle,
    wz_application_state_t* state);
bool wz_application_quit_requested(
    const wz_application_lifecycle_t* lifecycle);

#endif
