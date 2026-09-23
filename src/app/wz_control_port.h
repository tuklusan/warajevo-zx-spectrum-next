/*
 * Warajevo ZX Spectrum Next
 * Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
 * New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
 * Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
 * See LICENSE.txt and NOTICE.md for complete terms and provenance.
 */

#ifndef WZ_CONTROL_PORT_H
#define WZ_CONTROL_PORT_H

#include <stdbool.h>
#include <stdint.h>

#include "app/wz_host_socket.h"

#define WZ_CONTROL_PORT_FIRST ((uint16_t)30740)
#define WZ_CONTROL_PORT_LAST ((uint16_t)32787)

typedef enum wz_control_port_probe_result {
    WZ_CONTROL_PORT_PROBE_UNAVAILABLE = 0,
    WZ_CONTROL_PORT_PROBE_AVAILABLE = 1
} wz_control_port_probe_result_t;

typedef struct wz_control_port_owner {
    wz_host_socket_t ipv4_socket;
    wz_host_socket_t ipv6_socket;
    bool ipv4_active;
    bool ipv6_active;
    uint16_t selected_port;
} wz_control_port_owner_t;

void wz_control_port_owner_init(wz_control_port_owner_t* owner);
wz_control_port_probe_result_t wz_control_port_probe(wz_control_port_owner_t* owner);
void wz_control_port_owner_close(wz_control_port_owner_t* owner);

#endif
