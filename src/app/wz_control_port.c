/*
 * Warajevo ZX Spectrum Next
 * Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
 * New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
 * Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
 * See LICENSE.txt and NOTICE.md for complete terms and provenance.
 */

#include "app/wz_control_port.h"

void wz_control_port_owner_init(wz_control_port_owner_t* owner)
{
    if (owner == NULL) return;
    owner->ipv4_socket = WZ_HOST_SOCKET_INVALID;
    owner->ipv6_socket = WZ_HOST_SOCKET_INVALID;
    owner->ipv4_active = false;
    owner->ipv6_active = false;
    owner->selected_port = 0;
}

wz_control_port_probe_result_t wz_control_port_probe(wz_control_port_owner_t* owner)
{
    uint32_t port;
    if (owner == NULL) return WZ_CONTROL_PORT_PROBE_UNAVAILABLE;
    wz_control_port_owner_init(owner);
    for (port = WZ_CONTROL_PORT_FIRST; port <= WZ_CONTROL_PORT_LAST; ++port) {
        wz_host_socket_t ipv4 = wz_host_socket_tcp_open_family(WZ_HOST_SOCKET_IPV4);
        wz_host_socket_t ipv6 = wz_host_socket_tcp_open_family(WZ_HOST_SOCKET_IPV6);
        bool ipv4_ok = ipv4 != WZ_HOST_SOCKET_INVALID &&
            wz_host_socket_bind_listen_family(ipv4, WZ_HOST_SOCKET_IPV4,
                                              (uint16_t)port, 16);
        wz_host_socket_bind_failure_t ipv4_failure = ipv4_ok
            ? WZ_HOST_SOCKET_BIND_OK
            : (ipv4 == WZ_HOST_SOCKET_INVALID
                ? WZ_HOST_SOCKET_BIND_UNAVAILABLE
                : wz_host_socket_last_bind_failure());
        bool ipv6_ok = ipv6 != WZ_HOST_SOCKET_INVALID &&
            wz_host_socket_bind_listen_family(ipv6, WZ_HOST_SOCKET_IPV6,
                                              (uint16_t)port, 16);
        wz_host_socket_bind_failure_t ipv6_failure = ipv6_ok
            ? WZ_HOST_SOCKET_BIND_OK
            : (ipv6 == WZ_HOST_SOCKET_INVALID
                ? WZ_HOST_SOCKET_BIND_UNAVAILABLE
                : wz_host_socket_last_bind_failure());
        bool ipv4_unavailable = !ipv4_ok && ipv4_failure == WZ_HOST_SOCKET_BIND_UNAVAILABLE;
        bool ipv6_unavailable = !ipv6_ok && ipv6_failure == WZ_HOST_SOCKET_BIND_UNAVAILABLE;
        bool ownership_conflict = (!ipv4_ok && !ipv4_unavailable) ||
                                  (!ipv6_ok && !ipv6_unavailable);
        if (!ownership_conflict && (ipv4_ok || ipv6_ok) &&
            (!ipv4_ok || wz_host_socket_set_nonblocking(ipv4, true)) &&
            (!ipv6_ok || wz_host_socket_set_nonblocking(ipv6, true))) {
            owner->ipv4_socket = ipv4;
            owner->ipv6_socket = ipv6;
            owner->ipv4_active = ipv4_ok;
            owner->ipv6_active = ipv6_ok;
            owner->selected_port = (uint16_t)port;
            return WZ_CONTROL_PORT_PROBE_AVAILABLE;
        }
        wz_host_socket_close(&ipv4);
        wz_host_socket_close(&ipv6);
    }
    return WZ_CONTROL_PORT_PROBE_UNAVAILABLE;
}

void wz_control_port_owner_close(wz_control_port_owner_t* owner)
{
    if (owner == NULL) return;
    wz_host_socket_close(&owner->ipv4_socket);
    wz_host_socket_close(&owner->ipv6_socket);
    owner->selected_port = 0;
    owner->ipv4_active = false;
    owner->ipv6_active = false;
}
