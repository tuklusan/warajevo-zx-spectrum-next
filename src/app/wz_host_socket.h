/*
 * Warajevo ZX Spectrum Next
 * Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
 * New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
 * Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
 * See LICENSE.txt and NOTICE.md for complete terms and provenance.
 */

#ifndef WZ_HOST_SOCKET_H
#define WZ_HOST_SOCKET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef intptr_t wz_host_socket_t;

typedef enum wz_host_socket_family {
    WZ_HOST_SOCKET_IPV4 = 0,
    WZ_HOST_SOCKET_IPV6 = 1
} wz_host_socket_family_t;

typedef enum wz_host_socket_bind_failure {
    WZ_HOST_SOCKET_BIND_OK = 0,
    WZ_HOST_SOCKET_BIND_OWNERSHIP = 1,
    WZ_HOST_SOCKET_BIND_UNAVAILABLE = 2
} wz_host_socket_bind_failure_t;

#define WZ_HOST_SOCKET_INVALID ((wz_host_socket_t)-1)

bool wz_host_socket_system_init(void);
void wz_host_socket_system_shutdown(void);
wz_host_socket_t wz_host_socket_tcp_open(void);
wz_host_socket_t wz_host_socket_tcp_open_family(wz_host_socket_family_t family);
bool wz_host_socket_set_nonblocking(wz_host_socket_t socket_handle, bool enabled);
bool wz_host_socket_bind_listen(wz_host_socket_t socket_handle,
                                uint16_t port,
                                int backlog);
bool wz_host_socket_bind_listen_family(wz_host_socket_t socket_handle,
                                       wz_host_socket_family_t family,
                                       uint16_t port,
                                       int backlog);
wz_host_socket_bind_failure_t wz_host_socket_last_bind_failure(void);
wz_host_socket_t wz_host_socket_accept(wz_host_socket_t socket_handle);
int wz_host_socket_receive(wz_host_socket_t socket_handle,
                           void* buffer,
                           size_t capacity);
int wz_host_socket_send(wz_host_socket_t socket_handle,
                        const void* buffer,
                        size_t length);
void wz_host_socket_close(wz_host_socket_t* socket_handle);
bool wz_host_socket_would_block(void);

#endif
