/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "diagnostics/wz_syslog_udp.h"
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#define WZ_SYSLOG_FACILITY_USER 1u
#define WZ_SYSLOG_SOURCE "wzsn"

static int severity_value(wz_syslog_severity_t severity)
{
    return severity <= WZ_SYSLOG_DEBUG ? (int)severity : (int)WZ_SYSLOG_INFO;
}

static size_t safe_message(char* output, size_t capacity, const char* message)
{
    size_t used = 0u;
    if (output == NULL || capacity == 0u) return 0u;
    if (message == NULL) message = "diagnostic event";
    while (message[used] != '\0' && used + 1u < capacity) {
        unsigned char c = (unsigned char)message[used];
        output[used++] = (c >= 32u && c <= 126u) ? (char)c : '?';
    }
    output[used] = '\0';
    return used;
}

size_t wz_syslog_udp_format_trace(char* output, size_t capacity,
                                  const wz_trace_event_t* event)
{
    int written;
    if (output == NULL || capacity == 0u || event == NULL) return 0u;
    written = snprintf(output, capacity,
                       "<%u>1 - %s - - - [wzsn seq=\"%llu\" tick=\"%llu\"] trace kind=\"%u\"",
                       WZ_SYSLOG_FACILITY_USER * 8u + WZ_SYSLOG_INFO,
                       WZ_SYSLOG_SOURCE,
                       (unsigned long long)event->sequence,
                       (unsigned long long)event->master_tick,
                       (unsigned)event->kind);
    if (written < 0 || (size_t)written >= capacity) {
        output[capacity - 1u] = '\0';
        return 0u;
    }
    return (size_t)written;
}

static void send_raw(wz_syslog_udp_t* transport, const char* packet, int length)
{
    if (transport == NULL || !transport->enabled || !transport->resolved ||
        transport->socket_handle < 0 || packet == NULL || length <= 0) return;
#if defined(_WIN32)
    (void)sendto((SOCKET)transport->socket_handle, packet, length, 0,
                 (const struct sockaddr*)transport->address,
                 (int)transport->address_size);
#else
    (void)sendto((int)transport->socket_handle, packet, (size_t)length, MSG_DONTWAIT,
                 (const struct sockaddr*)transport->address, transport->address_size);
#endif
}

static void send_packet(wz_syslog_udp_t* transport, wz_syslog_severity_t severity,
                        const char* message)
{
    char packet[WZ_SYSLOG_UDP_MAX_PACKET];
    char safe[WZ_SYSLOG_UDP_MAX_PACKET];
    int written;
    if (message == NULL) return;
    safe_message(safe, sizeof(safe), message);
    written = snprintf(packet, sizeof(packet), "<%u>1 - %s - - - - %s",
                       WZ_SYSLOG_FACILITY_USER * 8u + (unsigned)severity_value(severity),
                       WZ_SYSLOG_SOURCE, safe);
    if (written <= 0 || (size_t)written >= sizeof(packet)) return;
    send_raw(transport, packet, written);
}

void wz_syslog_udp_init(wz_syslog_udp_t* transport, bool enabled)
{
    if (transport != NULL) {
        memset(transport, 0, sizeof(*transport));
        transport->socket_handle = (intptr_t)-1;
        transport->enabled = enabled;
    }
}

wz_result_t wz_syslog_udp_open(wz_syslog_udp_t* transport)
{
    struct addrinfo hints;
    struct addrinfo* result = NULL;
    struct addrinfo* current;
    char port[16];
    if (transport == NULL) return WZ_RESULT_INVALID_ARGUMENT;
    if (!transport->enabled) return WZ_RESULT_OK;
#if defined(_WIN32)
    { WSADATA data;
      if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return WZ_RESULT_TRACE_FAILURE;
      transport->winsock_initialized = true;
    }
#endif
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    (void)snprintf(port, sizeof(port), "%u", WZ_SYSLOG_UDP_PORT);
    if (getaddrinfo(WZ_SYSLOG_UDP_HOST, port, &hints, &result) != 0) return WZ_RESULT_TRACE_FAILURE;
    for (current = result; current != NULL; current = current->ai_next) {
        intptr_t handle = (intptr_t)socket(current->ai_family, current->ai_socktype, current->ai_protocol);
#if defined(_WIN32)
        if ((SOCKET)handle == INVALID_SOCKET) continue;
#else
        if ((int)handle < 0) continue;
#endif
#if defined(_WIN32)
        { u_long nonblocking = 1u;
          if (ioctlsocket((SOCKET)handle, FIONBIO, &nonblocking) != 0) {
              closesocket((SOCKET)handle); continue;
          }
        }
#else
        { int flags = fcntl((int)handle, F_GETFL, 0);
          if (flags < 0 || fcntl((int)handle, F_SETFL, flags | O_NONBLOCK) != 0) {
              close((int)handle); continue;
          }
        }
#endif
        if ((size_t)current->ai_addrlen <= sizeof(transport->address)) {
            memcpy(transport->address, current->ai_addr, current->ai_addrlen);
            transport->address_size = current->ai_addrlen;
            transport->socket_handle = handle;
            transport->resolved = true;
            break;
        }
#if defined(_WIN32)
        closesocket((SOCKET)handle);
#else
        close(handle);
#endif
    }
    freeaddrinfo(result);
    if (!transport->resolved) {
#if defined(_WIN32)
        WSACleanup();
        transport->winsock_initialized = false;
#endif
        return WZ_RESULT_TRACE_FAILURE;
    }
    return WZ_RESULT_OK;
}

void wz_syslog_udp_close(wz_syslog_udp_t* transport)
{
    if (transport == NULL || transport->socket_handle < 0) return;
#if defined(_WIN32)
    closesocket((SOCKET)transport->socket_handle);
    if (transport->winsock_initialized) WSACleanup();
    transport->winsock_initialized = false;
#else
    close((int)transport->socket_handle);
#endif
    transport->socket_handle = (intptr_t)-1;
    transport->resolved = false;
}

void wz_syslog_udp_emit(const wz_trace_event_t* event, void* context)
{
    char packet[WZ_SYSLOG_UDP_MAX_PACKET];
    size_t length;
    wz_syslog_udp_t* transport = (wz_syslog_udp_t*)context;
    if (event == NULL || transport == NULL) return;
    length = wz_syslog_udp_format_trace(packet, sizeof(packet), event);
    if (length != 0u) send_raw(transport, packet, (int)length);
}

void wz_syslog_udp_log(wz_syslog_udp_t* transport,
                       wz_syslog_severity_t severity,
                       const char* message)
{
    send_packet(transport, severity, message);
}
