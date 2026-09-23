/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_application_lifecycle.h"

wz_result_t wz_application_lifecycle_init(
    wz_application_lifecycle_t* lifecycle,
    wz_application_quit_callback_t quit_callback,
    void* quit_context)
{
    if (lifecycle == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    lifecycle->state = WZ_APPLICATION_RUNNING;
    lifecycle->quit_callback = quit_callback;
    lifecycle->quit_context = quit_context;
    lifecycle->quit_callback_invoked = false;
    return WZ_RESULT_OK;
}

wz_result_t wz_application_request_quit(wz_application_lifecycle_t* lifecycle)
{
    if (lifecycle == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (lifecycle->state == WZ_APPLICATION_TERMINATED) {
        return WZ_RESULT_INVALID_STATE;
    }
    if (lifecycle->state == WZ_APPLICATION_QUIT_REQUESTED) {
        return WZ_RESULT_OK;
    }

    lifecycle->state = WZ_APPLICATION_QUIT_REQUESTED;
    if (!lifecycle->quit_callback_invoked && lifecycle->quit_callback != 0) {
        lifecycle->quit_callback_invoked = true;
        lifecycle->quit_callback(lifecycle->quit_context);
    }
    return WZ_RESULT_OK;
}

wz_result_t wz_application_mark_terminated(
    wz_application_lifecycle_t* lifecycle)
{
    if (lifecycle == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (lifecycle->state == WZ_APPLICATION_RUNNING) {
        return WZ_RESULT_INVALID_STATE;
    }
    lifecycle->state = WZ_APPLICATION_TERMINATED;
    return WZ_RESULT_OK;
}

wz_result_t wz_application_read_state(
    const wz_application_lifecycle_t* lifecycle,
    wz_application_state_t* state)
{
    if (lifecycle == 0 || state == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    *state = lifecycle->state;
    return WZ_RESULT_OK;
}

bool wz_application_quit_requested(
    const wz_application_lifecycle_t* lifecycle)
{
    return lifecycle != 0 && lifecycle->state != WZ_APPLICATION_RUNNING;
}
