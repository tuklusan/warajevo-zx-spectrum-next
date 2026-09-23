/*
 * Warajevo ZX Spectrum Next
 * Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
 * New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
 * Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
 * See LICENSE.txt and NOTICE.md for complete terms and provenance.
 */

#ifndef WZ_TELNET_CLIENT_H
#define WZ_TELNET_CLIENT_H

#include <stdbool.h>

#include "app/wz_host_socket.h"
#include "app/wz_input_arbiter.h"

typedef struct wz_telnet_client_gate {
    wz_host_socket_t active_client;
    wz_input_arbiter_t* input_arbiter;
} wz_telnet_client_gate_t;

void wz_telnet_client_gate_init(wz_telnet_client_gate_t* gate);
void wz_telnet_client_bind_input(wz_telnet_client_gate_t* gate,
                                 wz_input_arbiter_t* arbiter);
wz_host_socket_t wz_telnet_client_accept(
    wz_telnet_client_gate_t* gate,
    wz_host_socket_t listener);
void wz_telnet_client_release(wz_telnet_client_gate_t* gate);
void wz_telnet_client_disconnect(wz_telnet_client_gate_t* gate);
bool wz_telnet_client_is_active(const wz_telnet_client_gate_t* gate);

#endif
