/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_DIAGNOSTICS_WZ_DIAGNOSTICS_ROUTER_H
#define WZ_DIAGNOSTICS_WZ_DIAGNOSTICS_ROUTER_H

#include <stdbool.h>

#include "core/wz_trace.h"
#include "diagnostics/wz_syslog_udp.h"

typedef struct {
    wz_trace_emit_fn local_emit;
    void* local_context;
    wz_syslog_udp_t* syslog;
    bool forwarding_enabled;
} wz_diagnostics_router_t;

void wz_diagnostics_router_init(wz_diagnostics_router_t* router,
                                wz_trace_emit_fn local_emit,
                                void* local_context,
                                wz_syslog_udp_t* syslog,
                                bool forwarding_enabled);
void wz_diagnostics_router_emit(const wz_trace_event_t* event, void* context);
void wz_diagnostics_router_log(wz_diagnostics_router_t* router,
                               wz_syslog_severity_t severity,
                               const char* message);

#endif
