/*
 * Warajevo ZX Spectrum Next
 * Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
 * New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
 * Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
 * See LICENSE.txt and NOTICE.md for complete terms and provenance.
 */

#include "app/wz_telnet_client.h"
#include "app/wz_telnet_input.h"

static const char wz_telnet_busy[] = "BUSY\r\n";

void wz_telnet_client_gate_init(wz_telnet_client_gate_t* gate)
{
    if (gate != NULL) {
        gate->active_client = WZ_HOST_SOCKET_INVALID;
        gate->input_arbiter = NULL;
    }
}

void wz_telnet_client_bind_input(wz_telnet_client_gate_t* gate,
                                 wz_input_arbiter_t* arbiter)
{
    if (gate != NULL) gate->input_arbiter = arbiter;
}

wz_host_socket_t wz_telnet_client_accept(
    wz_telnet_client_gate_t* gate,
    wz_host_socket_t listener)
{
    wz_host_socket_t candidate;
    if (gate == NULL || listener == WZ_HOST_SOCKET_INVALID) {
        return WZ_HOST_SOCKET_INVALID;
    }
    candidate = wz_host_socket_accept(listener);
    if (candidate == WZ_HOST_SOCKET_INVALID) return candidate;
    if (gate->active_client != WZ_HOST_SOCKET_INVALID) {
        (void)wz_host_socket_send(candidate, wz_telnet_busy,
                                  sizeof(wz_telnet_busy) - 1u);
        wz_host_socket_close(&candidate);
        return WZ_HOST_SOCKET_INVALID;
    }
    gate->active_client = candidate;
    return candidate;
}

void wz_telnet_client_release(wz_telnet_client_gate_t* gate)
{
    if (gate == NULL) return;
    wz_host_socket_close(&gate->active_client);
}

void wz_telnet_client_disconnect(wz_telnet_client_gate_t* gate)
{
    /* The listener belongs to the caller and is intentionally untouched. */
    if (gate != NULL && gate->input_arbiter != NULL) {
        (void)wz_telnet_input_release_all(gate->input_arbiter);
    }
    wz_telnet_client_release(gate);
}

bool wz_telnet_client_is_active(const wz_telnet_client_gate_t* gate)
{
    return gate != NULL && gate->active_client != WZ_HOST_SOCKET_INVALID;
}
