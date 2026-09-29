/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_control_port.h"
#include "app/wz_host_socket.h"

#include <stdio.h>
#include <stdlib.h>

#define PORT_COUNT ((size_t)(WZ_CONTROL_PORT_LAST - WZ_CONTROL_PORT_FIRST + 1u))

static int occupy_family(wz_host_socket_t* sockets,
                         wz_host_socket_family_t family)
{
    for (size_t index = 0u; index < PORT_COUNT; ++index) {
        uint32_t port = (uint32_t)WZ_CONTROL_PORT_FIRST + (uint32_t)index;
        sockets[index] = wz_host_socket_tcp_open_family(family);
        if (sockets[index] == WZ_HOST_SOCKET_INVALID) return 0;
        if (!wz_host_socket_bind_listen_family(sockets[index], family,
                                                (uint16_t)port, 16)) {
            wz_host_socket_bind_failure_t failure =
                wz_host_socket_last_bind_failure();
            wz_host_socket_close(&sockets[index]);
            if (failure != WZ_HOST_SOCKET_BIND_OWNERSHIP) return 0;
        }
    }
    return 1;
}

int main(void)
{
    wz_host_socket_t* ipv4 = NULL;
    wz_control_port_owner_t owner;
    int initialized = 0;
    int owner_initialized = 0;
    int success = 0;

    ipv4 = (wz_host_socket_t*)malloc(PORT_COUNT * sizeof(*ipv4));
    if (ipv4 == NULL) goto cleanup;
    for (size_t index = 0u; index < PORT_COUNT; ++index) {
        ipv4[index] = WZ_HOST_SOCKET_INVALID;
    }
    if (!wz_host_socket_system_init()) goto cleanup;
    initialized = 1;
    if (!occupy_family(ipv4, WZ_HOST_SOCKET_IPV4)) goto cleanup;

    wz_control_port_owner_init(&owner);
    owner_initialized = 1;
    if (wz_control_port_probe(&owner) != WZ_CONTROL_PORT_PROBE_UNAVAILABLE ||
        owner.selected_port != 0u || owner.ipv4_active || owner.ipv6_active ||
        owner.ipv4_socket != WZ_HOST_SOCKET_INVALID ||
        owner.ipv6_socket != WZ_HOST_SOCKET_INVALID) goto cleanup;
    wz_control_port_owner_close(&owner);
    success = 1;
cleanup:
    if (owner_initialized) wz_control_port_owner_close(&owner);
    if (ipv4 != NULL) {
        for (size_t index = 0u; index < PORT_COUNT; ++index)
            wz_host_socket_close(&ipv4[index]);
    }
    free(ipv4);
    if (initialized) wz_host_socket_system_shutdown();
    if (!success) {
        (void)fputs("control-port exhaustion contract failed\n", stderr);
        return 1;
    }
    printf("control-port-exhaustion cases=1 occupied_ipv4_ports=%lu status=pass\n",
           (unsigned long)PORT_COUNT);
    return 0;
}
