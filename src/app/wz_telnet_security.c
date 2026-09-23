/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_telnet_security.h"

static const char wz_telnet_permission_summary[] =
    "default-deny host-file/destructive/quit policy";

void wz_telnet_security_init(wz_telnet_security_state_t* state)
{
    if (state == NULL) return;
    state->plaintext = true;
    state->authenticated = false;
    state->encrypted = false;
}

bool wz_telnet_security_is_initial_baseline(
    const wz_telnet_security_state_t* state)
{
    return state != NULL && state->plaintext &&
           !state->authenticated && !state->encrypted;
}

void wz_telnet_security_apply_ui(
    const wz_telnet_security_state_t* state,
    wz_ui_remote_control_status_t* status)
{
    if (state == NULL || status == NULL) return;
    status->plaintext_no_authentication =
        state->plaintext && !state->authenticated && !state->encrypted;
    status->permission_summary = wz_telnet_permission_summary;
}
