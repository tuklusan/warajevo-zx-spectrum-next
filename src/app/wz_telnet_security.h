/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_TELNET_SECURITY_H
#define WZ_APP_WZ_TELNET_SECURITY_H

#include <stdbool.h>

#include "app/wz_ui_layout.h"

typedef struct wz_telnet_security_state {
    bool plaintext;
    bool authenticated;
    bool encrypted;
} wz_telnet_security_state_t;

void wz_telnet_security_init(wz_telnet_security_state_t* state);
bool wz_telnet_security_is_initial_baseline(
    const wz_telnet_security_state_t* state);
void wz_telnet_security_apply_ui(
    const wz_telnet_security_state_t* state,
    wz_ui_remote_control_status_t* status);

#endif
