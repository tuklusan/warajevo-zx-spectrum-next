/*
 * Warajevo ZX Spectrum Next
 * Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
 * New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
 * Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
 * See LICENSE.txt and NOTICE.md for complete terms and provenance.
 */

#include "app/wz_host_socket.h"

#include <limits.h>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

static int wz_host_socket_size_to_int(size_t value)
{
    return value > (size_t)INT_MAX ? INT_MAX : (int)value;
}

static wz_host_socket_bind_failure_t wz_last_bind_failure = WZ_HOST_SOCKET_BIND_OK;

static void wz_host_socket_record_bind_failure(void)
{
#if defined(_WIN32)
    int error = WSAGetLastError();
    wz_last_bind_failure = error == WSAEADDRINUSE
        ? WZ_HOST_SOCKET_BIND_OWNERSHIP
        : WZ_HOST_SOCKET_BIND_UNAVAILABLE;
#else
    wz_last_bind_failure = errno == EADDRINUSE
        ? WZ_HOST_SOCKET_BIND_OWNERSHIP
        : WZ_HOST_SOCKET_BIND_UNAVAILABLE;
#endif
}

bool wz_host_socket_system_init(void)
{
#if defined(_WIN32)
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data) == 0;
#else
    return true;
#endif
}

void wz_host_socket_system_shutdown(void)
{
#if defined(_WIN32)
    (void)WSACleanup();
#endif
}

wz_host_socket_t wz_host_socket_tcp_open(void)
{
    return wz_host_socket_tcp_open_family(WZ_HOST_SOCKET_IPV4);
}

wz_host_socket_t wz_host_socket_tcp_open_family(wz_host_socket_family_t family)
{
    int address_family = family == WZ_HOST_SOCKET_IPV6 ? AF_INET6 : AF_INET;
#if defined(_WIN32)
    SOCKET handle = socket(address_family, SOCK_STREAM, IPPROTO_TCP);
    return handle == INVALID_SOCKET ? WZ_HOST_SOCKET_INVALID : (wz_host_socket_t)handle;
#else
    int handle = socket(address_family, SOCK_STREAM, IPPROTO_TCP);
    return handle < 0 ? WZ_HOST_SOCKET_INVALID : (wz_host_socket_t)handle;
#endif
}

bool wz_host_socket_set_nonblocking(wz_host_socket_t socket_handle, bool enabled)
{
#if defined(_WIN32)
    u_long mode = enabled ? 1UL : 0UL;
    return ioctlsocket((SOCKET)socket_handle, FIONBIO, &mode) == 0;
#else
    int flags = fcntl((int)socket_handle, F_GETFL, 0);
    if (flags < 0) return false;
    return fcntl((int)socket_handle,
                 F_SETFL,
                 enabled ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK)) == 0;
#endif
}

bool wz_host_socket_bind_listen(wz_host_socket_t socket_handle,
                                uint16_t port,
                                int backlog)
{
    return wz_host_socket_bind_listen_family(socket_handle,
                                             WZ_HOST_SOCKET_IPV4,
                                             port,
                                             backlog);
}

bool wz_host_socket_bind_listen_family(wz_host_socket_t socket_handle,
                                       wz_host_socket_family_t family,
                                       uint16_t port,
                                       int backlog)
{
    struct sockaddr_in address;
    struct sockaddr_in6 address6;
    int reuse = 1;
    int exclusive = 1;
    wz_last_bind_failure = WZ_HOST_SOCKET_BIND_UNAVAILABLE;
    if (socket_handle == WZ_HOST_SOCKET_INVALID || backlog < 0) return false;
    if (family == WZ_HOST_SOCKET_IPV6) {
        memset(&address6, 0, sizeof(address6));
        address6.sin6_family = AF_INET6;
        address6.sin6_addr = in6addr_any;
        address6.sin6_port = htons(port);
#if defined(_WIN32)
        int only_v6 = 1;
        if (setsockopt((SOCKET)socket_handle, IPPROTO_IPV6, IPV6_V6ONLY,
                       (const char*)&only_v6, sizeof(only_v6)) != 0) return false;
        if (setsockopt((SOCKET)socket_handle, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
                       (const char*)&exclusive, sizeof(exclusive)) != 0) return false;
        if (bind((SOCKET)socket_handle, (const struct sockaddr*)&address6,
                 (int)sizeof(address6)) != 0) { wz_host_socket_record_bind_failure(); return false; }
        if (listen((SOCKET)socket_handle, backlog) != 0) { wz_host_socket_record_bind_failure(); return false; }
        wz_last_bind_failure = WZ_HOST_SOCKET_BIND_OK;
        return true;
#else
        int only_v6 = 1;
        if (setsockopt((int)socket_handle, IPPROTO_IPV6, IPV6_V6ONLY,
                       &only_v6, sizeof(only_v6)) != 0) return false;
        if (setsockopt((int)socket_handle, SOL_SOCKET, SO_REUSEADDR,
                       &reuse, sizeof(reuse)) != 0) return false;
        if (bind((int)socket_handle, (const struct sockaddr*)&address6,
                 (socklen_t)sizeof(address6)) != 0) { wz_host_socket_record_bind_failure(); return false; }
        if (listen((int)socket_handle, backlog) != 0) { wz_host_socket_record_bind_failure(); return false; }
        wz_last_bind_failure = WZ_HOST_SOCKET_BIND_OK;
        return true;
#endif
    }
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);
#if defined(_WIN32)
    if (setsockopt((SOCKET)socket_handle, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
                   (const char*)&exclusive, sizeof(exclusive)) != 0) return false;
    if (bind((SOCKET)socket_handle, (const struct sockaddr*)&address,
             (int)sizeof(address)) != 0) { wz_host_socket_record_bind_failure(); return false; }
    if (listen((SOCKET)socket_handle, backlog) != 0) { wz_host_socket_record_bind_failure(); return false; }
    wz_last_bind_failure = WZ_HOST_SOCKET_BIND_OK;
    return true;
#else
    if (setsockopt((int)socket_handle, SOL_SOCKET, SO_REUSEADDR,
                   &reuse, sizeof(reuse)) != 0) return false;
    if (bind((int)socket_handle, (const struct sockaddr*)&address,
             (socklen_t)sizeof(address)) != 0) { wz_host_socket_record_bind_failure(); return false; }
    if (listen((int)socket_handle, backlog) != 0) { wz_host_socket_record_bind_failure(); return false; }
    wz_last_bind_failure = WZ_HOST_SOCKET_BIND_OK;
    return true;
#endif
}

wz_host_socket_bind_failure_t wz_host_socket_last_bind_failure(void)
{
    return wz_last_bind_failure;
}

wz_host_socket_t wz_host_socket_accept(wz_host_socket_t socket_handle)
{
#if defined(_WIN32)
    SOCKET accepted = accept((SOCKET)socket_handle, NULL, NULL);
    return accepted == INVALID_SOCKET ? WZ_HOST_SOCKET_INVALID : (wz_host_socket_t)accepted;
#else
    int accepted = accept((int)socket_handle, NULL, NULL);
    return accepted < 0 ? WZ_HOST_SOCKET_INVALID : (wz_host_socket_t)accepted;
#endif
}

int wz_host_socket_receive(wz_host_socket_t socket_handle,
                           void* buffer,
                           size_t capacity)
{
    if (buffer == NULL || capacity == 0) return 0;
#if defined(_WIN32)
    return recv((SOCKET)socket_handle, (char*)buffer,
                wz_host_socket_size_to_int(capacity), 0);
#else
    return (int)recv((int)socket_handle, buffer, capacity, 0);
#endif
}

int wz_host_socket_send(wz_host_socket_t socket_handle,
                        const void* buffer,
                        size_t length)
{
    if (buffer == NULL || length == 0) return 0;
#if defined(_WIN32)
    return send((SOCKET)socket_handle, (const char*)buffer,
                wz_host_socket_size_to_int(length), 0);
#else
    return (int)send((int)socket_handle, buffer, length, 0);
#endif
}

void wz_host_socket_close(wz_host_socket_t* socket_handle)
{
    if (socket_handle == NULL || *socket_handle == WZ_HOST_SOCKET_INVALID) return;
#if defined(_WIN32)
    (void)closesocket((SOCKET)*socket_handle);
#else
    (void)close((int)*socket_handle);
#endif
    *socket_handle = WZ_HOST_SOCKET_INVALID;
}

bool wz_host_socket_would_block(void)
{
#if defined(_WIN32)
    int error = WSAGetLastError();
    return error == WSAEWOULDBLOCK || error == WSAEINPROGRESS;
#else
    return errno == EAGAIN || errno == EWOULDBLOCK;
#endif
}
