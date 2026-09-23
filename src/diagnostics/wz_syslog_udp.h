/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_DIAGNOSTICS_WZ_SYSLOG_UDP_H
#define WZ_DIAGNOSTICS_WZ_SYSLOG_UDP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "core/wz_trace.h"

#define WZ_SYSLOG_UDP_HOST "sanyalnet-oracle-vps2.duckdns.org"
#define WZ_SYSLOG_UDP_PORT 65514u
#define WZ_SYSLOG_UDP_MAX_PACKET 1024u

typedef enum {
    WZ_SYSLOG_EMERGENCY = 0u, WZ_SYSLOG_ALERT, WZ_SYSLOG_CRITICAL,
    WZ_SYSLOG_ERROR, WZ_SYSLOG_WARNING, WZ_SYSLOG_NOTICE,
    WZ_SYSLOG_INFO, WZ_SYSLOG_DEBUG
} wz_syslog_severity_t;

typedef struct {
    intptr_t socket_handle;
    bool enabled;
    bool resolved;
#if defined(_WIN32)
    bool winsock_initialized;
#endif
    unsigned char address[128];
    size_t address_size;
} wz_syslog_udp_t;

void wz_syslog_udp_init(wz_syslog_udp_t* transport, bool enabled);
wz_result_t wz_syslog_udp_open(wz_syslog_udp_t* transport);
void wz_syslog_udp_close(wz_syslog_udp_t* transport);
void wz_syslog_udp_emit(const wz_trace_event_t* event, void* context);
void wz_syslog_udp_log(wz_syslog_udp_t* transport,
                       wz_syslog_severity_t severity,
                       const char* message);
size_t wz_syslog_udp_format_trace(char* output, size_t capacity,
                                  const wz_trace_event_t* event);

#endif
